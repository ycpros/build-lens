// ============================================================
// BuildLens — C++ 编译性能观测工具
//
// AnalysisReport.cpp
// 功能：AnalysisReport 的实现，将所有分析结果序列化为 JSON 格式。
//       输出的 JSON Schema 见 docs/BuildLens 架构与愿景.md 第五节。
// ============================================================

#include "core/AnalysisReport.h"

#include <QJsonArray>

// ====================================================================
// 公开接口
// ====================================================================

QJsonDocument AnalysisReport::ToJsonDocument() const {
  QJsonObject root;

  // 元信息。
  root["version"] = "0.2.0";
  root["totalFileCount"] = total_file_count;
  root["totalBuildTimeSeconds"] = total_build_time_s;

  // 文件列表。
  root["files"] = FilesToJson();

  // 分析器结果。
  root["criticalPath"] = CriticalPathToJson();
  root["hotspotsByName"] = HotspotsToJson(hotspots_by_name);
  root["hotspotsByCategory"] = HotspotsToJson(hotspots_by_category);
  root["bottlenecks"] = BottlenecksToJson();

  return QJsonDocument(root);
}

QByteArray AnalysisReport::ToJsonBytes() const {
  return ToJsonDocument().toJson(QJsonDocument::Compact);
}

QString AnalysisReport::ToJsonString() const {
  return QString::fromUtf8(
      ToJsonDocument().toJson(QJsonDocument::Indented));
}

// ====================================================================
// 私有辅助
// ====================================================================

QJsonArray AnalysisReport::FilesToJson() const {
  QJsonArray arr;
  for (const FileSummary& fs : files) {
    QJsonObject obj;
    obj["filename"] = fs.filename;
    obj["sourcePath"] = fs.source_path;
    obj["totalDurationMs"] = fs.total_duration_ms;
    obj["eventCount"] = fs.event_count;
    arr.append(obj);
  }
  return arr;
}

QJsonObject AnalysisReport::CriticalPathToJson() const {
  QJsonObject obj;
  QJsonArray path_arr;

  for (const CriticalPathItem& item : critical_path.path) {
    QJsonObject path_item;
    path_item["name"] = item.name;
    path_item["durationUs"] = item.duration_us;
    path_item["percentage"] = item.percentage;
    if (item.merged_depth > 1) {
      path_item["mergedDepth"] = item.merged_depth;
    }
    path_arr.append(path_item);
  }

  obj["path"] = path_arr;
  obj["totalDurationUs"] = critical_path.total_duration_us;
  return obj;
}

QJsonObject AnalysisReport::HotspotsToJson(
    const HotspotResult& hr) const {
  QJsonObject obj;
  QJsonArray arr;

  for (const HotspotItem& item : hr.hotspots) {
    QJsonObject hi;
    hi["key"] = item.key;
    hi["totalDurationUs"] = item.total_duration_us;
    hi["occurrenceCount"] = item.occurrence_count;
    hi["percentage"] = item.percentage;
    arr.append(hi);
  }

  obj["hotspots"] = arr;
  obj["grandTotalUs"] = hr.grand_total_us;
  return obj;
}

QJsonObject AnalysisReport::BottlenecksToJson() const {
  QJsonObject obj;
  QJsonArray arr;

  for (const BottleneckItem& item : bottlenecks.bottlenecks) {
    QJsonObject bi;
    bi["name"] = item.name;
    bi["durationMs"] = item.duration_ms;
    bi["anomalyScore"] = item.anomaly_score;
    arr.append(bi);
  }

  obj["bottlenecks"] = arr;
  obj["meanMs"] = bottlenecks.mean_ms;
  obj["stddevMs"] = bottlenecks.stddev_ms;
  obj["thresholdMultiplier"] = bottlenecks.threshold_multiplier;
  return obj;
}
