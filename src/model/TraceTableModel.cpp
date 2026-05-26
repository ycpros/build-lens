// ============================================================
// BuildLens — C++ 编译性能观测工具
//
// TraceTableModel.cpp
// 功能：TraceTableModel 类实现，驱动 QTableView 的数据展示与排序。
// ============================================================

#include "model/TraceTableModel.h"

#include <QBrush>
#include <QColor>
#include <QFont>
#include <algorithm>

namespace {

// 将毫秒格式化为人类可读的耗时字符串。
// 例如：1234 ms → "1.23 s"，38200 ms → "38.2 s"。
QString FormatDuration(double ms) {
  if (ms < 1000.0) {
    return QString("%1 ms").arg(static_cast<int>(ms));
  }
  return QString("%1 s").arg(ms / 1000.0, 0, 'f', 1);
}

}  // namespace

// ----- 构造与数据更新 -----

TraceTableModel::TraceTableModel(QObject* parent)
    : QAbstractTableModel(parent) {}

void TraceTableModel::SetFiles(const std::vector<FileSummary>& files) {
  beginResetModel();
  files_ = files;
  // 默认按耗时降序排列。
  sort(kColumnDurationMs, Qt::DescendingOrder);
  endResetModel();
}

// ----- QAbstractTableModel 接口 -----

int TraceTableModel::rowCount(const QModelIndex& parent) const {
  if (parent.isValid()) return 0;
  return static_cast<int>(files_.size());
}

int TraceTableModel::columnCount(const QModelIndex& parent) const {
  if (parent.isValid()) return 0;
  return kColumnCount;
}

QVariant TraceTableModel::data(const QModelIndex& index, int role) const {
  if (!index.isValid()) return {};
  if (index.row() < 0 || index.row() >= static_cast<int>(files_.size())) {
    return {};
  }

  const FileSummary& file = files_[index.row()];

  if (role == Qt::DisplayRole) {
    switch (index.column()) {
      case kColumnFilename:
        return file.filename;
      case kColumnDurationMs:
        return FormatDuration(file.total_duration_ms);
      case kColumnSourcePath:
        return file.source_path;
      default:
        return {};
    }
  }

  // 耗时列靠右对齐，便于阅读数字。
  if (role == Qt::TextAlignmentRole && index.column() == kColumnDurationMs) {
    return static_cast<int>(Qt::AlignRight | Qt::AlignVCenter);
  }

  return {};
}

QVariant TraceTableModel::headerData(int section,
                                     Qt::Orientation orientation,
                                     int role) const {
  if (orientation != Qt::Horizontal || role != Qt::DisplayRole) {
    return {};
  }

  switch (section) {
    case kColumnFilename:
      return QStringLiteral("文件名");
    case kColumnDurationMs:
      return QStringLiteral("耗时");
    case kColumnSourcePath:
      return QStringLiteral("源路径");
    default:
      return {};
  }
}

void TraceTableModel::sort(int column, Qt::SortOrder order) {
  // 按指定列排序，默认升序；耗时列特殊处理为数值比较。
  std::sort(files_.begin(), files_.end(),
            [column, order](const FileSummary& a, const FileSummary& b) {
              int compare = 0;
              switch (column) {
                case kColumnFilename:
                  compare = a.filename.localeAwareCompare(b.filename);
                  break;
                case kColumnDurationMs:
                  if (a.total_duration_ms < b.total_duration_ms) compare = -1;
                  if (a.total_duration_ms > b.total_duration_ms) compare = 1;
                  break;
                case kColumnSourcePath:
                  compare = a.source_path.localeAwareCompare(b.source_path);
                  break;
                default:
                  break;
              }
              return (order == Qt::AscendingOrder) ? compare < 0 : compare > 0;
            });
}
