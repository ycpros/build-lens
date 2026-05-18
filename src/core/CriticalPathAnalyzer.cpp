// ============================================================
// BuildLens — C++ 编译性能观测工具
//
// CriticalPathAnalyzer.cpp
// 功能：CriticalPathAnalyzer 实现，DFS 查找最长耗时路径。
// ============================================================

#include "core/CriticalPathAnalyzer.h"

#include <algorithm>

namespace {

// 合并路径中连续的同名节点。
// 例如 "DebugType → DebugType → DebugType" → "DebugType (×3)"
// 保留最后一个（最深）节点的 duration_us 和 percentage，
// merged_depth 记录合并层数。
void CollapseDuplicates(std::vector<CriticalPathItem>& path) {
  if (path.size() < 2) return;

  std::vector<CriticalPathItem> collapsed;
  collapsed.push_back(path[0]);

  for (size_t i = 1; i < path.size(); ++i) {
    if (path[i].name == collapsed.back().name) {
      // 同名：合并到前一项。
      collapsed.back().duration_us = path[i].duration_us;
      collapsed.back().percentage = path[i].percentage;
      collapsed.back().merged_depth++;
    } else {
      collapsed.push_back(path[i]);
    }
  }

  path = std::move(collapsed);
}

}  // namespace

// static
CriticalPathResult CriticalPathAnalyzer::Analyze(const CompileGraph& graph) {
  CriticalPathResult best_result;

  if (graph.roots.empty()) {
    return best_result;
  }

  // 遍历所有根节点（多线程时有多个），取关键路径最长者。
  for (const TraceNode& root : graph.roots) {
    auto [path, total_dur] = FindLongestPath(root);
    if (total_dur > best_result.total_duration_us) {
      best_result.path = std::move(path);
      best_result.total_duration_us = total_dur;
    }
  }

  // 合并连续同名节点（如嵌套 DebugType 链）。
  CollapseDuplicates(best_result.path);

  // 计算每条路径节点占根节点的百分比。
  if (!best_result.path.empty()) {
    double root_duration = best_result.path[0].duration_us;
    if (root_duration > 0.0) {
      for (auto& item : best_result.path) {
        item.percentage = (item.duration_us / root_duration) * 100.0;
      }
    }
  }

  return best_result;
}

// static
std::pair<std::vector<CriticalPathItem>, double>
CriticalPathAnalyzer::FindLongestPath(const TraceNode& node) {
  // 叶子节点：路径仅包含自身。
  if (node.children.empty()) {
    CriticalPathItem item;
    item.name = node.name;
    item.duration_us = node.duration_us;
    return {{item}, node.duration_us};
  }

  // 有子节点：对每个子节点递归，取路径最长的一个。
  std::vector<CriticalPathItem> best_sub_path;
  double best_sub_duration = 0.0;

  for (const TraceNode& child : node.children) {
    auto [sub_path, sub_dur] = FindLongestPath(child);
    if (sub_dur > best_sub_duration) {
      best_sub_duration = sub_dur;
      best_sub_path = std::move(sub_path);
    }
  }

  // 在当前节点前添加自身，构成完整路径。
  CriticalPathItem self_item;
  self_item.name = node.name;
  self_item.duration_us = node.duration_us + best_sub_duration;

  std::vector<CriticalPathItem> full_path;
  full_path.push_back(self_item);
  for (auto& item : best_sub_path) {
    full_path.push_back(std::move(item));
  }

  return {full_path, node.duration_us + best_sub_duration};
}
