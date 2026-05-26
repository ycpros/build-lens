// ============================================================
// BuildLens — C++ 编译性能观测工具
//
// TraceEvent.h
// 功能：定义单条编译 trace 事件的完整数据结构。
//       对应 Clang -ftime-trace JSON 中 traceEvents 数组的每一项。
//       保留事件的嵌套关系和时间范围，供 GraphBuilder 重建调用树。
// ============================================================

#ifndef BUILD_LENS_TRACEEVENT_H_
#define BUILD_LENS_TRACEEVENT_H_

#include <QString>

// 单条 trace 事件。
// 存储 Clang -ftime-trace Chrome Trace Event 格式的原始字段，
// 供 TraceGraphBuilder 重建嵌套调用关系。
struct TraceEvent {
  // 事件名称，例如 "ExecuteCompiler"、"ParseAST"、"Instantiate"。
  QString name;

  // 事件类别，例如 "parse"、"codegen"、"driver"。
  // 对应 JSON 的 "cat" 字段；Clang 实际使用 "cat" 而非 "ph" 的类别语义。
  QString category;

  // 事件细节，例如 Source 事件中的头文件路径，或解析事件中的 args.detail。
  QString detail;

  // 事件阶段类型，通常为 "X"（Complete Event）。
  // "X" 表示有开始和持续时间的事件。
  QString phase;

  // 开始时间戳，单位微秒。
  double ts = 0.0;

  // 持续时间，单位微秒。
  double dur = 0.0;

  // 进程 ID，同一编译单元通常 pid 相同。
  int pid = 0;

  // 线程 ID，多线程编译时区分不同编译线程。
  int tid = 0;

  // Clang 自带的 "Total ..." 汇总事件。默认分析会排除这类事件，
  // 避免和真实时间线事件重复计数。
  bool is_summary = false;
};

#endif  // BUILD_LENS_TRACEEVENT_H_
