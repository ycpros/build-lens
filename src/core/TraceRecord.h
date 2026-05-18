// ============================================================
// BuildLens — C++ 编译性能观测工具
//
// TraceRecord.h
// 功能：定义单条编译记录的数据结构。
//       每个 TraceRecord 对应一个源文件的 -ftime-trace 分析结果。
// ============================================================

#ifndef BUILD_LENS_TRACERECORD_H_
#define BUILD_LENS_TRACERECORD_H_

#include <QString>

// 单条编译记录。
// 对应一个 .cpp 文件经 Clang -ftime-trace 输出的 JSON，
// 存储解析后的关键字段供后续排序和展示。
struct TraceRecord {
  // 源文件名，不含路径，例如 "renderer.cpp"。
  QString filename;

  // 该文件的编译总耗时，单位毫秒。
  // 取自 JSON 中 cat=="phase" && name=="Total" 的 dur 字段（微秒转毫秒）。
  double total_duration_ms = 0.0;

  // JSON 文件的完整路径，用于双击打开或显示来源。
  QString source_path;
};

#endif  // BUILD_LENS_TRACERECORD_H_
