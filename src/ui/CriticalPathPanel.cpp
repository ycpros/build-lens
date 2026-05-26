// ============================================================
// BuildLens — C++ 编译性能观测工具
//
// CriticalPathPanel.cpp
// 功能：CriticalPathPanel 实现。
// ============================================================

#include "ui/CriticalPathPanel.h"

#include <QHeaderView>

CriticalPathPanel::CriticalPathPanel(QWidget* parent)
    : QTreeWidget(parent) {
  setHeaderLabels({QStringLiteral("名称"),
                   QStringLiteral("包含耗时"),
                   QStringLiteral("独占耗时"),
                   QStringLiteral("占比")});
  setRootIsDecorated(false);
  setAlternatingRowColors(true);
  setSelectionMode(QAbstractItemView::NoSelection);
  header()->setStretchLastSection(true);
  setColumnWidth(0, 260);
  setColumnWidth(1, 120);
  setColumnWidth(2, 120);
}

void CriticalPathPanel::SetCriticalPath(const CriticalPathResult& cp) {
  clear();

  if (cp.path.empty()) {
    QTreeWidgetItem* placeholder = new QTreeWidgetItem(this);
    placeholder->setText(0, QStringLiteral("请选择一个文件以查看其关键路径"));
    placeholder->setFlags(placeholder->flags() & ~Qt::ItemIsSelectable);
    return;
  }

  // 构建缩进链：每条路径项是顶层节点，用空白前缀模拟层级。
  for (int i = 0; i < static_cast<int>(cp.path.size()); ++i) {
    const CriticalPathItem& item = cp.path[i];
    QTreeWidgetItem* tree_item = new QTreeWidgetItem(this);

    // 缩进前缀。
    QString indent(i * 2, QChar(' '));
    tree_item->setText(0, indent + item.name);
    tree_item->setText(1, FormatUs(item.inclusive_duration_us));
    tree_item->setText(2, FormatUs(item.exclusive_duration_us));
    tree_item->setText(3, QStringLiteral("%1%")
                               .arg(item.percentage_of_root, 0, 'f', 1));

    tree_item->setTextAlignment(1, Qt::AlignRight | Qt::AlignVCenter);
    tree_item->setTextAlignment(2, Qt::AlignRight | Qt::AlignVCenter);
    tree_item->setTextAlignment(3, Qt::AlignRight | Qt::AlignVCenter);
    tree_item->setFlags(tree_item->flags() & ~Qt::ItemIsSelectable);

    if (!item.detail.isEmpty()) {
      tree_item->setToolTip(0, item.detail);
    }
  }
}

// static
QString CriticalPathPanel::FormatUs(double us) {
  if (us >= 1'000'000.0) {
    return QStringLiteral("%1 s").arg(us / 1'000'000.0, 0, 'f', 3);
  }
  if (us >= 1'000.0) {
    return QStringLiteral("%1 ms").arg(us / 1'000.0, 0, 'f', 2);
  }
  return QStringLiteral("%1 μs").arg(us, 0, 'f', 0);
}
