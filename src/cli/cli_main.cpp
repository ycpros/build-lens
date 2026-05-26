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
#include <QTextStream>

#include <algorithm>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

#include "core/AnalysisPipeline.h"
#include "core/BuildLensVersion.h"

int main(int argc, char* argv[]) {
  QCoreApplication app(argc, argv);
  QCoreApplication::setApplicationName("BuildLens CLI");
  QCoreApplication::setApplicationVersion(
      QString::fromLatin1(buildlens::kApplicationVersion));

  // Windows 控制台默认 GBK，SetConsoleOutputCP(CP_UTF8) 修复中文乱码。
#ifdef Q_OS_WIN
  SetConsoleOutputCP(CP_UTF8);
  SetConsoleCP(CP_UTF8);
#endif

  // ----- 命令行参数 -----
  QCommandLineParser parser;
  parser.setApplicationDescription(
      "BuildLens — C++ 编译性能分析工具 (CLI)\n"
      "解析 Clang -ftime-trace JSON 并输出结构化分析报告。\n"
      "输出格式：JSON（v0.5 schema）。");
  parser.addHelpOption();

  QCommandLineOption versionOption(
      QStringList() << "V" << "version",
      "显示版本信息。");
  parser.addOption(versionOption);

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

  QCommandLineOption quietOption(
      QStringList() << "q" << "quiet",
      "静默模式，仅输出错误信息到 stderr。");
  parser.addOption(quietOption);

  parser.process(app);

  if (parser.isSet(versionOption)) {
    QTextStream out(stdout);
    out << "BuildLens CLI "
        << QString::fromLatin1(buildlens::kApplicationVersion) << "\n";
    return 0;
  }

  const bool quiet = parser.isSet(quietOption);

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

  // ----- 执行分析管线 -----
  QTextStream progress(stderr);
  QTextStream err(stderr);
  if (!quiet) {
    progress << "[1/3] 执行分析管线..." << Qt::endl;
  }

  AnalysisOptions options;
  options.threshold_sigma = threshold;
  options.top_n = top_n;

  AnalysisRunResult run_result = AnalysisPipeline::Run(input_path, options);
  for (const QString& warning : run_result.warnings) {
    err << "警告：" << warning << "\n";
  }

  if (!run_result.has_data) {
    return 0;
  }

  const AnalysisReport& report = run_result.report;
  if (!quiet) {
    progress << "  解析完成：" << report.total_file_count
             << " 个源文件\n";
  }

  // ----- 输出 -----
  if (!quiet) {
    progress << "[2/3] 输出报告..." << Qt::endl;
  }
  QString json_output = report.ToJsonString();

  if (!output_path.isEmpty()) {
    QFile out_file(output_path);
    if (out_file.open(QIODevice::WriteOnly | QIODevice::Text)) {
      out_file.write(json_output.toUtf8());
      out_file.close();
      if (!quiet) {
        progress << "报告已写入：" << output_path << Qt::endl;
      }
    } else {
      err << "错误：无法写入文件 " << output_path << Qt::endl;
      return 1;
    }
  } else {
    QTextStream out(stdout);
    out << json_output << Qt::endl;
  }

  if (!quiet) {
    progress << "[3/3] 汇总结果..." << Qt::endl;
    progress << "分析完成。\n"
             << "  文件数：" << report.total_file_count << "\n"
             << "  总编译耗时：" << report.total_build_time_s << " s\n"
             << "  最慢文件 Top-3：";
    for (int i = 0; i < std::min(3, static_cast<int>(
        report.files.size())); ++i) {
      if (i > 0) progress << ", ";
      progress << report.files[i].filename
               << "("
               << report.files[i].total_duration_ms / 1000.0
               << "s)";
    }
    progress << "\n"
             << "  热点类别 Top-3：";
    for (int i = 0; i < std::min(3, static_cast<int>(
        report.hotspots_by_category.hotspots.size())); ++i) {
      if (i > 0) progress << ", ";
      progress << report.hotspots_by_category.hotspots[i].key
               << "("
               << static_cast<int>(report.hotspots_by_category.hotspots[i].percentage)
               << "%)";
    }
    progress << "\n"
             << "  Source 热点 Top-3：";
    for (int i = 0; i < std::min(3, static_cast<int>(
        report.source_hotspots.hotspots.size())); ++i) {
      if (i > 0) progress << ", ";
      progress << report.source_hotspots.hotspots[i].key
               << "("
               << static_cast<int>(report.source_hotspots.hotspots[i].percentage)
               << "%)";
    }
    progress << "\n";

    if (!report.bottlenecks.bottlenecks.empty()) {
      progress << "  检测到 " << report.bottlenecks.bottlenecks.size()
               << " 个瓶颈文件\n";
    }
  }

  return 0;
}
