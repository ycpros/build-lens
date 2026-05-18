// ============================================================
// BuildLens — C++ 编译性能观测工具
//
// MainWindow.h
// 功能：应用程序主窗口，包含菜单栏、文件选择、表格展示、
//       搜索过滤和状态栏信息。
// ============================================================

#ifndef BUILD_LENS_MAINWINDOW_H_
#define BUILD_LENS_MAINWINDOW_H_

#include <QMainWindow>
#include <memory>

class QLineEdit;
class QSortFilterProxyModel;
class QStatusBar;
class QTableView;
class TraceTableModel;

// BuildLens 主窗口。
//
// 布局：
//   - 菜单栏：File → Open Directory 打开编译输出目录
//   - 工具栏区：搜索框（按文件名实时过滤）
//   - 中央区域：QTableView 展示编译耗时列表
//   - 状态栏：文件总数、总耗时、平均耗时
class MainWindow : public QMainWindow {
  Q_OBJECT

 public:
  explicit MainWindow(QWidget* parent = nullptr);
  ~MainWindow() override = default;

 private slots:
  // 弹出目录选择对话框并加载 -ftime-trace 数据。
  void OnOpenDirectory();

  // 搜索框文本变化时触发过滤。
  void OnFilterChanged(const QString& text);

 private:
  // 加载指定目录下的 -ftime-trace JSON 数据并更新界面。
  void LoadDirectory(const QString& dir_path);

  // 更新状态栏：文件数、总耗时、平均耗时。
  void UpdateStatusBar();

  // ----- UI 组件 -----
  QTableView* table_view_;
  QLineEdit* filter_input_;

  // ----- 数据模型 -----
  TraceTableModel* model_;
  QSortFilterProxyModel* proxy_model_;
};

#endif  // BUILD_LENS_MAINWINDOW_H_
