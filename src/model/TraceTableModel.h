// ============================================================
// BuildLens — C++ 编译性能观测工具
//
// TraceTableModel.h
// 功能：将 TraceRecord 列表适配为 QAbstractTableModel，
//       供 QTableView 展示、排序和过滤。
// ============================================================

#ifndef BUILD_LENS_TRACETABLEMODEL_H_
#define BUILD_LENS_TRACETABLEMODEL_H_

#include <QAbstractTableModel>
#include <QString>
#include <vector>

#include "core/TraceRecord.h"

// 编译记录表格模型。
//
// 列定义：
//   0 — 文件名（QString）
//   1 — 耗时（double，单位毫秒）
//   2 — 源路径（QString）
//
// 默认按耗时降序排列。
class TraceTableModel : public QAbstractTableModel {
  Q_OBJECT

 public:
  // 表格列索引。
  enum Column {
    kColumnFilename = 0,
    kColumnDurationMs,
    kColumnSourcePath,
    kColumnCount  // 总列数
  };

  explicit TraceTableModel(QObject* parent = nullptr);

  // 替换全部数据并重置模型。
  void SetRecords(const std::vector<TraceRecord>& records);

  // 返回当前持有的所有记录，供界面层计算统计信息。
  const std::vector<TraceRecord>& records() const { return records_; }

  // ----- QAbstractTableModel 接口实现 -----
  int rowCount(const QModelIndex& parent = QModelIndex()) const override;
  int columnCount(const QModelIndex& parent = QModelIndex()) const override;
  QVariant data(const QModelIndex& index,
                int role = Qt::DisplayRole) const override;
  QVariant headerData(int section, Qt::Orientation orientation,
                      int role = Qt::DisplayRole) const override;
  void sort(int column, Qt::SortOrder order = Qt::DescendingOrder) override;

 private:
  std::vector<TraceRecord> records_;
};

#endif  // BUILD_LENS_TRACETABLEMODEL_H_
