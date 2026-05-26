// ============================================================
// BuildLens — C++ 编译性能观测工具
//
// AnalysisReport.h
// 功能：定义统一的分析报告结构，包含三个分析器的输出。
//      负责将分析结果序列化为 JSON 格式。
//      同样支持 CLI JSON 输出和 Web UI 加载。
// ============================================================

#ifndef BUILD_LENS_ANALYSISREPORT_H_
#define BUILD_LENS_ANALYSISREPORT_H_

#include <QJsonDocument>
#include <QJsonObject>
#include <QString>

#include <vector>

#include "core/BottleneckDetector.h"
#include "core/CriticalPathAnalyzer.h"
#include "core/FileSummary.h"
#include "core/HotspotAnalyzer.h"

// 统一分析报告。
//
// 包含一次分析运行的所有输出：
//   - 文件摘要列表
//   - 关键路径分析
//   - 热点分析（按名称和按类别）
//   - 瓶颈检测
//
// 用法：
//   AnalysisReport report;
//   report.files = ...;
//   report.critical_path = CriticalPathAnalyzer::Analyze(...);
//   report.hotspots_by_name = HotspotAnalyzer::Analyze(...);
//   report.bottlenecks = BottleneckDetector::Detect(...);
//
//   QJsonDocument doc = report.ToJson();
//   → 写入文件或通过 HTTP 提供。
struct AnalysisReport {
  // ---- 文件摘要 ----
  std::vector<FileSummary> files;

  // ---- 分析器结果 ----
  CriticalPathResult critical_path;
  HotspotResult hotspots_by_name;
  HotspotResult hotspots_by_category;
  HotspotResult source_hotspots;
  BottleneckResult bottlenecks;

  // 分析包含的总文件数。
  int total_file_count = 0;

  // 所有文件的总编译耗时，单位秒。
  double total_build_time_s = 0.0;

  // ---- 序列化 ----

  // 将报告序列化为 QJsonDocument。
  QJsonDocument ToJsonDocument() const;

  // 将报告序列化为紧凑 JSON 字节数组。
  QByteArray ToJsonBytes() const;

  // 将报告序列化为格式化 JSON 字符串（用于输出到文件）。
  QString ToJsonString() const;

 private:
  // 辅助：将 FileSummary 列表序列化为 JSON 数组。
  QJsonArray FilesToJson() const;

  // 辅助：将 CriticalPathResult 序列化为 JSON 对象。
  QJsonObject CriticalPathToJson() const;

  // 辅助：将 HotspotResult 序列化为 JSON 对象。
  QJsonObject HotspotsToJson(const HotspotResult& hr) const;

  // 辅助：将 BottleneckResult 序列化为 JSON 对象。
  QJsonObject BottlenecksToJson() const;
};

#endif  // BUILD_LENS_ANALYSISREPORT_H_
