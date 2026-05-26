// ============================================================
// BuildLens — C++ 编译性能观测工具
//
// HotspotPanel.cpp
// 功能：HotspotPanel 实现。
// ============================================================

#include "ui/HotspotPanel.h"

#include <QHBoxLayout>
#include <QHeaderView>
#include <QVBoxLayout>

HotspotPanel::HotspotPanel(QWidget* parent) : QWidget(parent) {
  QVBoxLayout* layout = new QVBoxLayout(this);
  layout->setContentsMargins(0, 0, 0, 0);

  combo_ = new QComboBox(this);
  combo_->addItem(QStringLiteral("按事件名称"));
  combo_->addItem(QStringLiteral("按事件类别"));
  combo_->addItem(QStringLiteral("按头文件/Source"));
  layout->addWidget(combo_);

  table_ = new QTableWidget(this);
  table_->setColumnCount(5);
  table_->setHorizontalHeaderLabels({QStringLiteral("排名"),
                                     QStringLiteral("名称"),
                                     QStringLiteral("独占耗时"),
                                     QStringLiteral("出现次数"),
                                     QStringLiteral("占比")});
  table_->setAlternatingRowColors(true);
  table_->setSelectionMode(QAbstractItemView::NoSelection);
  table_->setEditTriggers(QAbstractItemView::NoEditTriggers);
  table_->verticalHeader()->hide();
  table_->horizontalHeader()->setStretchLastSection(true);
  table_->setColumnWidth(0, 50);
  table_->setColumnWidth(1, 240);
  table_->setColumnWidth(2, 110);
  table_->setColumnWidth(3, 70);

  layout->addWidget(table_);

  connect(combo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
          this, &HotspotPanel::OnDimensionChanged);
}

void HotspotPanel::SetHotspots(const HotspotResult& by_name,
                                const HotspotResult& by_category,
                                const HotspotResult& source) {
  by_name_ = by_name;
  by_category_ = by_category;
  source_ = source;
  OnDimensionChanged(combo_->currentIndex());
}

void HotspotPanel::OnDimensionChanged(int index) {
  switch (index) {
    case 0: PopulateTable(by_name_); break;
    case 1: PopulateTable(by_category_); break;
    case 2: PopulateTable(source_); break;
  }
}

void HotspotPanel::PopulateTable(const HotspotResult& hr) {
  table_->setRowCount(static_cast<int>(hr.hotspots.size()));

  for (int i = 0; i < static_cast<int>(hr.hotspots.size()); ++i) {
    const HotspotItem& item = hr.hotspots[i];

    QTableWidgetItem* rank = new QTableWidgetItem(
        QString::number(i + 1));
    rank->setTextAlignment(Qt::AlignCenter);
    table_->setItem(i, 0, rank);

    table_->setItem(i, 1, new QTableWidgetItem(item.key));

    QString dur_text;
    if (item.duration_us >= 1'000'000.0) {
      dur_text = QStringLiteral("%1 s").arg(item.duration_us / 1'000'000.0, 0, 'f', 3);
    } else {
      dur_text = QStringLiteral("%1 ms").arg(item.duration_us / 1'000.0, 0, 'f', 2);
    }
    QTableWidgetItem* dur = new QTableWidgetItem(dur_text);
    dur->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
    table_->setItem(i, 2, dur);

    QTableWidgetItem* occ = new QTableWidgetItem(
        QString::number(item.occurrence_count));
    occ->setTextAlignment(Qt::AlignCenter);
    table_->setItem(i, 3, occ);

    QTableWidgetItem* pct = new QTableWidgetItem(
        QStringLiteral("%1%").arg(item.percentage, 0, 'f', 1));
    pct->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
    table_->setItem(i, 4, pct);
  }
}
