// ============================================================
// BuildLens — C++ 编译性能观测工具
//
// HotspotAnalyzer.h
// 功能：按维度（事件名称 / 类别）聚合事件的总耗时，
//       找出耗时最高的 Top-N 热点。
// ============================================================

#ifndef BUILD_LENS_HOTSPOTANALYZER_H_
#define BUILD_LENS_HOTSPOTANALYZER_H_

#include <QString>

#include <vector>

#include "core/TraceGraph.h"

// 单个热点项。
struct HotspotItem {
  // 聚合维度的值，例如事件名 "ParseAST" 或类别 "parse"。
  QString key;

  // 该维度下所有事件的累计独占耗时，单位微秒。
  double duration_us = 0.0;

  // 该维度下的事件发生次数（出现在多少个文件中）。
  int occurrence_count = 0;

  // 占总耗时百分比（0-100）。
  double percentage = 0.0;
};

// 热点分析结果。
struct HotspotResult {
  // 按耗时降序排列的热点列表（Top-N）。
  std::vector<HotspotItem> hotspots;

  // 所有被统计事件的总耗时，单位微秒。
  double grand_total_duration_us = 0.0;

  // 当前结果使用的耗时口径。
  QString metric = QStringLiteral("exclusiveDurationUs");
};

// 热点分析器。
//
// 支持按事件名称（name）或类别（category）聚合。
// 对多个文件的事件列表做跨文件统计：
//   Parse 类事件总耗时 8000ms 占 42%，CodeGen 类事件 6500ms 占 34%...
//
// 用法：
//   std::vector<FileTraceResult> all_files = TraceParser::ParseDirectoryEvents(...);
//   HotspotResult result = HotspotAnalyzer::Analyze(all_files,
//                                    HotspotAnalyzer::Dimension::kCategory);
class HotspotAnalyzer {
 public:
  // 聚合维度。
  enum class Dimension {
    kByName,     // 按事件名称聚合（如 "ParseAST", "CodeGenFunction"）
    kByCategory  // 按事件类别聚合（如 "parse", "codegen", "inst"）
  };

  // 分析多文件调用图的热点。
  // graphs：TraceGraphBuilder 输出的调用图。
  // dim：聚合维度。
  // top_n：返回前 N 项热点（默认 10）。
  static HotspotResult Analyze(
      const std::vector<CompileGraph>& graphs,
      Dimension dim,
      int top_n = 10);

  // 专门按 Source 事件 detail 聚合头文件/源文件热点。
  static HotspotResult AnalyzeSourceHotspots(
      const std::vector<CompileGraph>& graphs,
      int top_n = 10);
};

#endif  // BUILD_LENS_HOTSPOTANALYZER_H_
