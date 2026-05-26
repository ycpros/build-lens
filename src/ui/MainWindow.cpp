// ============================================================
// BuildLens — C++ 编译性能观测工具
//
// MainWindow.cpp
// 功能：MainWindow 类实现，构建界面布局并串联用户操作。
// ============================================================

#include "ui/MainWindow.h"

#include <QApplication>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMenuBar>
#include <QMessageBox>
#include <QSortFilterProxyModel>
#include <QStatusBar>
#include <QTableView>
#include <QVBoxLayout>

#include <algorithm>
#include <numeric>

#include "core/AnalysisPipeline.h"
#include "model/TraceTableModel.h"

namespace {

// 创建搜索框，位于过滤区域。
QLineEdit* CreateFilterInput(QWidget* parent) {
  QLineEdit* input = new QLineEdit(parent);
  input->setPlaceholderText(QStringLiteral("搜索文件名…"));
  input->setClearButtonEnabled(true);
  input->setMaximumWidth(300);
  return input;
}

}  // namespace

// ----- 构造与析构 -----

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent),
      table_view_(nullptr),
      filter_input_(nullptr),
      model_(nullptr),
      proxy_model_(nullptr) {
  // 窗口基础属性。
  setWindowTitle(QStringLiteral("BuildLens — C++ 编译性能观测"));
  resize(1000, 600);

  // ----- 菜单栏 -----
  QMenu* file_menu = menuBar()->addMenu(QStringLiteral("文件(&F)"));
  QAction* open_action = file_menu->addAction(
      QStringLiteral("打开目录…"), this, &MainWindow::OnOpenDirectory);
  open_action->setShortcut(QKeySequence::Open);
  file_menu->addSeparator();
  file_menu->addAction(QStringLiteral("退出(&Q)"), qApp, &QApplication::quit);

  // ----- 数据模型 -----
  model_ = new TraceTableModel(this);
  proxy_model_ = new QSortFilterProxyModel(this);
  proxy_model_->setSourceModel(model_);
  // 按文件名过滤，忽略大小写。
  proxy_model_->setFilterCaseSensitivity(Qt::CaseInsensitive);
  proxy_model_->setFilterKeyColumn(TraceTableModel::kColumnFilename);

  // ----- 中央控件 -----
  QWidget* central_widget = new QWidget(this);
  QVBoxLayout* main_layout = new QVBoxLayout(central_widget);

  // 顶部：过滤输入框。
  QHBoxLayout* filter_layout = new QHBoxLayout();
  filter_layout->addWidget(new QLabel(QStringLiteral("过滤："), this));
  filter_input_ = CreateFilterInput(this);
  filter_layout->addWidget(filter_input_);
  filter_layout->addStretch();
  main_layout->addLayout(filter_layout);

  // 中央：编译耗时表格。
  table_view_ = new QTableView(this);
  table_view_->setModel(proxy_model_);
  table_view_->setSortingEnabled(true);
  table_view_->setAlternatingRowColors(true);
  table_view_->setSelectionBehavior(QAbstractItemView::SelectRows);
  table_view_->setSelectionMode(QAbstractItemView::SingleSelection);
  table_view_->horizontalHeader()->setStretchLastSection(true);
  table_view_->verticalHeader()->hide();
  main_layout->addWidget(table_view_);

  setCentralWidget(central_widget);

  // ----- 信号连接 -----
  connect(filter_input_, &QLineEdit::textChanged, this,
          &MainWindow::OnFilterChanged);

  // 初始状态。
  statusBar()->showMessage(
      QStringLiteral("请打开包含 -ftime-trace JSON 的目录"));
}

// ----- 槽函数 -----

void MainWindow::OnOpenDirectory() {
  QString dir_path = QFileDialog::getExistingDirectory(
      this, QStringLiteral("选择包含 -ftime-trace JSON 的目录"));
  if (dir_path.isEmpty()) {
    return;  // 用户取消选择。
  }
  LoadDirectory(dir_path);
}

void MainWindow::OnFilterChanged(const QString& text) {
  proxy_model_->setFilterFixedString(text);
  UpdateStatusBar();
}

// ----- 私有方法 -----

void MainWindow::LoadDirectory(const QString& dir_path) {
  QApplication::setOverrideCursor(Qt::WaitCursor);

  AnalysisRunResult run = AnalysisPipeline::Run(dir_path);

  if (!run.has_data) {
    QApplication::restoreOverrideCursor();
    const QString detail = run.warnings.isEmpty()
        ? QStringLiteral("请确认目录中包含 Clang -ftime-trace 生成的 .json 文件。")
        : run.warnings.join(QStringLiteral("\n"));
    QMessageBox::information(
        this, QStringLiteral("提示"),
        QStringLiteral("未找到有效的 -ftime-trace JSON 文件。\n\n"
                       "%1").arg(detail));
    return;
  }

  model_->SetFiles(run.report.files);
  table_view_->resizeColumnsToContents();

  QApplication::restoreOverrideCursor();
  UpdateStatusBar();
}

void MainWindow::UpdateStatusBar() {
  // 计算当前过滤后可见的记录数。
  const int visible_count = proxy_model_->rowCount();
  if (visible_count == 0) {
    statusBar()->showMessage(QStringLiteral("无匹配数据"));
    return;
  }

  // 遍历代理模型可见行，通过 mapToSource 获取源数据索引。
  double total_ms = 0.0;
  int total_visible = 0;
  for (int proxy_row = 0; proxy_row < proxy_model_->rowCount(); ++proxy_row) {
    // 将代理模型中的行索引映射回源模型。
    QModelIndex source_index =
        proxy_model_->mapToSource(proxy_model_->index(proxy_row, 0));
    if (!source_index.isValid()) continue;
    total_ms +=
        model_->files()[source_index.row()].total_duration_ms;
    ++total_visible;
  }

  double avg_ms =
      (total_visible > 0) ? (total_ms / total_visible) : 0.0;

  statusBar()->showMessage(
      QStringLiteral("文件：%1 | 总耗时：%2 s | 平均：%3 s")
          .arg(total_visible)
          .arg(total_ms / 1000.0, 0, 'f', 1)
          .arg(avg_ms / 1000.0, 0, 'f', 2));
}
