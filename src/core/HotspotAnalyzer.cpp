// ============================================================
// BuildLens — C++ 编译性能观测工具
//
// HotspotAnalyzer.cpp
// 功能：HotspotAnalyzer 实现，按维度聚合事件耗时并排序。
// ============================================================

#include "core/HotspotAnalyzer.h"

#include <algorithm>
#include <functional>
#include <map>

namespace {

using Aggregator = std::map<QString, std::pair<double, int>>;

void WalkNodes(const TraceNode& node,
               const std::function<void(const TraceNode&)>& visitor) {
  visitor(node);
  for (const TraceNode& child : node.children) {
    WalkNodes(child, visitor);
  }
}

HotspotResult BuildResult(const Aggregator& aggregator,
                          double grand_total,
                          int top_n) {
  HotspotResult result;
  result.grand_total_duration_us = grand_total;

  for (const auto& pair : aggregator) {
    HotspotItem item;
    item.key = pair.first;
    item.duration_us = pair.second.first;
    item.occurrence_count = pair.second.second;
    item.percentage = (grand_total > 0.0)
                          ? (pair.second.first / grand_total * 100.0)
                          : 0.0;
    result.hotspots.push_back(item);
  }

  std::sort(result.hotspots.begin(), result.hotspots.end(),
            [](const HotspotItem& a, const HotspotItem& b) {
              if (a.duration_us != b.duration_us) {
                return a.duration_us > b.duration_us;
              }
              return a.key.localeAwareCompare(b.key) < 0;
            });

  if (static_cast<int>(result.hotspots.size()) > top_n) {
    result.hotspots.resize(static_cast<size_t>(top_n));
  }

  return result;
}

}  // namespace

// static
HotspotResult HotspotAnalyzer::Analyze(
    const std::vector<CompileGraph>& graphs,
    Dimension dim,
    int top_n) {
  Aggregator aggregator;
  double grand_total = 0.0;

  for (const CompileGraph& graph : graphs) {
    for (const TraceNode& root : graph.roots) {
      WalkNodes(root, [&](const TraceNode& node) {
        if (node.exclusive_duration_us <= 0.0) return;
        QString key = (dim == Dimension::kByName) ? node.name : node.category;
        if (key.isEmpty()) return;

        aggregator[key].first += node.exclusive_duration_us;
        aggregator[key].second += 1;
        grand_total += node.exclusive_duration_us;
      });
    }
  }

  return BuildResult(aggregator, grand_total, top_n);
}

// static
HotspotResult HotspotAnalyzer::AnalyzeSourceHotspots(
    const std::vector<CompileGraph>& graphs,
    int top_n) {
  Aggregator aggregator;
  double grand_total = 0.0;

  for (const CompileGraph& graph : graphs) {
    for (const TraceNode& root : graph.roots) {
      WalkNodes(root, [&](const TraceNode& node) {
        if (node.name != QStringLiteral("Source")) return;
        if (node.detail.isEmpty() || node.exclusive_duration_us <= 0.0) return;

        aggregator[node.detail].first += node.exclusive_duration_us;
        aggregator[node.detail].second += 1;
        grand_total += node.exclusive_duration_us;
      });
    }
  }

  return BuildResult(aggregator, grand_total, top_n);
}
