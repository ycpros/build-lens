// ============================================================
// BuildLens — C++ 编译性能观测工具
//
// TraceGraph.h
// 功能：定义编译调用图的数据结构。
//       TraceNode 表示调用树中的一个节点（事件），
//       CompileGraph 聚合一个源文件的所有线程的调用树。
// ============================================================

#ifndef BUILD_LENS_TRACEGRAPH_H_
#define BUILD_LENS_TRACEGRAPH_H_

#include <QString>

#include <vector>

// 调用树中的单个节点。
// 对应一个 trace 事件，通过 children 字段维护父子嵌套关系。
// 所有时间字段单位为微秒。
struct TraceNode {
  // 事件名称，例如 "ExecuteCompiler"、"ParseAST"。
  QString name;

  // 事件类别，例如 "parse"、"codegen"、"inst"。
  QString category;

  // 开始时间，单位微秒。
  double start_us = 0.0;

  // 结束时间（start_us + duration），单位微秒。
  double end_us = 0.0;

  // 本事件自身的持续时间，单位微秒（不含子节点重叠部分时为 exclusive 时间）。
  double duration_us = 0.0;

  // 所属线程 ID。
  int thread_id = 1;

  // 子事件列表。
  // 每个子事件的时间范围完全包含在当前节点的时间范围内。
  std::vector<TraceNode> children;
};

// 编译调用图。
// 一个源文件的 trace 数据经过 GraphBuilder 处理后得到的嵌套调用结构。
struct CompileGraph {
  // 源文件名，例如 "renderer.cpp"。
  QString source_name;

  // JSON 文件的完整路径。
  QString source_path;

  // 按线程分组的根节点列表。
  // 每个线程的根节点是该线程时间线上最外层的事件（通常是 ExecuteCompiler）。
  // 单线程编译时只有一个根节点。
  std::vector<TraceNode> roots;
};

#endif  // BUILD_LENS_TRACEGRAPH_H_
