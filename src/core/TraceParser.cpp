// ============================================================
// BuildLens — C++ 编译性能观测工具
//
// TraceParser.cpp
// 功能：TraceParser 类实现，解析 -ftime-trace JSON 并提取编译事件。
//       v0.2 扩展：支持提取全部 trace 事件（ParseFileEvents），
//       不再仅提取总耗时。正确处理 Clang 的 ph:"b"/"e" 对事件。
// ============================================================

#include "core/TraceParser.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDirIterator>
#include <QRegularExpression>

#include <algorithm>
#include <map>

// 支持的 JSON 文件后缀。
static const char* kJsonSuffix = "*.json";

namespace {

bool IsSummaryName(const QString& name) {
  return name.startsWith(QStringLiteral("Total "));
}

// 从事件名称推断类别。
// Clang -ftime-trace 的 Complete 事件（ph:"X"）不包含 cat 字段，
// 但事件名遵循固定命名约定，可据此推断类别。
QString InferCategory(const QString& name) {
  if (IsSummaryName(name)) {
    return InferCategory(name.mid(6));
  }
  if (name.startsWith("Source")) return QStringLiteral("source");
  if (name.startsWith("Parse") || name == "Frontend")
    return QStringLiteral("parse");
  if (name.startsWith("Instantiate") || name.startsWith("Deduce") ||
      name.startsWith("Specialize") ||
      name == "PerformPendingInstantiations")
    return QStringLiteral("instantiate");
  if (name.startsWith("CodeGen") || name.startsWith("Emit") ||
      name.startsWith("RunPass") || name.startsWith("Opt") ||
      name == "Backend")
    return QStringLiteral("codegen");
  if (name.startsWith("Debug"))
    return QStringLiteral("debug");
  if (name == "ExecuteCompiler")
    return QStringLiteral("driver");
  return QStringLiteral("other");
}

QString NormalizeCategory(const QJsonObject& obj, const QString& name) {
  QString category = obj.value("cat").toString().trimmed();
  if (!category.isEmpty()) {
    return category.toLower();
  }
  return InferCategory(name);
}

QString NormalizeDetail(QString detail) {
  detail = detail.trimmed();
  const QString spelling_marker = QStringLiteral("<Spelling=");
  const int spelling_start = detail.indexOf(spelling_marker);
  if (spelling_start >= 0) {
    const int value_start = spelling_start + spelling_marker.size();
    const int value_end = detail.indexOf('>', value_start);
    if (value_end > value_start) {
      detail = detail.mid(value_start, value_end - value_start).trimmed();
    }
  }

  detail.replace('\\', '/');

  static const QRegularExpression kTrailingLocation(
      QStringLiteral("(:\\d+)(:\\d+)?$"));
  detail.remove(kTrailingLocation);
  return detail;
}

QString EventIdToString(const QJsonValue& value) {
  if (value.isString()) {
    return value.toString();
  }
  if (value.isDouble()) {
    return QString::number(static_cast<qint64>(value.toDouble()));
  }
  return {};
}

QString BeginEndKey(const QJsonObject& obj) {
  return QStringLiteral("%1:%2:%3:%4")
      .arg(obj.value("pid").toInt(1))
      .arg(obj.value("tid").toInt(1))
      .arg(obj.value("name").toString())
      .arg(EventIdToString(obj.value("id")));
}

TraceEvent MakeTraceEvent(const QJsonObject& obj,
                          const QString& phase,
                          double ts,
                          double dur) {
  TraceEvent ev;
  ev.name = obj.value("name").toString();
  ev.category = NormalizeCategory(obj, ev.name);
  ev.detail = NormalizeDetail(
      obj.value("args").toObject().value("detail").toString());
  ev.phase = phase;
  ev.ts = ts;
  ev.dur = dur;
  ev.pid = obj.value("pid").toInt(1);
  ev.tid = obj.value("tid").toInt(1);
  ev.is_summary = IsSummaryName(ev.name);
  return ev;
}

}  // namespace

// ----- 私有辅助方法 -----

// static
QString TraceParser::InferSourceName(const QString& json_path) {
  QFileInfo file_info(json_path);
  QString base_name = file_info.fileName();
  if (base_name.endsWith(".json", Qt::CaseInsensitive)) {
    base_name.chop(5);
  }
  return base_name;
}

// static
FileTraceResult TraceParser::ParseFileEvents(const QString& file_path) {
  FileTraceResult result;
  result.source_path = file_path;
  result.filename = InferSourceName(file_path);

  QFile file(file_path);
  if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
    return result;
  }

  QByteArray content = file.readAll();
  file.close();

  QJsonParseError parse_error;
  QJsonDocument doc = QJsonDocument::fromJson(content, &parse_error);
  if (parse_error.error != QJsonParseError::NoError) {
    return result;
  }

  QJsonObject root = doc.object();
  QJsonArray events_array = root.value("traceEvents").toArray();

  // 按原始顺序匹配 ph:"b"/"e" 对事件。同一键使用栈，固定 LIFO 语义。
  std::map<QString, std::vector<QJsonObject>> begin_stacks;

  for (const QJsonValue& value : events_array) {
    QJsonObject obj = value.toObject();
    QString ph = obj.value("ph").toString();

    if (ph == "X" || (ph.isEmpty() && obj.contains("dur"))) {
      // Complete Event：直接创建 TraceEvent。
      TraceEvent ev = MakeTraceEvent(
          obj,
          ph.isEmpty() ? QStringLiteral("X") : ph,
          obj.value("ts").toDouble(0.0),
          obj.value("dur").toDouble(0.0));
      if (ev.dur > 0.0) {
        result.events.push_back(ev);
      }
    } else if (ph == "b") {
      begin_stacks[BeginEndKey(obj)].push_back(obj);
    } else if (ph == "e") {
      QString key = BeginEndKey(obj);
      auto it = begin_stacks.find(key);
      if (it != begin_stacks.end() && !it->second.empty()) {
        QJsonObject begin_obj = it->second.back();
        it->second.pop_back();
        if (it->second.empty()) {
          begin_stacks.erase(it);
        }

        double start_ts = begin_obj.value("ts").toDouble(0.0);
        double end_ts = obj.value("ts").toDouble(0.0);
        double dur = end_ts - start_ts;

        if (dur > 0.0) {
          TraceEvent ev = MakeTraceEvent(
              begin_obj, QStringLiteral("X"), start_ts, dur);
          result.events.push_back(ev);
        }
      }
    }
    // 忽略 ph:"M"（元数据）、ph:"i"（瞬时）等其他类型。
  }

  // 按 ts 升序排列，方便 GraphBuilder 的栈算法。
  std::sort(result.events.begin(), result.events.end(),
            [](const TraceEvent& a, const TraceEvent& b) {
              if (a.ts != b.ts) return a.ts < b.ts;
              return a.dur > b.dur;
            });

  return result;
}

// static
std::vector<FileTraceResult> TraceParser::ParseDirectoryEvents(
    const QString& dir_path) {
  std::vector<FileTraceResult> results;

  QDir dir(dir_path);
  if (!dir.exists()) {
    return results;
  }

  QStringList json_files;
  QDirIterator it(dir_path,
                  {kJsonSuffix},
                  QDir::Files,
                  QDirIterator::Subdirectories);
  while (it.hasNext()) {
    it.next();
    json_files << it.filePath();
  }

  std::sort(json_files.begin(), json_files.end(), [](const QString& a,
                                                     const QString& b) {
    return QFileInfo(a).fileName() < QFileInfo(b).fileName();
  });

  for (const QString& full_path : json_files) {
    FileTraceResult result = ParseFileEvents(full_path);
    if (!result.events.empty()) {
      results.push_back(result);
    }
  }

  return results;
}
