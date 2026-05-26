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
#include <QSplitter>
#include <QStatusBar>
#include <QTabWidget>
#include <QTableView>
#include <QVBoxLayout>

#include <algorithm>
#include <numeric>

#include "core/AnalysisPipeline.h"
#include "model/TraceTableModel.h"
#include "ui/BottleneckPanel.h"
#include "ui/CriticalPathPanel.h"
#include "ui/HotspotPanel.h"

namespace {

QLineEdit* CreateFilterInput(QWidget* parent) {
  QLineEdit* input = new QLineEdit(parent);
  input->setPlaceholderText(QStringLiteral("搜索文件名…"));
  input->setClearButtonEnabled(true);
  input->setMaximumWidth(300);
  return input;
}

}  // namespace

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent),
      table_view_(nullptr),
      filter_input_(nullptr),
      splitter_(nullptr),
      tab_widget_(nullptr),
      critical_path_panel_(nullptr),
      hotspot_panel_(nullptr),
      bottleneck_panel_(nullptr),
      model_(nullptr),
      proxy_model_(nullptr) {
  setWindowTitle(QStringLiteral("BuildLens — C++ 编译性能观测"));
  resize(1100, 750);

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

  // 上下分屏：表格 + 分析面板。
  splitter_ = new QSplitter(Qt::Vertical, this);

  // 上半：编译耗时表格。
  table_view_ = new QTableView(splitter_);
  table_view_->setModel(proxy_model_);
  table_view_->setSortingEnabled(true);
  table_view_->setAlternatingRowColors(true);
  table_view_->setSelectionBehavior(QAbstractItemView::SelectRows);
  table_view_->setSelectionMode(QAbstractItemView::SingleSelection);
  table_view_->horizontalHeader()->setStretchLastSection(true);
  table_view_->verticalHeader()->hide();

  // 下半：分析面板（TabWidget）。
  tab_widget_ = new QTabWidget(splitter_);

  critical_path_panel_ = new CriticalPathPanel(tab_widget_);
  tab_widget_->addTab(critical_path_panel_, QStringLiteral("关键路径"));

  hotspot_panel_ = new HotspotPanel(tab_widget_);
  tab_widget_->addTab(hotspot_panel_, QStringLiteral("热点"));

  bottleneck_panel_ = new BottleneckPanel(tab_widget_);
  tab_widget_->addTab(bottleneck_panel_, QStringLiteral("瓶颈"));

  splitter_->addWidget(table_view_);
  splitter_->addWidget(tab_widget_);
  splitter_->setStretchFactor(0, 6);
  splitter_->setStretchFactor(1, 4);

  main_layout->addWidget(splitter_);
  setCentralWidget(central_widget);

  // ----- 信号连接 -----
  connect(filter_input_, &QLineEdit::textChanged, this,
          &MainWindow::OnFilterChanged);
  connect(table_view_->selectionModel(),
          &QItemSelectionModel::currentRowChanged,
          this, &MainWindow::OnFileSelectionChanged);

  statusBar()->showMessage(
      QStringLiteral("请打开包含 -ftime-trace JSON 的目录"));
}

// ----- 槽函数 -----

void MainWindow::OnOpenDirectory() {
  QString dir_path = QFileDialog::getExistingDirectory(
      this, QStringLiteral("选择包含 -ftime-trace JSON 的目录"));
  if (dir_path.isEmpty()) {
    return;
  }
  LoadDirectory(dir_path);
}

void MainWindow::OnFilterChanged(const QString& text) {
  proxy_model_->setFilterFixedString(text);
  UpdateStatusBar();
}

void MainWindow::OnFileSelectionChanged(const QModelIndex& current,
                                        const QModelIndex& /*previous*/) {
  if (!current.isValid()) return;
  QModelIndex source_index = proxy_model_->mapToSource(current);
  if (!source_index.isValid()) return;
  UpdateDetailPanels(source_index.row());
}

// ----- 私有方法 -----

void MainWindow::LoadDirectory(const QString& dir_path) {
  QApplication::setOverrideCursor(Qt::WaitCursor);

  current_run_ = AnalysisPipeline::Run(dir_path);

  if (!current_run_.has_data) {
    QApplication::restoreOverrideCursor();
    const QString detail = current_run_.warnings.isEmpty()
        ? QStringLiteral("请确认目录中包含 Clang -ftime-trace 生成的 .json 文件。")
        : current_run_.warnings.join(QStringLiteral("\n"));
    QMessageBox::information(
        this, QStringLiteral("提示"),
        QStringLiteral("未找到有效的 -ftime-trace JSON 文件。\n\n%1").arg(detail));
    return;
  }

  model_->SetFiles(current_run_.report.files);
  table_view_->resizeColumnsToContents();

  // 更新全局面板（热点和瓶颈不依赖文件选择）。
  hotspot_panel_->SetHotspots(current_run_.report.hotspots_by_name,
                              current_run_.report.hotspots_by_category,
                              current_run_.report.source_hotspots);
  bottleneck_panel_->SetBottlenecks(current_run_.report.bottlenecks);

  QApplication::restoreOverrideCursor();
  UpdateStatusBar();

  // 自动选中第一行。
  if (proxy_model_->rowCount() > 0) {
    table_view_->selectRow(0);
  }
}

void MainWindow::UpdateStatusBar() {
  const int visible_count = proxy_model_->rowCount();
  if (visible_count == 0) {
    statusBar()->showMessage(QStringLiteral("无匹配数据"));
    return;
  }

  double total_ms = 0.0;
  int total_visible = 0;
  for (int proxy_row = 0; proxy_row < proxy_model_->rowCount(); ++proxy_row) {
    QModelIndex source_index =
        proxy_model_->mapToSource(proxy_model_->index(proxy_row, 0));
    if (!source_index.isValid()) continue;
    total_ms += model_->files()[source_index.row()].total_duration_ms;
    ++total_visible;
  }

  double avg_ms = (total_visible > 0) ? (total_ms / total_visible) : 0.0;

  const AnalysisReport& report = current_run_.report;

  QString summary = QStringLiteral("文件：%1 | 总耗时：%2 s | 平均：%3 s")
                        .arg(total_visible)
                        .arg(total_ms / 1000.0, 0, 'f', 1)
                        .arg(avg_ms / 1000.0, 0, 'f', 2);

  // v0.5: 追加分析摘要。
  if (report.total_file_count > 0) {
    QString cp_name;
    double best_pct = 0.0;
    if (!report.critical_path.path.empty()) {
      int best_idx = 0;
      for (int i = 0; i < static_cast<int>(report.critical_path.path.size()); ++i) {
        if (report.critical_path.path[i].percentage_of_root > best_pct &&
            report.critical_path.path[i].name != QStringLiteral("ExecuteCompiler")) {
          best_pct = report.critical_path.path[i].percentage_of_root;
          best_idx = i;
        }
      }
      cp_name = report.critical_path.path[best_idx].name;
    }

    QString top_hotspot;
    if (!report.hotspots_by_category.hotspots.empty()) {
      const HotspotItem& h = report.hotspots_by_category.hotspots[0];
      top_hotspot = QStringLiteral("%1(%2%)")
                        .arg(h.key)
                        .arg(static_cast<int>(h.percentage));
    }

    summary += QStringLiteral("  |  关键路径：%1(%2%)  |  热点：%3  |  瓶颈：%4")
                   .arg(cp_name.isEmpty() ? QStringLiteral("-") : cp_name)
                   .arg(cp_name.isEmpty()
                            ? 0
                            : static_cast<int>(best_pct))
                   .arg(top_hotspot.isEmpty() ? QStringLiteral("-") : top_hotspot)
                   .arg(report.bottlenecks.bottlenecks.size());
  }

  statusBar()->showMessage(summary);
}

void MainWindow::UpdateDetailPanels(int source_row) {
  if (source_row < 0 || source_row >= static_cast<int>(model_->files().size())) {
    critical_path_panel_->SetCriticalPath(CriticalPathResult{});
    return;
  }

  const QString& source_path = model_->files()[source_row].source_path;

  for (const auto& pfa : current_run_.report.per_file_analyses) {
    if (pfa.source_path == source_path) {
      critical_path_panel_->SetCriticalPath(pfa.critical_path);
      return;
    }
  }

  critical_path_panel_->SetCriticalPath(CriticalPathResult{});
}
