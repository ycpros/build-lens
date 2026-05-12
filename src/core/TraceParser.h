// ============================================================
// BuildLens — C++ 编译性能观测工具
//
// TraceParser.h
// 功能：解析 Clang -ftime-trace 输出的 JSON 文件，
//       提取每个源文件的编译耗时数据。
// ============================================================

#ifndef BUILD_LENS_TRACEPARSER_H_
#define BUILD_LENS_TRACEPARSER_H_

#include <QString>
#include <vector>
#include <QJsonArray>

#include "core/TraceRecord.h"

// -ftime-trace 文件解析器。
//
// 用法：
//   auto records = TraceParser::ParseDirectory("path/to/build");
//
// 输入格式：Clang -ftime-trace 为每个 .cpp 生成同名 .json 文件，
// 例如 renderer.cpp → renderer.cpp.json。
// JSON 采用 Chrome Trace Event 格式，总耗时记录在
// ph=="X" && name=="ExecuteCompiler" 的事件中，dur 单位为微秒。
class TraceParser {
 public:
  // 解析目录下所有 -ftime-trace JSON 文件。
  // dir_path：包含 .json 文件的目录路径。
  // 返回解析成功得到的 TraceRecord 列表；解析失败的文件会被跳过。
  static std::vector<TraceRecord> ParseDirectory(const QString& dir_path);

 private:
  // 解析单个 -ftime-trace JSON 文件。
  // file_path：单个 .json 文件的完整路径。
  // 返回值：解析成功则填充 TraceRecord，失败返回空值（std::nullopt 语义暂用空对象标记）。
  static TraceRecord ParseFile(const QString& file_path);

  // 从 JSON 文件路径推断源文件名。
  // 例如 "D:/build/renderer.cpp.json" → "renderer.cpp"。
  static QString InferSourceName(const QString& json_path);

  // 从 -ftime-trace JSON 的 traceEvents 数组中提取总耗时（微秒）。
  // 返回微秒值；未找到 Total 事件则返回 0.0。
  static double ExtractTotalDuration(const QJsonArray& events);
};

#endif  // BUILD_LENS_TRACEPARSER_H_
