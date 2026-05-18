// ============================================================
// BuildLens — C++ 编译性能观测工具
//
// CriticalPathAnalyzer.h
// 功能：在 CompileGraph 上找到最大 inclusive 耗时链。
//       输出关键路径上的事件列表及每个节点占总时间的百分比。
// ============================================================

#ifndef BUILD_LENS_CRITICALPATHANALYZER_H_
#define BUILD_LENS_CRITICALPATHANALYZER_H_

#include <QString>

#include <vector>

#include "core/TraceGraph.h"

// 关键路径上单个节点的描述。
struct CriticalPathItem {
  // 事件名称。
  QString name;

  // 事件细节，例如 Source 事件中的头文件路径。
  QString detail;

  // 该节点的包含耗时，单位微秒。
  double inclusive_duration_us = 0.0;

  // 该节点扣除直接子节点后的独占耗时，单位微秒。
  double exclusive_duration_us = 0.0;

  // inclusive_duration_us 占根节点墙钟耗时的百分比（0-100）。
  double percentage_of_root = 0.0;

  // 合并层级数（连续同名节点合并后的层数，1 = 未合并）。
  int merged_depth = 1;
};

// 关键路径分析结果。
struct CriticalPathResult {
  // 关键路径所属源文件名，例如 "renderer.cpp"。
  QString source_file;

  // 关键路径所属 trace JSON 文件路径。
  QString source_path;

  // 关键路径上的事件列表，从根到叶，按嵌套深度排列。
  std::vector<CriticalPathItem> path;

  // 关键路径总耗时，单位微秒。
  double total_duration_us = 0.0;
};

// 关键路径分析器。
//
// 算法：选择 inclusive 耗时最大的根节点，然后逐层选择 inclusive 耗时
// 最大的子节点。父子耗时不相加，total_duration_us 始终等于根节点耗时。
//
// 用法：
//   CompileGraph graph = TraceGraphBuilder::Build(...);
//   CriticalPathResult result = CriticalPathAnalyzer::Analyze(graph);
//   → result.path 包含从 ExecuteCompiler 到最深叶子的事件链。
class CriticalPathAnalyzer {
 public:
  // 分析调用图的关键路径。
  // graph：已完成嵌套构建的 CompileGraph。
  static CriticalPathResult Analyze(const CompileGraph& graph);

 private:
  static std::vector<CriticalPathItem> BuildPath(const TraceNode& root);
};

#endif  // BUILD_LENS_CRITICALPATHANALYZER_H_
