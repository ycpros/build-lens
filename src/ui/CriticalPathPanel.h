// ============================================================
// BuildLens — C++ 编译性能观测工具
//
// CriticalPathPanel.h
// 功能：展示选中文件的关键路径链（从根到叶的嵌套事件列表）。
// ============================================================

#ifndef BUILD_LENS_CRITICALPATHPANEL_H_
#define BUILD_LENS_CRITICALPATHPANEL_H_

#include <QTreeWidget>

#include "core/CriticalPathAnalyzer.h"

class CriticalPathPanel : public QTreeWidget {
  Q_OBJECT

 public:
  explicit CriticalPathPanel(QWidget* parent = nullptr);

  // 更新展示指定文件的关键路径（空路径 → 显示"请选择一个文件"）。
  void SetCriticalPath(const CriticalPathResult& cp);

 private:
  static QString FormatUs(double us);
};

#endif  // BUILD_LENS_CRITICALPATHPANEL_H_
