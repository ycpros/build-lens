// ============================================================
// BuildLens — C++ 编译性能观测工具
//
// TraceParser.cpp
// 功能：TraceParser 类实现，解析 -ftime-trace JSON 并提取编译耗时。
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

// 支持的 JSON 文件后缀。
static const char* kJsonSuffix = "*.json";

// ----- 私有辅助方法 -----

// static
QString TraceParser::InferSourceName(const QString& json_path) {
  // 取文件名部分，例如 "renderer.cpp.json"。
  QFileInfo file_info(json_path);
  QString base_name = file_info.fileName();
  // 去掉末尾的 ".json" 后缀。
  if (base_name.endsWith(".json", Qt::CaseInsensitive)) {
    base_name.chop(5);  // 去掉 ".json"
  }
  return base_name;
}

// static
double TraceParser::ExtractTotalDuration(const QJsonArray& events) {
  // Clang -ftime-trace 使用 Chrome Trace Event 格式。
  // 总编译耗时记录在 ph=="X" && name=="ExecuteCompiler" 的事件中，
  // dur 字段单位为微秒。
  for (const QJsonValue& value : events) {
    QJsonObject event = value.toObject();
    if (event.value("ph").toString() == "X" &&
        event.value("name").toString() == "ExecuteCompiler") {
      // dur 字段单位为微秒，转换为毫秒后返回。
      return event.value("dur").toDouble(0.0) / 1000.0;
    }
  }
  return 0.0;
}

// ----- 公有接口 -----

// static
TraceRecord TraceParser::ParseFile(const QString& file_path) {
  TraceRecord record;
  record.source_path = file_path;
  record.filename = InferSourceName(file_path);

  // 读取 JSON 文件内容。
  QFile file(file_path);
  if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
    return record;  // 文件无法打开，返回空记录。
  }

  QByteArray content = file.readAll();
  file.close();

  // 解析 JSON。
  QJsonParseError parse_error;
  QJsonDocument doc = QJsonDocument::fromJson(content, &parse_error);
  if (parse_error.error != QJsonParseError::NoError) {
    return record;  // JSON 格式错误，返回空记录。
  }

  // 提取 traceEvents 数组并查找 Total 耗时。
  QJsonObject root = doc.object();
  QJsonArray events = root.value("traceEvents").toArray();
  record.total_duration_ms = ExtractTotalDuration(events);

  return record;
}

// static
std::vector<TraceRecord> TraceParser::ParseDirectory(const QString& dir_path) {
  std::vector<TraceRecord> records;

  QDir dir(dir_path);
  if (!dir.exists()) {
    return records;  // 目录不存在，返回空列表。
  }

  // 使用 QDirIterator 遍历目录及所有子目录
  QStringList json_files;
  QDirIterator it(dir_path,
                  {kJsonSuffix},               // 只匹配 .json 文件
                  QDir::Files,                  // 只处理文件
                  QDirIterator::Subdirectories); // 递归子目录
  while (it.hasNext()) {
    it.next();
    json_files << it.filePath(); // 获取完整路径
  }

  // 按文件名排序以保证输出稳定
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
