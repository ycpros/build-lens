// ============================================================
// BuildLens — C++ 编译性能观测工具
//
// TraceParser.h
// 功能：解析 Clang -ftime-trace 输出的 JSON 文件，
//       提取编译耗时数据，支持两种粒度：
//       - ParseDirectory()：每个文件一条总耗时记录（v0.1 兼容）
//       - ParseDirectoryEvents()：每个文件返回全部事件（v0.2 新增）
// ============================================================

#ifndef BUILD_LENS_TRACEPARSER_H_
#define BUILD_LENS_TRACEPARSER_H_

#include <QString>
#include <QJsonArray>

#include <vector>

#include "core/TraceEvent.h"
#include "core/TraceRecord.h"

// 单个文件的解析结果（含全部事件）。
// 用于 v0.2 分析管线：TraceGraphBuilder → Analyzers。
struct FileTraceResult {
  // 从 JSON 路径推断出的源文件名，例如 "renderer.cpp"。
  QString filename;

  // JSON 文件完整路径。
  QString source_path;

  // 该文件的所有 trace 事件。
  // TraceGraphBuilder 会对这些事件按线程构建调用树。
  std::vector<TraceEvent> events;
};

// -ftime-trace 文件解析器。
//
// v0.1 用法（桌面 UI 兼容）：
//   auto records = TraceParser::ParseDirectory("path/to/build");
//   → 每个文件一条 TraceRecord（仅总耗时）。
//
// v0.2 用法（分析管线）：
//   auto results = TraceParser::ParseDirectoryEvents("path/to/build");
//   → 每个文件返回所有 TraceEvent，供 TraceGraphBuilder 构建调用树。
class TraceParser {
 public:
  // ----- v0.1 兼容接口 -----

  // 解析目录下所有 -ftime-trace JSON 文件，返回每条文件的总耗时记录。
  // dir_path：包含 .json 文件的目录路径（递归子目录）。
  static std::vector<TraceRecord> ParseDirectory(const QString& dir_path);

  // ----- v0.2 新增接口 -----

  // 解析目录下所有 -ftime-trace JSON 文件，返回每个文件的全部事件。
  // dir_path：包含 .json 文件的目录路径（递归子目录）。
  // 返回每个文件的 FileTraceResult，其中 events 按 ts 升序排列。
  static std::vector<FileTraceResult> ParseDirectoryEvents(
      const QString& dir_path);

  // 解析单个 JSON 文件返回总耗时记录。
  // file_path：JSON 文件的完整路径。
  static TraceRecord ParseFile(const QString& file_path);

  // 解析单个 -ftime-trace JSON 文件为 FileTraceResult。
  // file_path：JSON 文件的完整路径。
  static FileTraceResult ParseFileEvents(const QString& file_path);

  // 从 JSON 文件路径推断源文件名。
  // 例如 "D:/build/renderer.cpp.json" → "renderer.cpp"。
  static QString InferSourceName(const QString& json_path);

  // 从 -ftime-trace JSON 的 traceEvents 数组中提取总耗时（微秒）。
  // 支持两种格式：
  //   - 真实 Clang 格式：ph=="X" && name=="ExecuteCompiler"
  //   - 简化示例格式：cat=="phase" && name=="Total"
  static double ExtractTotalDuration(const QJsonArray& events);
};

#endif  // BUILD_LENS_TRACEPARSER_H_
