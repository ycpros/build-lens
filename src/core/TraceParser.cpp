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

#include <algorithm>
#include <unordered_map>

// 支持的 JSON 文件后缀。
static const char* kJsonSuffix = "*.json";

// 从事件名称推断类别。
// Clang -ftime-trace 的 Complete 事件（ph:"X"）不包含 cat 字段，
// 但事件名遵循固定命名约定，可据此推断类别。
static QString InferCategory(const QString& name) {
  if (name.startsWith("Source")) return QStringLiteral("source");
  if (name.startsWith("Parse") || name == "Frontend")
    return QStringLiteral("parse");
  if (name.startsWith("Instantiate") || name.startsWith("Deduce") ||
      name.startsWith("Specialize"))
    return QStringLiteral("instantiate");
  if (name.startsWith("CodeGen") || name.startsWith("Emit") ||
      name.startsWith("RunPass") || name.startsWith("Opt") ||
      name == "Backend")
    return QStringLiteral("codegen");
  if (name.startsWith("Debug"))
    return QStringLiteral("debug");
  if (name == "ExecuteCompiler")
    return QStringLiteral("driver");
  if (name.startsWith("Total "))
    return InferCategory(name.mid(6));  // 去掉 "Total " 前缀
  return QStringLiteral("other");
}

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
double TraceParser::ExtractTotalDuration(const QJsonArray& events) {
  // 真实 Clang 格式：ph=="X" && name=="ExecuteCompiler"
  for (const QJsonValue& value : events) {
    QJsonObject event = value.toObject();
    if (event.value("ph").toString() == "X" &&
        event.value("name").toString() == "ExecuteCompiler") {
      return event.value("dur").toDouble(0.0);
    }
  }
  // 回退：示例数据格式 cat=="phase" && name=="Total"
  for (const QJsonValue& value : events) {
    QJsonObject event = value.toObject();
    if (event.value("cat").toString() == "phase" &&
        event.value("name").toString() == "Total") {
      return event.value("dur").toDouble(0.0);
    }
  }
  return 0.0;
}

// ----- v0.2 新增：解析全部事件 -----

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

  // 阶段 1：收集 ph:"X" 完成事件。
  // 阶段 2：匹配 ph:"b"/"e" 对事件，计算持续时间。
  // 使用 id 字段匹配 b/e 对。
  std::unordered_map<int, double> begin_timestamps;  // id → ts (微秒)
  std::unordered_map<int, QJsonObject> begin_objects; // id → begin 事件对象

  for (const QJsonValue& value : events_array) {
    QJsonObject obj = value.toObject();
    QString ph = obj.value("ph").toString();

    if (ph == "X") {
      // Complete Event：直接创建 TraceEvent。
      TraceEvent ev;
      ev.name = obj.value("name").toString();
      ev.category = InferCategory(ev.name);
      ev.phase = ph;
      ev.ts = obj.value("ts").toDouble(0.0);
      ev.dur = obj.value("dur").toDouble(0.0);
      ev.pid = obj.value("pid").toInt(1);
      ev.tid = obj.value("tid").toInt(1);

      if (ev.dur > 0.0) {
        result.events.push_back(ev);
      }
    } else if (ph == "b") {
      // Begin Event：暂存 ts 和原始对象。
      int id = obj.value("id").toInt(-1);
      if (id >= 0) {
        begin_timestamps[id] = obj.value("ts").toDouble(0.0);
        begin_objects[id] = obj;
      }
    } else if (ph == "e") {
      // End Event：匹配对应的 begin，计算 duration。
      int id = obj.value("id").toInt(-1);
      if (id >= 0 && begin_timestamps.count(id)) {
        double start_ts = begin_timestamps[id];
        double end_ts = obj.value("ts").toDouble(0.0);
        double dur = end_ts - start_ts;

        if (dur > 0.0) {
          const QJsonObject& begin_obj = begin_objects[id];

          TraceEvent ev;
          ev.name = begin_obj.value("name").toString();
          ev.category = InferCategory(ev.name);
          ev.phase = QStringLiteral("X");  // 归一化为 Complete Event
          ev.ts = start_ts;
          ev.dur = dur;
          ev.pid = begin_obj.value("pid").toInt(1);
          ev.tid = begin_obj.value("tid").toInt(1);

          result.events.push_back(ev);
        }

        begin_timestamps.erase(id);
        begin_objects.erase(id);
      }
    }
    // 忽略 ph:"M"（元数据）、ph:"i"（瞬时）等其他类型。
  }

  // 按 ts 升序排列，方便 GraphBuilder 的栈算法。
  std::sort(result.events.begin(), result.events.end(),
            [](const TraceEvent& a, const TraceEvent& b) {
              return a.ts < b.ts;
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

  QDirIterator it(dir_path,
                  {kJsonSuffix},
                  QDir::Files,
                  QDirIterator::Subdirectories);
  while (it.hasNext()) {
    it.next();
    FileTraceResult result = ParseFileEvents(it.filePath());
    if (!result.events.empty()) {
      results.push_back(result);
    }
  }

  return results;
}

// ----- v0.1 兼容接口（保持向后兼容）-----

// static
TraceRecord TraceParser::ParseFile(const QString& file_path) {
  TraceRecord record;
  record.source_path = file_path;
  record.filename = InferSourceName(file_path);

  QFile file(file_path);
  if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
    return record;
  }

  QByteArray content = file.readAll();
  file.close();

  QJsonParseError parse_error;
  QJsonDocument doc = QJsonDocument::fromJson(content, &parse_error);
  if (parse_error.error != QJsonParseError::NoError) {
    return record;
  }

  QJsonObject root = doc.object();
  QJsonArray events = root.value("traceEvents").toArray();

  double total_us = ExtractTotalDuration(events);
  record.total_duration_ms = total_us / 1000.0;

  return record;
}

// static
std::vector<TraceRecord> TraceParser::ParseDirectory(
    const QString& dir_path) {
  std::vector<TraceRecord> records;

  QDir dir(dir_path);
  if (!dir.exists()) {
    return records;
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

  std::sort(json_files.begin(), json_files.end(), [](const QString &a, const QString &b){
      return QFileInfo(a).fileName() < QFileInfo(b).fileName();
  });

  for (const QString& full_path : json_files) {
    TraceRecord record = ParseFile(full_path);
    if (record.total_duration_ms > 0.0) {
      records.push_back(record);
    }
  }

  return records;
}
