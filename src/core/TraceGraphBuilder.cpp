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
#include <stack>

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
                return a.ts < b.ts;
              });

    auto roots = BuildThreadGraph(pair.second);
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
  std::stack<TraceNode*> node_stack;

  for (const TraceEvent& ev : thread_events) {
    // 跳过持续时间为 0 的事件（可能是瞬时事件）。
    if (ev.dur <= 0.0) continue;

    // 创建节点。
    TraceNode node;
    node.name = ev.name;
    node.category = ev.category;
    node.start_us = ev.ts;
    node.end_us = ev.ts + ev.dur;
    node.duration_us = ev.dur;
    node.thread_id = ev.tid;

    // 弹出已结束的节点：栈顶节点 end_us <= 当前事件 start_us。
    // 这意味着栈顶节点在 当前事件开始之前就已经结束了。
    while (!node_stack.empty() &&
           node_stack.top()->end_us <= node.start_us) {
      node_stack.pop();
    }

    // ── 核心判断 ──
    // 检查栈顶节点是否嵌套当前事件：
    // 条件：栈顶 end_us >= 当前事件 end_us 且 start_us <= 当前事件 start_us
    // 这意味着当前事件完全在栈顶节点的时间范围内。
    bool nested = false;
    if (!node_stack.empty()) {
      TraceNode* parent = node_stack.top();
      if (parent->end_us >= node.end_us &&
          parent->start_us <= node.start_us) {
        parent->children.push_back(std::move(node));
        // 新节点现在是栈中最深层的事件。
        node_stack.push(&parent->children.back());
        nested = true;
      }
    }

    if (!nested) {
      // 当前事件不在任何活跃节点内部，它是新的根节点
      //（对单线程编译，通常只有 ExecuteCompiler 会是根节点）。
      roots.push_back(std::move(node));
      node_stack.push(&roots.back());
    }
  }

  // 去重：如果存在多个根但其中某些实际是嵌套关系，合并之。
  // （栈算法已保证正确性，此步骤仅处理极端边缘情况。）
  if (roots.size() > 1) {
    // 检查是否某个根是另一个根的父节点（按时间包含关系）。
    for (size_t i = 0; i < roots.size(); ++i) {
      for (size_t j = 0; j < roots.size(); ++j) {
        if (i != j &&
            roots[i].start_us <= roots[j].start_us &&
            roots[i].end_us >= roots[j].end_us) {
          // roots[i] 包含了 roots[j]，将 roots[j] 移入 roots[i] 的子节点。
          roots[i].children.push_back(std::move(roots[j]));
          roots.erase(roots.begin() + static_cast<long long>(j));
          // 调整索引。
          if (j < i) --i;
          --j;
          if (roots.size() <= 1) break;
        }
      }
    }
  }

  return roots;
}
