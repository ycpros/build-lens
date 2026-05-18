// ============================================================
// BuildLens — C++ 编译性能观测工具
//
// HotspotAnalyzer.cpp
// 功能：HotspotAnalyzer 实现，按维度聚合事件耗时并排序。
// ============================================================

#include "core/HotspotAnalyzer.h"
#include "core/TraceParser.h"  // for FileTraceResult

#include <algorithm>
#include <map>

// static
HotspotResult HotspotAnalyzer::Analyze(
    const std::vector<FileTraceResult>& files,
    Dimension dim,
    int top_n) {
  HotspotResult result;

  // 使用 map 按 key 聚合累计耗时和出现次数。
  // key = 事件名或类别；value = {total_us, count}。
  std::map<QString, std::pair<double, int>> aggregator;

  double grand_total = 0.0;

  for (const FileTraceResult& file : files) {
    for (const TraceEvent& ev : file.events) {
      // 跳过没有耗时的占位事件。
      if (ev.dur <= 0.0) continue;

      // 确定聚合键。
      QString key = (dim == Dimension::kByName) ? ev.name : ev.category;
      if (key.isEmpty()) continue;

      aggregator[key].first += ev.dur;
      aggregator[key].second += 1;
      grand_total += ev.dur;
    }
  }

  result.grand_total_us = grand_total;

  // 转到 vector 排序。
  for (const auto& pair : aggregator) {
    HotspotItem item;
    item.key = pair.first;
    item.total_duration_us = pair.second.first;
    item.occurrence_count = pair.second.second;
    item.percentage = (grand_total > 0.0)
                          ? (pair.second.first / grand_total * 100.0)
                          : 0.0;
    result.hotspots.push_back(item);
  }

  // 按累计耗时降序排列。
  std::sort(result.hotspots.begin(), result.hotspots.end(),
            [](const HotspotItem& a, const HotspotItem& b) {
              return a.total_duration_us > b.total_duration_us;
            });

  // 截取 Top-N。
  if (static_cast<int>(result.hotspots.size()) > top_n) {
    result.hotspots.resize(static_cast<size_t>(top_n));
  }

  return result;
}
