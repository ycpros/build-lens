// ============================================================
// BuildLens — C++ 编译性能观测工具
//
// HotspotPanel.h
// 功能：展示全局热点排行（按名称/类别/Source 三个维度切换）。
// ============================================================

#ifndef BUILD_LENS_HOTSPOTPANEL_H_
#define BUILD_LENS_HOTSPOTPANEL_H_

#include <QComboBox>
#include <QTableWidget>
#include <QWidget>

#include "core/HotspotAnalyzer.h"

class HotspotPanel : public QWidget {
  Q_OBJECT

 public:
  explicit HotspotPanel(QWidget* parent = nullptr);

  // 设置三个维度的热点数据。
  void SetHotspots(const HotspotResult& by_name,
                   const HotspotResult& by_category,
                   const HotspotResult& source);

 private slots:
  void OnDimensionChanged(int index);

 private:
  void PopulateTable(const HotspotResult& hr);

  QComboBox* combo_;
  QTableWidget* table_;

  HotspotResult by_name_;
  HotspotResult by_category_;
  HotspotResult source_;
};

#endif  // BUILD_LENS_HOTSPOTPANEL_H_
