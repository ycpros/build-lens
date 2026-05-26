// ============================================================
// BuildLens — C++ 编译性能观测工具
//
// TraceParser.h
// 功能：解析 Clang -ftime-trace 输出的 JSON 文件，
//       提取完整 trace 事件，供 AnalysisPipeline 和 TraceGraphBuilder 使用。
// ============================================================

#ifndef BUILD_LENS_TRACEPARSER_H_
#define BUILD_LENS_TRACEPARSER_H_

#include <QString>

#include <vector>

#include "core/TraceEvent.h"

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
// 推荐用法：
//   auto result = AnalysisPipeline::Run("path/to/build");
//   → 返回完整 AnalysisReport。
//
// 事件级用法：
//   auto results = TraceParser::ParseDirectoryEvents("path/to/build");
//   → 每个文件返回所有 TraceEvent，供 TraceGraphBuilder 构建调用树。
class TraceParser {
 public:
  // 解析目录下所有 -ftime-trace JSON 文件，返回每个文件的全部事件。
  // dir_path：包含 .json 文件的目录路径（递归子目录）。
  // 返回每个文件的 FileTraceResult，其中 events 按 ts 升序排列。
  static std::vector<FileTraceResult> ParseDirectoryEvents(
      const QString& dir_path);

  // 解析单个 -ftime-trace JSON 文件为 FileTraceResult。
  // file_path：JSON 文件的完整路径。
  static FileTraceResult ParseFileEvents(const QString& file_path);

  // 从 JSON 文件路径推断源文件名。
  // 例如 "D:/build/renderer.cpp.json" → "renderer.cpp"。
  static QString InferSourceName(const QString& json_path);
};

#endif  // BUILD_LENS_TRACEPARSER_H_
