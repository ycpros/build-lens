// ============================================================
// BuildLens — C++ 编译性能观测工具
//
// TraceGraphBuilder.h
// 功能：将 TraceParser 输出的扁平 TraceEvent 列表，
//       按线程重建嵌套调用树（CompileGraph）。
//       核心算法：按时间排序 + 栈结构恢复嵌套关系。
// ============================================================

#ifndef BUILD_LENS_TRACEGRAPHBUILDER_H_
#define BUILD_LENS_TRACEGRAPHBUILDER_H_

#include <QString>

#include <vector>

#include "core/TraceEvent.h"
#include "core/TraceGraph.h"

// 调用图构建器。
//
// 用法：
//   auto events = TraceParser::ParseFileEvents("renderer.cpp.json");
//   CompileGraph graph = TraceGraphBuilder::Build("renderer.cpp.json",
//                                                  events.events);
//
// 算法：
//   对每个线程，将事件按 ts 排序，使用栈结构恢复嵌套关系。
//   同一线程中：时间范围完全包含的事件成为父事件的子节点。
//   不同线程之间视为并行（各自独立的根节点）。
class TraceGraphBuilder {
 public:
  // 从事件列表构建 CompileGraph。
  // source_path：JSON 文件的完整路径（用于生成 source_name）。
  // events：ParseFileEvents 输出的全部事件（已按 ts 排序）。
  static CompileGraph Build(const QString& source_path,
                            const std::vector<TraceEvent>& events);

 private:
  // 对单个线程的事件列表构建调用树。
  // thread_events：同一线程的 TraceEvent，假设已按 ts 升序排列。
  // 返回该线程的根节点列表（通常只有一个根节点）。
  static std::vector<TraceNode> BuildThreadGraph(
      const std::vector<TraceEvent>& thread_events);
};

#endif  // BUILD_LENS_TRACEGRAPHBUILDER_H_
