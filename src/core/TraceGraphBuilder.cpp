// ============================================================
// BuildLens — C++ 编译性能观测工具
//
// TraceGraphBuilder.cpp
// 功能：TraceGraphBuilder 实现，核心栈算法将扁平事件列表
//       重建为嵌套调用树。
// ============================================================

#include "core/TraceGraphBuilder.h"

#include <QFileInfo>

#include <algorithm>
#include <map>

namespace {

bool Contains(const TraceNode& parent, const TraceNode& child) {
  return parent.start_us <= child.start_us && parent.end_us >= child.end_us;
}

double ClippedChildDuration(const TraceNode& parent, const TraceNode& child) {
  const double start = std::max(parent.start_us, child.start_us);
  const double end = std::min(parent.end_us, child.end_us);
  return std::max(0.0, end - start);
}

void ComputeExclusiveDuration(TraceNode& node) {
  double child_duration_sum = 0.0;
  for (TraceNode& child : node.children) {
    ComputeExclusiveDuration(child);
    child_duration_sum += ClippedChildDuration(node, child);
  }

  node.exclusive_duration_us = node.duration_us - child_duration_sum;
  node.exclusive_duration_us =
      std::max(0.0, std::min(node.duration_us, node.exclusive_duration_us));
}

}  // namespace

// static
CompileGraph TraceGraphBuilder::Build(
    const QString& source_path,
    const std::vector<TraceEvent>& events) {
  CompileGraph graph;
  graph.source_path = source_path;

  // 从路径推断源文件名。
  QFileInfo fi(source_path);
  QString name = fi.fileName();
  if (name.endsWith(".json", Qt::CaseInsensitive)) {
    name.chop(5);
  }
  graph.source_name = name.isEmpty() ? source_path : name;

  if (events.empty()) {
    return graph;
  }

  // 按 (pid, tid) 分组事件。
  // key = "pid:tic"，value = 该线程的事件列表。
  std::map<QString, std::vector<TraceEvent>> thread_groups;
  for (const TraceEvent& ev : events) {
    if (ev.is_summary || ev.dur <= 0.0 || ev.name.isEmpty()) {
      continue;
    }
    QString key = QStringLiteral("%1:%2")
                      .arg(ev.pid)
                      .arg(ev.tid);
    thread_groups[key].push_back(ev);
  }

  // 对每个线程分别构建调用树。
  for (auto& pair : thread_groups) {
    // 确保同一线程内按 ts 排序。
    std::sort(pair.second.begin(), pair.second.end(),
              [](const TraceEvent& a, const TraceEvent& b) {
                if (a.ts != b.ts) return a.ts < b.ts;
                return a.dur > b.dur;
              });

    auto roots = BuildThreadGraph(pair.second);
    for (TraceNode& root : roots) {
      ComputeExclusiveDuration(root);
    }
    for (auto& root : roots) {
      graph.roots.push_back(std::move(root));
    }
  }

  return graph;
}

// static
std::vector<TraceNode> TraceGraphBuilder::BuildThreadGraph(
    const std::vector<TraceEvent>& thread_events) {
  std::vector<TraceNode> roots;

  // 栈保存当前开启但尚未结束的事件节点。
  // 栈顶 = 当前活跃的最深层事件。
  std::vector<TraceNode*> node_stack;

  for (const TraceEvent& ev : thread_events) {
    // 跳过持续时间为 0 的事件（可能是瞬时事件）。
    if (ev.dur <= 0.0) continue;

    // 创建节点。
    TraceNode node;
    node.name = ev.name;
    node.category = ev.category;
    node.detail = ev.detail;
    node.start_us = ev.ts;
    node.end_us = ev.ts + ev.dur;
    node.duration_us = ev.dur;
    node.thread_id = ev.tid;

    // 弹出无法完整包含当前事件的节点。严格包含避免部分重叠被误建为父子。
    while (!node_stack.empty() && !Contains(*node_stack.back(), node)) {
      node_stack.pop_back();
    }

    if (!node_stack.empty()) {
      TraceNode* parent = node_stack.back();
      parent->children.push_back(std::move(node));
      node_stack.push_back(&parent->children.back());
    } else {
      // 当前事件不在任何活跃节点内部，它是新的根节点
      //（对单线程编译，通常只有 ExecuteCompiler 会是根节点）。
      roots.push_back(std::move(node));
      node_stack.push_back(&roots.back());
    }
  }

  return roots;
}
