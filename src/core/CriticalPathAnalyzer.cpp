// ============================================================
// BuildLens — C++ 编译性能观测工具
//
// CriticalPathAnalyzer.cpp
// 功能：CriticalPathAnalyzer 实现，DFS 查找最长耗时路径。
// ============================================================

#include "core/CriticalPathAnalyzer.h"

#include <algorithm>

namespace {

bool IsBetterNode(const TraceNode& candidate, const TraceNode& current) {
  if (candidate.duration_us != current.duration_us) {
    return candidate.duration_us > current.duration_us;
  }
  if (candidate.exclusive_duration_us != current.exclusive_duration_us) {
    return candidate.exclusive_duration_us > current.exclusive_duration_us;
  }
  if (candidate.start_us != current.start_us) {
    return candidate.start_us < current.start_us;
  }
  return candidate.name.localeAwareCompare(current.name) < 0;
}

// 合并路径中连续的同名节点。
// 例如 "DebugType → DebugType → DebugType" → "DebugType (×3)"
// 保留最后一个（最深）节点的耗时与 detail，
// merged_depth 记录合并层数。
void CollapseDuplicates(std::vector<CriticalPathItem>& path) {
  if (path.size() < 2) return;

  std::vector<CriticalPathItem> collapsed;
  collapsed.push_back(path[0]);

  for (size_t i = 1; i < path.size(); ++i) {
    if (path[i].name == collapsed.back().name) {
      // 同名：合并到前一项。
      path[i].merged_depth += collapsed.back().merged_depth;
      collapsed.back() = path[i];
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

  // 遍历所有根节点（多线程时有多个），取 inclusive 耗时最长者。
  const TraceNode* best_root = &graph.roots.front();
  for (const TraceNode& root : graph.roots) {
    if (IsBetterNode(root, *best_root)) {
      best_root = &root;
    }
  }

  best_result.total_duration_us = best_root->duration_us;
  best_result.path = BuildPath(*best_root);

  // 合并连续同名节点（如嵌套 DebugType 链）。
  CollapseDuplicates(best_result.path);

  // 计算每条路径节点占根节点的百分比。
  if (best_result.total_duration_us > 0.0) {
    for (auto& item : best_result.path) {
      item.percentage_of_root =
          (item.inclusive_duration_us / best_result.total_duration_us) * 100.0;
    }
  }

  return best_result;
}

// static
std::vector<CriticalPathItem> CriticalPathAnalyzer::BuildPath(
    const TraceNode& root) {
  std::vector<CriticalPathItem> path;
  const TraceNode* current = &root;

  while (current != nullptr) {
    CriticalPathItem item;
    item.name = current->name;
    item.detail = current->detail;
    item.inclusive_duration_us = current->duration_us;
    item.exclusive_duration_us = current->exclusive_duration_us;
    path.push_back(item);

    const TraceNode* best_child = nullptr;
    for (const TraceNode& child : current->children) {
      if (best_child == nullptr || IsBetterNode(child, *best_child)) {
        best_child = &child;
      }
    }
    current = best_child;
  }

  return path;
}
