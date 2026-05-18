// ============================================================
// BuildLens — C++ 编译性能观测工具
//
// cli_main.cpp
// 功能：CLI 入口，解析命令行参数，调用分析管线并输出 JSON 报告。
//
// 用法：
//   buildlens-cli --input <dir_or_file.json> [--output report.json]
//
// 与桌面 UI（main.cpp）分离，减少 GUI 依赖的场景。
// ============================================================

#include <QCoreApplication>
#include <QCommandLineParser>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

#include "core/AnalysisReport.h"
#include "core/BottleneckDetector.h"
#include "core/CriticalPathAnalyzer.h"
#include "core/HotspotAnalyzer.h"
#include "core/TraceGraphBuilder.h"
#include "core/TraceParser.h"

int main(int argc, char* argv[]) {
  QCoreApplication app(argc, argv);
  QCoreApplication::setApplicationName("BuildLens CLI");
  QCoreApplication::setApplicationVersion("0.2.0");

  // Windows 控制台默认 GBK，SetConsoleOutputCP(CP_UTF8) 修复中文乱码。
#ifdef Q_OS_WIN
  SetConsoleOutputCP(CP_UTF8);
  SetConsoleCP(CP_UTF8);
#endif

  // ----- 命令行参数 -----
  QCommandLineParser parser;
  parser.setApplicationDescription(
      "BuildLens — C++ 编译性能分析工具 (CLI)\n"
      "解析 Clang -ftime-trace JSON 并输出结构化分析报告。");
  parser.addHelpOption();
  parser.addVersionOption();

  QCommandLineOption inputOption(
      QStringList() << "i" << "input",
      "输入路径：目录（递归扫描 .json）或单个 .json 文件",
      "path");
  parser.addOption(inputOption);

  QCommandLineOption outputOption(
      QStringList() << "o" << "output",
      "输出 JSON 文件路径（默认输出到 stdout）",
      "file");
  parser.addOption(outputOption);

  QCommandLineOption thresholdOption(
      QStringList() << "t" << "threshold",
      "瓶颈检测阈值倍率（默认 2.0，即 mean + 2σ）",
      "sigma", "2.0");
  parser.addOption(thresholdOption);

  QCommandLineOption hotspotTopOption(
      QStringList() << "n" << "top-n",
      "热点分析返回前 N 项（默认 10）",
      "n", "10");
  parser.addOption(hotspotTopOption);

  parser.process(app);

  if (!parser.isSet(inputOption)) {
    QTextStream err(stderr);
    err << "错误：必须指定 --input 参数。\n"
        << "用法：buildlens-cli --input <build_dir> [--output report.json]\n";
    return 1;
  }

  // ----- 参数解析 -----
  QString input_path = parser.value(inputOption);
  QString output_path = parser.value(outputOption);
  double threshold = parser.value(thresholdOption).toDouble();
  int top_n = parser.value(hotspotTopOption).toInt();
  if (threshold <= 0.0) threshold = 2.0;
  if (top_n <= 0) top_n = 10;

  // ----- 阶段 1：解析 -----
  QTextStream progress(stderr);
  progress << "[1/4] 解析 trace 文件..." << Qt::endl;

  std::vector<FileTraceResult> file_results;
  std::vector<TraceRecord> records;

  QFileInfo input_info(input_path);
  if (input_info.isDir()) {
    file_results = TraceParser::ParseDirectoryEvents(input_path);
    records = TraceParser::ParseDirectory(input_path);
  } else {
    // 单个文件。
    FileTraceResult single = TraceParser::ParseFileEvents(input_path);
    if (!single.events.empty()) {
      file_results.push_back(single);
    }
    records.push_back(TraceParser::ParseFile(input_path));
  }

  if (file_results.empty()) {
    progress << "警告：未找到有效的 -ftime-trace JSON 文件。\n";
    return 0;
  }

  progress << "  解析完成：" << file_results.size()
           << " 个源文件，共 " << records.size() << " 条记录\n";

  // ----- 阶段 2：构建调用图 + 分析 -----
  progress << "[2/4] 构建调用图并执行分析..." << Qt::endl;

  AnalysisReport report;
  report.total_file_count = static_cast<int>(file_results.size());

  // 文件摘要。
  for (const FileTraceResult& fr : file_results) {
    FileSummary fs;
    fs.filename = fr.filename;
    fs.source_path = fr.source_path;
    fs.event_count = static_cast<int>(fr.events.size());
    // 查找对应 TraceRecord 的总耗时。
    for (const TraceRecord& rec : records) {
      if (rec.source_path == fr.source_path) {
        fs.total_duration_ms = rec.total_duration_ms;
        report.total_build_time_s += rec.total_duration_ms / 1000.0;
        break;
      }
    }
    report.files.push_back(fs);

    // 对每个文件构建调用图并分析关键路径。
    CompileGraph graph = TraceGraphBuilder::Build(fr.source_path, fr.events);
    CriticalPathResult cp = CriticalPathAnalyzer::Analyze(graph);
    // 保留最长的关键路径（跨文件）。
    if (cp.total_duration_us > report.critical_path.total_duration_us) {
      report.critical_path = std::move(cp);
    }
  }

  // 热点分析（跨文件聚合）。
  progress << "[3/4] 热点分析..." << Qt::endl;
  report.hotspots_by_name =
      HotspotAnalyzer::Analyze(file_results,
                               HotspotAnalyzer::Dimension::kByName,
                               top_n);
  report.hotspots_by_category =
      HotspotAnalyzer::Analyze(file_results,
                               HotspotAnalyzer::Dimension::kByCategory,
                               top_n);

  // 瓶颈检测。
  progress << "[4/4] 瓶颈检测..." << Qt::endl;
  report.bottlenecks = BottleneckDetector::Detect(records, threshold);

  // ----- 输出 -----
  QString json_output = report.ToJsonString();

  if (!output_path.isEmpty()) {
    QFile out_file(output_path);
    if (out_file.open(QIODevice::WriteOnly | QIODevice::Text)) {
      out_file.write(json_output.toUtf8());
      out_file.close();
      progress << "报告已写入：" << output_path << Qt::endl;
    } else {
      progress << "错误：无法写入文件 " << output_path << Qt::endl;
      return 1;
    }
  } else {
    // 输出到 stdout。
    QTextStream out(stdout);
    out << json_output << Qt::endl;
  }

  progress << "分析完成。\n"
           << "  文件数：" << report.total_file_count << "\n"
           << "  总编译耗时：" << report.total_build_time_s << " s\n"
           << "  热点（按名称）Top-3：";
  for (int i = 0; i < std::min(3, static_cast<int>(
      report.hotspots_by_name.hotspots.size())); ++i) {
    if (i > 0) progress << ", ";
    progress << report.hotspots_by_name.hotspots[i].key
             << "("
             << static_cast<int>(report.hotspots_by_name.hotspots[i].percentage)
             << "%)";
  }
  progress << "\n";

  if (!report.bottlenecks.bottlenecks.empty()) {
    progress << "  检测到 " << report.bottlenecks.bottlenecks.size()
             << " 个瓶颈文件\n";
  }

  return 0;
}
