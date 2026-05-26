// ============================================================
// BuildLens — C++ 编译性能观测工具
//
// MainWindow.h
// 功能：应用程序主窗口，包含菜单栏、文件表、分析面板和状态栏。
// ============================================================

#ifndef BUILD_LENS_MAINWINDOW_H_
#define BUILD_LENS_MAINWINDOW_H_

#include <QMainWindow>
#include <memory>

#include "core/AnalysisPipeline.h"

class BottleneckPanel;
class CriticalPathPanel;
class HotspotPanel;
class QLineEdit;
class QSortFilterProxyModel;
class QSplitter;
class QTabWidget;
class QTableView;
class TraceTableModel;

class MainWindow : public QMainWindow {
  Q_OBJECT

 public:
  explicit MainWindow(QWidget* parent = nullptr);
  ~MainWindow() override = default;

 private slots:
  void OnOpenDirectory();
  void OnFilterChanged(const QString& text);
  void OnFileSelectionChanged(const QModelIndex& current,
                              const QModelIndex& previous);

 private:
  void LoadDirectory(const QString& dir_path);
  void UpdateStatusBar();
  void UpdateDetailPanels(int source_row);

  // ----- UI 组件 -----
  QTableView* table_view_;
  QLineEdit* filter_input_;
  QSplitter* splitter_;
  QTabWidget* tab_widget_;

  // ----- 分析面板 -----
  CriticalPathPanel* critical_path_panel_;
  HotspotPanel* hotspot_panel_;
  BottleneckPanel* bottleneck_panel_;

  // ----- 数据模型 -----
  TraceTableModel* model_;
  QSortFilterProxyModel* proxy_model_;

  // ----- 当前分析结果 -----
  AnalysisRunResult current_run_;
};

#endif  // BUILD_LENS_MAINWINDOW_H_
