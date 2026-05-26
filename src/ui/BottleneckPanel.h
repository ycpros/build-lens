// ============================================================
// BuildLens — C++ 编译性能观测工具
//
// BottleneckPanel.h
// 功能：展示瓶颈检测结果（统计摘要 + 异常文件列表）。
// ============================================================

#ifndef BUILD_LENS_BOTTLENECKPANEL_H_
#define BUILD_LENS_BOTTLENECKPANEL_H_

#include <QLabel>
#include <QTableWidget>
#include <QWidget>

#include "core/BottleneckDetector.h"

class BottleneckPanel : public QWidget {
  Q_OBJECT

 public:
  explicit BottleneckPanel(QWidget* parent = nullptr);

  // 更新展示瓶颈检测结果。
  void SetBottlenecks(const BottleneckResult& result);

 private:
  QLabel* summary_;
  QTableWidget* table_;
};

#endif  // BUILD_LENS_BOTTLENECKPANEL_H_
