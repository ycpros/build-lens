// ============================================================
// BuildLens — C++ 编译性能观测工具
//
// BottleneckDetector.h
// 功能：对多个文件的编译耗时进行统计分析，使用均值 + Nσ 阈值
//       检测异常耗时文件，标记为"瓶颈"。
// ============================================================

#ifndef BUILD_LENS_BOTTLENECKDETECTOR_H_
#define BUILD_LENS_BOTTLENECKDETECTOR_H_

#include <QString>

#include <vector>

#include "core/FileSummary.h"

// 单个瓶颈项。
struct BottleneckItem {
  // 源文件名，例如 "big_header.cpp"。
  QString name;

  // 该文件的编译总耗时，单位毫秒。
  double duration_ms = 0.0;

  // 异常分数 = (duration - mean) / stddev。
  // 分数越高表示越偏离正常范围。
  double anomaly_score = 0.0;
};

// 瓶颈检测结果。
struct BottleneckResult {
  // 被标记为瓶颈的文件列表，按 anomaly_score 降序。
  std::vector<BottleneckItem> bottlenecks;

  // 所有文件的平均耗时，单位毫秒。
  double mean_ms = 0.0;

  // 所有文件的标准差，单位毫秒。
  double stddev_ms = 0.0;

  // 使用的阈值倍率（默认 2.0，即 mean + 2σ）。
  double threshold_multiplier = 2.0;

  // 实际瓶颈阈值，单位毫秒。
  double threshold_ms = 0.0;
};

// 瓶颈检测器。
//
// 算法：统计所有文件的编译耗时，计算均值 μ 和标准差 σ。
// 任何耗时超过 μ + Nσ 的文件被标记为瓶颈。
// N 默认为 2.0（约对应正态分布的前 2.3%）。
//
// 用法：
//   BottleneckResult result = BottleneckDetector::Detect(report.files);
//   → result.bottlenecks 包含异常慢的文件。
class BottleneckDetector {
 public:
  // 检测瓶颈文件。
  // files：AnalysisPipeline 输出的文件摘要列表（每个文件一条总耗时）。
  // threshold_sigma：阈值倍率，默认 2.0（均值 + 2σ）。
  static BottleneckResult Detect(const std::vector<FileSummary>& files,
                                 double threshold_sigma = 2.0);
};

#endif  // BUILD_LENS_BOTTLENECKDETECTOR_H_
