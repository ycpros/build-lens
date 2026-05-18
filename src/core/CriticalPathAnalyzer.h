// ============================================================
// BuildLens — C++ 编译性能观测工具
//
// CriticalPathAnalyzer.h
// 功能：在 CompileGraph 上执行 DFS，找到从根到叶子的最长耗时路径。
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

  // 该节点及其子树的总耗时，单位微秒。
  double duration_us = 0.0;

  // 占根节点总耗时的百分比（0-100）。
  double percentage = 0.0;

  // 合并层级数（连续同名节点合并后的层数，1 = 未合并）。
  int merged_depth = 1;
};

// 关键路径分析结果。
struct CriticalPathResult {
  // 关键路径上的事件列表，从根到叶，按嵌套深度排列。
  std::vector<CriticalPathItem> path;

  // 关键路径总耗时，单位微秒。
  double total_duration_us = 0.0;
};

// 关键路径分析器。
//
// 算法：从 CompileGraph 的每个根节点开始，DFS 递归计算每条子路径的
// 总耗时，选取最长的一条。多线程时取所有根的关键路径中总耗时最长者。
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
  // 递归查找以 node 为起点的最长子树路径。
  // 返回 {从 node 到最深叶子的路径, 该路径的总耗时}。
  static std::pair<std::vector<CriticalPathItem>, double> FindLongestPath(
      const TraceNode& node);
};

#endif  // BUILD_LENS_CRITICALPATHANALYZER_H_
