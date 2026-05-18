// ============================================================
// BuildLens — C++ 编译性能观测工具
//
// AnalysisPipeline.h
// 功能：统一编排 trace 解析、调用图构建、分析器执行与报告生成。
// ============================================================

#ifndef BUILD_LENS_ANALYSISPIPELINE_H_
#define BUILD_LENS_ANALYSISPIPELINE_H_

#include <QString>
#include <QStringList>

#include "core/AnalysisReport.h"

// 一次分析运行的可调参数。
struct AnalysisOptions {
  // 瓶颈检测阈值倍率，默认 mean + 2σ。
  double threshold_sigma = 2.0;

  // 热点结果返回前 N 项。
  int top_n = 10;
};

// 一次分析运行的完整结果。
struct AnalysisRunResult {
  AnalysisReport report;
  QStringList warnings;
  bool has_data = false;
};

// 统一分析管线。
class AnalysisPipeline {
 public:
  // 对目录或单个 .json 文件执行完整分析。
  static AnalysisRunResult Run(
      const QString& input_path,
      const AnalysisOptions& options = AnalysisOptions());
};

#endif  // BUILD_LENS_ANALYSISPIPELINE_H_
