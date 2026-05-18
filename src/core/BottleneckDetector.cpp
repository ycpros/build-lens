// ============================================================
// BuildLens — C++ 编译性能观测工具
//
// BottleneckDetector.cpp
// 功能：BottleneckDetector 实现，均值+σ 统计异常检测。
// ============================================================

#include "core/BottleneckDetector.h"
#include "core/TraceRecord.h"

#include <algorithm>
#include <cmath>

// static
BottleneckResult BottleneckDetector::Detect(
    const std::vector<TraceRecord>& records,
    double threshold_sigma) {
  BottleneckResult result;
  result.threshold_multiplier = threshold_sigma;

  if (records.empty()) {
    return result;
  }

  // 提取所有文件的耗时列表（毫秒）。
  std::vector<double> durations;
  durations.reserve(records.size());
  for (const TraceRecord& rec : records) {
    durations.push_back(rec.total_duration_ms);
  }

  // ── 计算均值 ──
  double sum = 0.0;
  for (double d : durations) {
    sum += d;
  }
  double mean = sum / static_cast<double>(durations.size());
  result.mean_ms = mean;

  // ── 计算标准差 ──
  // σ = sqrt( Σ(x_i - μ)² / N )
  double variance_sum = 0.0;
  for (double d : durations) {
    double diff = d - mean;
    variance_sum += diff * diff;
  }
  double stddev = std::sqrt(variance_sum / static_cast<double>(durations.size()));
  result.stddev_ms = stddev;
  result.threshold_ms = mean + threshold_sigma * stddev;

  // 如果标准差为 0（所有文件耗时相同），阈值无效。
  if (stddev < 0.001) {
    return result;
  }

  // ── 检测异常 ──
  for (const TraceRecord& rec : records) {
    if (rec.total_duration_ms > result.threshold_ms) {
      BottleneckItem item;
      item.name = rec.filename;
      item.duration_ms = rec.total_duration_ms;
      item.anomaly_score =
          (rec.total_duration_ms - mean) / stddev;
      result.bottlenecks.push_back(item);
    }
  }

  // 按异常分数降序排列。
  std::sort(result.bottlenecks.begin(), result.bottlenecks.end(),
            [](const BottleneckItem& a, const BottleneckItem& b) {
              return a.anomaly_score > b.anomaly_score;
            });

  return result;
}
