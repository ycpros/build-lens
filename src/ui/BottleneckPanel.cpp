// ============================================================
// BuildLens — C++ 编译性能观测工具
//
// BottleneckPanel.cpp
// 功能：BottleneckPanel 实现。
// ============================================================

#include "ui/BottleneckPanel.h"

#include <QHeaderView>
#include <QVBoxLayout>

BottleneckPanel::BottleneckPanel(QWidget* parent) : QWidget(parent) {
  QVBoxLayout* layout = new QVBoxLayout(this);
  layout->setContentsMargins(0, 0, 0, 0);

  summary_ = new QLabel(this);
  summary_->setWordWrap(true);
  layout->addWidget(summary_);

  table_ = new QTableWidget(this);
  table_->setColumnCount(3);
  table_->setHorizontalHeaderLabels({QStringLiteral("文件"),
                                     QStringLiteral("耗时"),
                                     QStringLiteral("异常分数")});
  table_->setAlternatingRowColors(true);
  table_->setSelectionMode(QAbstractItemView::NoSelection);
  table_->setEditTriggers(QAbstractItemView::NoEditTriggers);
  table_->verticalHeader()->hide();
  table_->horizontalHeader()->setStretchLastSection(true);
  table_->setColumnWidth(0, 260);
  table_->setColumnWidth(1, 100);

  layout->addWidget(table_);
}

void BottleneckPanel::SetBottlenecks(const BottleneckResult& br) {
  summary_->setText(
      QStringLiteral("均值 %1 s | 标准差 %2 s | 阈值 %3 s (μ+%4σ) | 检出 %5 个瓶颈")
          .arg(br.mean_ms / 1000.0, 0, 'f', 2)
          .arg(br.stddev_ms / 1000.0, 0, 'f', 2)
          .arg(br.threshold_ms / 1000.0, 0, 'f', 2)
          .arg(br.threshold_multiplier, 0, 'f', 1)
          .arg(br.bottlenecks.size()));

  table_->setRowCount(static_cast<int>(br.bottlenecks.size()));

  for (int i = 0; i < static_cast<int>(br.bottlenecks.size()); ++i) {
    const BottleneckItem& item = br.bottlenecks[i];

    table_->setItem(i, 0, new QTableWidgetItem(item.name));

    QTableWidgetItem* dur = new QTableWidgetItem(
        QStringLiteral("%1 s").arg(item.duration_ms / 1000.0, 0, 'f', 3));
    dur->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
    table_->setItem(i, 1, dur);

    QTableWidgetItem* score = new QTableWidgetItem(
        QString::number(item.anomaly_score, 'f', 2));
    score->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
    table_->setItem(i, 2, score);
  }
}
