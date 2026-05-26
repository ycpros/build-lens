// ============================================================
// BuildLens — C++ 编译性能观测工具
//
// FileSummary.h
// 功能：定义单个 trace 文件的分析摘要。
// ============================================================

#ifndef BUILD_LENS_FILESUMMARY_H_
#define BUILD_LENS_FILESUMMARY_H_

#include <QString>

// 单个文件的分析摘要（用于报告、瓶颈检测和文件列表视图）。
struct FileSummary {
  QString filename;
  QString source_path;
  double total_duration_ms = 0.0;
  int event_count = 0;
};

#endif  // BUILD_LENS_FILESUMMARY_H_
