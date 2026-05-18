// ============================================================
// BuildLens — C++ 编译性能观测工具
//
// AnalysisPipeline.cpp
// 功能：AnalysisPipeline 实现，生成统一 AnalysisReport。
// ============================================================

#include "core/AnalysisPipeline.h"

#include <QFileInfo>

#include <algorithm>
#include <vector>

#include "core/BottleneckDetector.h"
#include "core/CriticalPathAnalyzer.h"
#include "core/HotspotAnalyzer.h"
#include "core/TraceGraphBuilder.h"
#include "core/TraceParser.h"
#include "core/TraceRecord.h"

namespace {

AnalysisOptions NormalizeOptions(AnalysisOptions options) {
  if (options.threshold_sigma <= 0.0) {
    options.threshold_sigma = 2.0;
  }
  if (options.top_n <= 0) {
    options.top_n = 10;
  }
  return options;
}

std::vector<FileTraceResult> ParseInputTraces(
    const QString& input_path,
    QStringList* warnings) {
  std::vector<FileTraceResult> file_results;
  const QFileInfo input_info(input_path);

  if (!input_info.exists()) {
    warnings->append(QStringLiteral("输入路径不存在：%1").arg(input_path));
    return file_results;
  }

  if (input_info.isDir()) {
    return TraceParser::ParseDirectoryEvents(input_path);
  }

  if (!input_info.isFile()) {
    warnings->append(QStringLiteral("输入路径不是目录或文件：%1").arg(input_path));
    return file_results;
  }

  FileTraceResult single = TraceParser::ParseFileEvents(input_path);
  if (!single.events.empty()) {
    file_results.push_back(std::move(single));
  }
  return file_results;
}

double ExtractTotalDurationUs(const std::vector<TraceEvent>& events) {
  // 优先使用真实时间线根事件。
  for (const TraceEvent& event : events) {
    if (event.name == QStringLiteral("ExecuteCompiler")) {
      return event.dur;
    }
  }

  // 回退到 Clang summary 事件。
  for (const TraceEvent& event : events) {
    if (event.name == QStringLiteral("Total ExecuteCompiler")) {
      return event.dur;
    }
  }

  // 兼容示例数据格式。
  for (const TraceEvent& event : events) {
    if (event.category == QStringLiteral("phase") &&
        event.name == QStringLiteral("Total")) {
      return event.dur;
    }
  }

  return 0.0;
}

std::vector<TraceRecord> BuildRecordsFromEvents(
    const std::vector<FileTraceResult>& file_results,
    QStringList* warnings) {
  std::vector<TraceRecord> records;
  records.reserve(file_results.size());

  for (const FileTraceResult& file : file_results) {
    const double total_duration_us = ExtractTotalDurationUs(file.events);
    if (total_duration_us <= 0.0) {
      warnings->append(QStringLiteral("未找到总耗时事件：%1")
                           .arg(file.source_path));
      continue;
    }

    TraceRecord record;
    record.filename = file.filename;
    record.source_path = file.source_path;
    record.total_duration_ms = total_duration_us / 1000.0;
    records.push_back(std::move(record));
  }

  return records;
}

const TraceRecord* FindRecordByPath(
    const std::vector<TraceRecord>& records,
    const QString& source_path) {
  for (const TraceRecord& record : records) {
    if (record.source_path == source_path) {
      return &record;
    }
  }
  return nullptr;
}

void SortFileSummaries(std::vector<FileSummary>* files) {
  std::sort(files->begin(), files->end(),
            [](const FileSummary& a, const FileSummary& b) {
              if (a.total_duration_ms != b.total_duration_ms) {
                return a.total_duration_ms > b.total_duration_ms;
              }
              return a.filename.localeAwareCompare(b.filename) < 0;
            });
}

}  // namespace

// static
AnalysisRunResult AnalysisPipeline::Run(
    const QString& input_path,
    const AnalysisOptions& options) {
  AnalysisRunResult result;
  const AnalysisOptions normalized_options = NormalizeOptions(options);

  std::vector<FileTraceResult> file_results =
      ParseInputTraces(input_path, &result.warnings);
  if (file_results.empty()) {
    result.warnings.append(
        QStringLiteral("未找到有效的 -ftime-trace JSON 文件。"));
    return result;
  }

  result.has_data = true;
  result.report.total_file_count = static_cast<int>(file_results.size());

  std::vector<TraceRecord> records =
      BuildRecordsFromEvents(file_results, &result.warnings);

  std::vector<CompileGraph> graphs;
  graphs.reserve(file_results.size());

  for (const FileTraceResult& file : file_results) {
    FileSummary summary;
    summary.filename = file.filename;
    summary.source_path = file.source_path;
    summary.event_count = static_cast<int>(file.events.size());

    const TraceRecord* record = FindRecordByPath(records, file.source_path);
    if (record != nullptr) {
      summary.total_duration_ms = record->total_duration_ms;
      result.report.total_build_time_s +=
          record->total_duration_ms / 1000.0;
    }
    result.report.files.push_back(std::move(summary));

    CompileGraph graph = TraceGraphBuilder::Build(file.source_path,
                                                  file.events);
    CriticalPathResult critical_path = CriticalPathAnalyzer::Analyze(graph);
    if (critical_path.total_duration_us >
        result.report.critical_path.total_duration_us) {
      critical_path.source_file = graph.source_name;
      critical_path.source_path = graph.source_path;
      result.report.critical_path = std::move(critical_path);
    }
    graphs.push_back(std::move(graph));
  }

  SortFileSummaries(&result.report.files);

  result.report.hotspots_by_name =
      HotspotAnalyzer::Analyze(graphs,
                               HotspotAnalyzer::Dimension::kByName,
                               normalized_options.top_n);
  result.report.hotspots_by_category =
      HotspotAnalyzer::Analyze(graphs,
                               HotspotAnalyzer::Dimension::kByCategory,
                               normalized_options.top_n);
  result.report.source_hotspots =
      HotspotAnalyzer::AnalyzeSourceHotspots(graphs, normalized_options.top_n);

  result.report.bottlenecks =
      BottleneckDetector::Detect(records, normalized_options.threshold_sigma);

  return result;
}
