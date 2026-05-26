#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTemporaryDir>
#include <QString>

#include <cmath>
#include <iostream>
#include <vector>

#include "core/AnalysisPipeline.h"
#include "core/BuildLensVersion.h"
#include "core/CriticalPathAnalyzer.h"
#include "core/HotspotAnalyzer.h"
#include "core/TraceGraphBuilder.h"
#include "core/TraceParser.h"

namespace {

bool failed = false;

void Check(bool condition, const char* message) {
  if (!condition) {
    failed = true;
    std::cerr << "FAIL: " << message << "\n";
  }
}

bool NearlyEqual(double a, double b, double epsilon = 0.001) {
  return std::abs(a - b) <= epsilon;
}

QString FixtureDir() {
#ifdef BUILDLENS_SOURCE_DIR
  return QDir(QString::fromUtf8(BUILDLENS_SOURCE_DIR))
      .filePath(QStringLiteral("resources/BuildLensSample/src"));
#else
  return QDir::current().filePath(QStringLiteral("../resources/BuildLensSample/src"));
#endif
}

TraceEvent Event(QString name,
                 double ts,
                 double dur,
                 QString category = {},
                 QString detail = {}) {
  TraceEvent ev;
  ev.name = std::move(name);
  ev.category = std::move(category);
  ev.detail = std::move(detail);
  ev.phase = QStringLiteral("X");
  ev.ts = ts;
  ev.dur = dur;
  ev.pid = 1;
  ev.tid = 1;
  ev.is_summary = ev.name.startsWith(QStringLiteral("Total "));
  return ev;
}

QString WriteTrace(QTemporaryDir& dir,
                   const QString& filename,
                   const QString& json) {
  const QString path = dir.path() + QStringLiteral("/") + filename;
  QFile file(path);
  Check(file.open(QIODevice::WriteOnly | QIODevice::Text),
        "open temporary trace");
  file.write(json.toUtf8());
  file.close();
  return path;
}

QString WriteTrace(QTemporaryDir& dir, const QString& json) {
  return WriteTrace(dir, QStringLiteral("sample.cpp.json"), json);
}

void TestParserSummaryDetailAndBeginEndStack() {
  QTemporaryDir dir;
  const QString path = WriteTrace(dir, R"JSON(
{
  "traceEvents": [
    {"ph":"X","name":"Total ExecuteCompiler","ts":0,"dur":1000,"pid":1,"tid":2},
    {"ph":"X","name":"ParseClass","ts":1,"dur":10,"pid":1,"tid":1,
     "args":{"detail":"C:/tmp/wrapper.h:12:3 <Spelling=D:\\inc\\real.h:7:9>"}},
    {"ph":"b","name":"Source","id":0,"ts":0,"pid":1,"tid":1,
     "args":{"detail":"D:\\inc\\outer.h:1:1"}},
    {"ph":"b","name":"Source","id":0,"ts":10,"pid":1,"tid":1,
     "args":{"detail":"D:\\inc\\inner.h:2:1"}},
    {"ph":"e","name":"Source","id":0,"ts":20,"pid":1,"tid":1},
    {"ph":"e","name":"Source","id":0,"ts":40,"pid":1,"tid":1},
    {"ph":"b","name":"Source","id":0,"ts":5,"pid":2,"tid":1,
     "args":{"detail":"D:\\inc\\other_pid.h:3:1"}},
    {"ph":"e","name":"Source","id":0,"ts":15,"pid":2,"tid":1}
  ]
}
)JSON");

  FileTraceResult parsed = TraceParser::ParseFileEvents(path);
  Check(parsed.events.size() == 5, "parser emits summary, X, and three b/e events");

  bool saw_summary = false;
  bool saw_spelling = false;
  bool saw_inner = false;
  bool saw_outer = false;
  bool saw_other_pid = false;
  for (const TraceEvent& ev : parsed.events) {
    if (ev.name == QStringLiteral("Total ExecuteCompiler")) {
      saw_summary = ev.is_summary;
    }
    if (ev.name == QStringLiteral("ParseClass")) {
      saw_spelling = (ev.detail == QStringLiteral("D:/inc/real.h"));
    }
    if (ev.detail == QStringLiteral("D:/inc/inner.h")) {
      saw_inner = NearlyEqual(ev.dur, 10.0);
    }
    if (ev.detail == QStringLiteral("D:/inc/outer.h")) {
      saw_outer = NearlyEqual(ev.dur, 40.0);
    }
    if (ev.detail == QStringLiteral("D:/inc/other_pid.h")) {
      saw_other_pid = NearlyEqual(ev.dur, 10.0);
    }
  }

  Check(saw_summary, "Total event is marked summary");
  Check(saw_spelling, "Spelling detail is normalized");
  Check(saw_inner, "same-key nested b/e uses LIFO for inner event");
  Check(saw_outer, "same-key nested b/e uses LIFO for outer event");
  Check(saw_other_pid, "same id across pid does not collide");
}

void TestGraphStrictContainmentAndExclusive() {
  std::vector<TraceEvent> events = {
      Event(QStringLiteral("A"), 0, 100, QStringLiteral("driver")),
      Event(QStringLiteral("B"), 50, 100, QStringLiteral("parse")),
      Event(QStringLiteral("C"), 70, 20, QStringLiteral("parse")),
  };

  CompileGraph graph = TraceGraphBuilder::Build(QStringLiteral("x.json"), events);
  Check(graph.roots.size() == 2, "partial overlap creates separate root");
  Check(graph.roots[0].name == QStringLiteral("A"), "first root is A");
  Check(graph.roots[0].children.empty(), "partial overlap B is not child of A");
  Check(graph.roots[1].name == QStringLiteral("B"), "second root is B");
  Check(graph.roots[1].children.size() == 1, "C is child of B");
  Check(NearlyEqual(graph.roots[1].exclusive_duration_us, 80.0),
        "exclusive duration subtracts direct children");
}

void TestCriticalPathSemanticsAndTieBreak() {
  std::vector<TraceEvent> events = {
      Event(QStringLiteral("Root"), 0, 100, QStringLiteral("driver")),
      Event(QStringLiteral("B"), 0, 50, QStringLiteral("parse")),
      Event(QStringLiteral("C"), 50, 50, QStringLiteral("parse")),
      Event(QStringLiteral("D"), 55, 20, QStringLiteral("parse")),
  };

  CompileGraph graph = TraceGraphBuilder::Build(QStringLiteral("x.json"), events);
  CriticalPathResult cp = CriticalPathAnalyzer::Analyze(graph);

  Check(NearlyEqual(cp.total_duration_us, 100.0),
        "critical path total equals root wall-clock duration");
  Check(cp.path.size() == 2, "critical path follows one child branch");
  Check(cp.path[1].name == QStringLiteral("B"),
        "tie chooses greater exclusive duration before start/name");
  Check(NearlyEqual(cp.path[1].percentage_of_root, 50.0),
        "critical path percentage uses inclusive/root");
}

void TestHotspotsAndSourceHotspots() {
  std::vector<TraceEvent> events = {
      Event(QStringLiteral("ExecuteCompiler"), 0, 100, QStringLiteral("driver")),
      Event(QStringLiteral("Total ExecuteCompiler"), 0, 100, QStringLiteral("driver")),
      Event(QStringLiteral("Source"), 0, 40, QStringLiteral("source"),
            QStringLiteral("D:/inc/a.h")),
      Event(QStringLiteral("ParseClass"), 40, 30, QStringLiteral("parse")),
  };

  CompileGraph graph = TraceGraphBuilder::Build(QStringLiteral("x.json"), events);
  std::vector<CompileGraph> graphs;
  graphs.push_back(std::move(graph));

  HotspotResult by_name =
      HotspotAnalyzer::Analyze(graphs, HotspotAnalyzer::Dimension::kByName, 10);
  bool saw_total = false;
  for (const HotspotItem& item : by_name.hotspots) {
    if (item.key.startsWith(QStringLiteral("Total "))) {
      saw_total = true;
    }
  }
  Check(!saw_total, "summary Total events do not enter hotspots");

  HotspotResult source = HotspotAnalyzer::AnalyzeSourceHotspots(graphs, 10);
  Check(!source.hotspots.empty(), "source hotspots are emitted");
  Check(source.hotspots[0].key == QStringLiteral("D:/inc/a.h"),
        "source hotspot key is detail path");
  Check(NearlyEqual(source.hotspots[0].duration_us, 40.0),
        "source hotspot uses exclusive duration");
}

void TestAnalysisPipelineSingleFileAndOptions() {
  QTemporaryDir dir;
  const QString path = WriteTrace(dir, R"JSON(
{
  "traceEvents": [
    {"ph":"X","name":"ExecuteCompiler","ts":0,"dur":100000,"pid":1,"tid":1},
    {"ph":"X","name":"Source","ts":0,"dur":30000,"pid":1,"tid":1,
     "args":{"detail":"D:\\inc\\a.h:1:1"}},
    {"ph":"X","name":"ParseClass","ts":30000,"dur":20000,"pid":1,"tid":1,
     "args":{"detail":"Widget"}}
  ]
}
)JSON");

  AnalysisOptions options;
  options.threshold_sigma = 1.5;
  options.top_n = 1;
  AnalysisRunResult run = AnalysisPipeline::Run(path, options);

  Check(run.has_data, "pipeline single-file input has data");
  Check(run.report.total_file_count == 1,
        "pipeline single-file report has one file");
  Check(run.report.files.size() == 1,
        "pipeline single-file report has one file summary");
  Check(NearlyEqual(run.report.files[0].total_duration_ms, 100.0),
        "pipeline single-file summary uses ExecuteCompiler duration");
  Check(run.report.hotspots_by_name.hotspots.size() <= 1,
        "pipeline top_n limits hotspots by name");
  Check(run.report.hotspots_by_category.hotspots.size() <= 1,
        "pipeline top_n limits hotspots by category");
  Check(run.report.source_hotspots.hotspots.size() <= 1,
        "pipeline top_n limits source hotspots");
  Check(NearlyEqual(run.report.bottlenecks.threshold_multiplier, 1.5),
        "pipeline passes threshold sigma to bottleneck detector");
  Check(run.report.critical_path.source_file == QStringLiteral("sample.cpp"),
        "pipeline records critical path source file");
  Check(run.report.critical_path.source_path == path,
        "pipeline records critical path source path");

  QJsonObject critical_path_json =
      run.report.ToJsonDocument()
          .object()
          .value("criticalPath")
          .toObject();
  Check(critical_path_json.value("sourceFile").toString() ==
            QStringLiteral("sample.cpp"),
        "critical path JSON has sourceFile");
  Check(critical_path_json.value("sourcePath").toString() == path,
        "critical path JSON has sourcePath");
}

void TestAnalysisPipelineDirectoryInput() {
  QTemporaryDir dir;
  WriteTrace(dir, QStringLiteral("fast.cpp.json"), R"JSON(
{
  "traceEvents": [
    {"ph":"X","name":"ExecuteCompiler","ts":0,"dur":100000,"pid":1,"tid":1},
    {"ph":"X","name":"Source","ts":0,"dur":40000,"pid":1,"tid":1,
     "args":{"detail":"D:\\inc\\fast.h:1:1"}}
  ]
}
)JSON");
  WriteTrace(dir, QStringLiteral("slow.cpp.json"), R"JSON(
{
  "traceEvents": [
    {"ph":"X","name":"ExecuteCompiler","ts":0,"dur":200000,"pid":1,"tid":1},
    {"ph":"X","name":"ParseClass","ts":0,"dur":80000,"pid":1,"tid":1,
     "args":{"detail":"SlowType"}}
  ]
}
)JSON");

  AnalysisRunResult run = AnalysisPipeline::Run(dir.path());

  Check(run.has_data, "pipeline directory input has data");
  Check(run.report.total_file_count == 2,
        "pipeline directory report has two files");
  Check(run.report.files.size() == 2,
        "pipeline directory report has two file summaries");
  Check(run.report.files[0].filename == QStringLiteral("slow.cpp"),
        "pipeline file summaries sort by duration descending");
  Check(NearlyEqual(run.report.total_build_time_s, 0.3),
        "pipeline directory report sums build time");
  Check(run.report.critical_path.source_file == QStringLiteral("slow.cpp"),
        "pipeline directory critical path comes from slowest file");
}

void TestAnalysisPipelineEmptyAndInvalidInput() {
  QTemporaryDir empty_dir;
  AnalysisRunResult empty_run = AnalysisPipeline::Run(empty_dir.path());
  Check(!empty_run.has_data, "pipeline empty directory has no data");
  Check(!empty_run.warnings.isEmpty(), "pipeline empty directory emits warning");

  QTemporaryDir invalid_dir;
  const QString invalid_path =
      WriteTrace(invalid_dir, QStringLiteral("invalid.cpp.json"),
                 QStringLiteral("not valid json"));
  AnalysisRunResult invalid_run = AnalysisPipeline::Run(invalid_path);
  Check(!invalid_run.has_data, "pipeline invalid trace has no data");
  Check(!invalid_run.warnings.isEmpty(), "pipeline invalid trace emits warning");
}

void TestRealBuildLensSampleFixture() {
  const QString fixture_dir = FixtureDir();
  Check(QDir(fixture_dir).exists(), "real BuildLensSample fixture exists");

  std::vector<FileTraceResult> files =
      TraceParser::ParseDirectoryEvents(fixture_dir);

  Check(files.size() == 9, "real fixture parses 9 source trace files");

  bool saw_source = false;
  bool saw_summary = false;
  bool saw_detail = false;
  for (const FileTraceResult& file : files) {
    for (const TraceEvent& ev : file.events) {
      if (ev.name == QStringLiteral("Source")) {
        saw_source = true;
      }
      if (ev.is_summary) {
        saw_summary = true;
      }
      if (!ev.detail.isEmpty()) {
        saw_detail = true;
      }
    }
  }

  Check(saw_source, "real fixture contains normalized Source b/e events");
  Check(saw_summary, "real fixture contains Total summary events");
  Check(saw_detail, "real fixture contains args.detail values");

  AnalysisRunResult run = AnalysisPipeline::Run(fixture_dir);
  Check(run.has_data, "real fixture pipeline has data");
  Check(run.report.total_file_count == 9,
        "real fixture pipeline reports 9 files");

  Check(!run.report.critical_path.path.empty(),
        "real fixture critical path exists");
  Check(run.report.critical_path.total_duration_us > 0.0,
        "real fixture critical path has wall-clock duration");
  Check(!run.report.critical_path.source_file.isEmpty(),
        "real fixture critical path has source file");
  Check(!run.report.critical_path.source_path.isEmpty(),
        "real fixture critical path has source path");
  Check(!run.report.hotspots_by_category.hotspots.empty(),
        "real fixture category hotspots exist");
  Check(!run.report.source_hotspots.hotspots.empty(),
        "real fixture source hotspots exist");
  Check(run.report.source_hotspots.hotspots[0].key != QStringLiteral("Source"),
        "real fixture source hotspot key is a detail path");

  bool saw_total_hotspot = false;
  for (const HotspotItem& item : run.report.hotspots_by_name.hotspots) {
    if (item.key.startsWith(QStringLiteral("Total "))) {
      saw_total_hotspot = true;
    }
  }
  Check(!saw_total_hotspot, "real fixture excludes Total events from hotspots");

  QJsonObject root = run.report.ToJsonDocument().object();
  Check(root.value("version").toString() ==
            QString::fromLatin1(buildlens::kReportSchemaVersion),
        "report JSON version matches centralized schema version");
  Check(root.contains("sourceHotspots"), "report JSON has sourceHotspots");
  Check(root.value("bottlenecks").toObject().contains("thresholdMs"),
        "report JSON has bottlenecks.thresholdMs");
  Check(root.value("criticalPath").toObject().contains("sourceFile"),
        "report JSON has criticalPath.sourceFile");
  Check(root.value("criticalPath").toObject().contains("sourcePath"),
        "report JSON has criticalPath.sourcePath");

  QJsonObject first_path_item =
      root.value("criticalPath")
          .toObject()
          .value("path")
          .toArray()
          .first()
          .toObject();
  Check(first_path_item.contains("inclusiveDurationUs"),
        "critical path JSON has inclusiveDurationUs");
  Check(first_path_item.contains("exclusiveDurationUs"),
        "critical path JSON has exclusiveDurationUs");
  Check(first_path_item.contains("percentageOfRoot"),
        "critical path JSON has percentageOfRoot");
  Check(root.value("hotspotsByCategory").toObject().value("metric").toString() ==
            QStringLiteral("exclusiveDurationUs"),
        "hotspot metric is exclusiveDurationUs");
}

}  // namespace

int main(int argc, char* argv[]) {
  QCoreApplication app(argc, argv);

  TestParserSummaryDetailAndBeginEndStack();
  TestGraphStrictContainmentAndExclusive();
  TestCriticalPathSemanticsAndTieBreak();
  TestHotspotsAndSourceHotspots();
  TestAnalysisPipelineSingleFileAndOptions();
  TestAnalysisPipelineDirectoryInput();
  TestAnalysisPipelineEmptyAndInvalidInput();
  TestRealBuildLensSampleFixture();

  if (failed) {
    return 1;
  }

  std::cout << "All BuildLens core tests passed.\n";
  return 0;
}
