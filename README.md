<!-- ========================================================
  BuildLens — C++ Compile Performance Observatory
  README.md — v0.1.0
  ======================================================== -->

<br />
<div align="center">

# BuildLens 🔍

**C++ 编译性能观测工具**

一眼看出哪个文件在拖慢你的构建速度

![license](https://img.shields.io/badge/license-GPLv3-blue)
![cpp](https://img.shields.io/badge/C%2B%2B-17-00599C)
![qt](https://img.shields.io/badge/Qt-5.15.2-41CD52)
![platform](https://img.shields.io/badge/platform-Windows%20%7C%20Linux-lightgrey)

---

</div>

<br />

## 📖 概述

BuildLens 是一个桌面 GUI 工具，解析 **Clang `-ftime-trace`** 输出的 JSON 文件（Chrome Trace Event 格式），以排序表格展示每个 `.cpp` 文件的编译耗时，帮助你快速定位性能瓶颈。

<br />

## 🚀 快速开始

### 环境要求

| 组件 | 版本 |
|------|------|
| 编译器 | MSVC 2019/2022（Windows）、Clang、GCC |
| Qt | **5.15.2**（msvc2019_64 套件） |
| CMake | ≥ 3.16 |
| C++ 标准 | C++17 |

### 构建

```bash
# 在 CLion 或 VS2022 中打开项目根目录

# 配置时的关键变量（Qt 安装路径）：
#   -DCMAKE_PREFIX_PATH=C:/Qt/5.15.2/msvc2019_64/lib/cmake
#
# 注意：若用 CMake 命令行：
cmake -B build -DCMAKE_PREFIX_PATH="C:/Qt/5.15.2/msvc2019_64/lib/cmake"
cmake --build build

# CMakeLists.txt 中已固定 Qt 5.15.2 版本，版本不匹配会报错
```

### 使用流程

```
① 用 Clang 编译项目时添加 -ftime-trace 参数
    ↓
② 每个 .cpp 会生成同名 .json 文件（如 main.cpp.json）
    ↓
③ 打开 BuildLens → 文件 → 打开目录…（Ctrl+O）
    ↓
④ 选择包含 .json 的目录（支持递归扫描子目录）
    ↓
⑤ 表格自动按耗时降序排列
```

### 示例数据

项目内附测试数据，无需自己编译即可试用：

```
resources\sample\         ← 8 个模拟的 -ftime-trace JSON 文件
  ├── renderer.cpp.json       (38.2 s)
  ├── ai_system.cpp.json      (31.7 s)
  ├── physics_world.cpp.json  (22.1 s)
  ├── ui_dashboard.cpp.json   (18.5 s)
  ├── shader_compiler.cpp.json(28.0 s)
  ├── ecs_framework.cpp.json  (99.0 s)
  ├── math_utils.cpp.json     (9.3 s)
  └── logger.cpp.json         (4.0 s)
```

> **注意**：示例数据使用简化格式仅作界面演示，与真实 Clang 输出的 Chrome Trace Event 格式不同。真实数据还包含 pid/tid/ts/ph 等字段及更细粒度的编译阶段事件。

<br />

## 🏗 架构

```
┌─────────────────────────────────────────────────────────────┐
│ Layer 1: 数据源 (Data Source)                                │
│                                                             │
│  Clang -ftime-trace  →  *.cpp.json  (Chrome Trace Event)    │
│                                                             │
│  扫描方式: QDirIterator + Subdirectories (递归子目录)         │
└──────────────────────────┬──────────────────────────────────┘
                           │
                           ▼
┌─────────────────────────────────────────────────────────────┐
│ Layer 2: 核心层 (Core Layer)                                 │
│  src/core/                                                  │
│                                                             │
│  TraceParser            TraceRecord                         │
│  ┌──────────────┐      ┌──────────────────┐                 │
│  │ ParseDirectory│ ──→ │ filename         │                 │
│  │ ParseFile    │      │ total_duration_ms│                 │
│  │ InferSourceName│    │ source_path      │                 │
│  └──────────────┘      └──────────────────┘                 │
│                                                             │
│  解析方式: QJsonDocument 解析 traceEvents 数组               │
│  匹配 ph=="X" && name=="ExecuteCompiler" → dur(μs → ms)    │
└──────────────────────────┬──────────────────────────────────┘
                           │ std::vector<TraceRecord>
                           ▼
┌─────────────────────────────────────────────────────────────┐
│ Layer 3: 模型层 (Model Layer)                                │
│  src/model/                                                 │
│                                                             │
│  TraceTableModel              QSortFilterProxyModel         │
│  ┌──────────────────────┐    ┌────────────────────────┐    │
│  │ QAbstractTableModel  │    │ setFilterFixedString() │    │
│  │ 3 列: 文件名/耗时/路径│ ←─│ setFilterKeyColumn(0)  │    │
│  │ 默认按耗时降序       │    │ 忽略大小写模糊匹配      │    │
│  └──────────────────────┘    └────────────────────────┘    │
└──────────────────────────┬──────────────────────────────────┘
                           │ setModel(proxy_model_)
                           ▼
┌─────────────────────────────────────────────────────────────┐
│ Layer 4: 界面层 (UI Layer)                                   │
│  src/ui/                                                    │
│                                                             │
│  MainWindow (QMainWindow)                                   │
│  ┌──────────────────────────────────────────────────────┐   │
│  │ [文件(&F)] 打开目录… (Ctrl+O) 退出(&Q)                │   │
│  ├──────────────────────────────────────────────────────┤   │
│  │ 过滤： [  搜索文件名…          ]                     │   │
│  ├──────────────────────────────────────────────────────┤   │
│  │ 文件名          │ 耗时    │ 源路径           (点击列头排序)│
│  │ ────────────────┼─────────┼─────────────────────── │   │
│  │ ecs_framework   │ 99.0 s  │ D:/build/.../ecs...   │   │
│  │ renderer        │ 38.2 s  │ D:/build/.../ren...   │   │
│  │ ai_system       │ 31.7 s  │ D:/build/.../ai_...   │   │
│  │ ...             │         │                       │   │
│  ├──────────────────────────────────────────────────────┤   │
│  │ 状态栏: 文件：8 | 总耗时：250.6 s | 平均：31.33 s     │   │
│  └──────────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────────┘
```

[查看完整 SVG 架构图 →](build-lens-architecture.html)

<br />

## ✨ 功能清单

### ✅ v0.1 MVP（当前版本）

| 功能 | 实现 | 说明 |
|------|------|------|
| 解析 Clang `-ftime-trace` JSON | ✅ | Chrome Trace Event 格式，匹配 `ph=="X"` + `name=="ExecuteCompiler"` |
| 递归扫描子目录 | ✅ | 使用 `QDirIterator::Subdirectories`，支持嵌套构建目录 |
| 编译耗时排序表格 | ✅ | 3 列：文件名、耗时（自动格式化 ms/s）、源路径 |
| 默认按耗时降序排列 | ✅ | `SetRecords()` 内部自动调用 `sort(kColumnDurationMs, Descending)` |
| 列头点击切换排序 | ✅ | 点击任意列头切换升序/降序，文件名/路径按 locale 字典序 |
| 按文件名实时搜索过滤 | ✅ | `QSortFilterProxyModel::setFilterFixedString()`，忽略大小写模糊匹配 |
| 状态栏统计 | ✅ | 格式：`文件：N \| 总耗时：X.X s \| 平均：X.XX s`，随过滤实时更新 |
| 目录选择对话框 | ✅ | `QFileDialog::getExistingDirectory()` + Ctrl+O 快捷键 |
| 加载中等待光标 | ✅ | `setOverrideCursor(Qt::WaitCursor)` + 空数据弹窗提示 |
| 交替行颜色 | ✅ | `setAlternatingRowColors(true)` |
| 单行选中 | ✅ | `SelectRows` + `SingleSelection` |

### ⏳ 未实现（不在 v0.1 范围内）

| 功能 | 说明 |
|------|------|
| ❌ 拖放文件夹 | 未实现 `dragEnterEvent` / `dropEvent` |
| ❌ 双击展开详情 | 未实现各编译阶段的耗时分解 |
| ❌ 依赖图可视化 | 待 v0.3 |
| ❌ 头文件开销分析 | 待 v0.3 |
| ❌ CI 集成报告 | 待 v1.0 |
| ❌ 深色主题 | 系统默认 |

<br />

## 📁 项目结构

```
build-lens/
├── CMakeLists.txt                 # Qt5 + C++17 + AUTOMOC 构建配置
├── README.md
├── build-lens-architecture.html   # SVG 架构图
├── LICENSE                        # GPL v3
├── resources/
│   └── sample/                    # 测试用 -ftime-trace JSON 数据
└── src/
    ├── main.cpp                   # 入口：QApplication + MainWindow 启动
    ├── core/                      # 数据层（不依赖 Qt 以外的 UI 组件）
    │   ├── TraceRecord.h          # 数据结构：filename / duration_ms / source_path
    │   ├── TraceParser.h          # JSON 解析器接口
    │   └── TraceParser.cpp        # 解析 Chrome Trace Event 格式
    ├── model/                     # Qt Model/View 适配层
    │   ├── TraceTableModel.h      # QAbstractTableModel 3 列接口
    │   └── TraceTableModel.cpp    # 排序、格式化、数据映射
    └── ui/                        # 界面层
        ├── MainWindow.h           # 主窗口接口
        └── MainWindow.cpp         # 菜单栏 / 表格 / 搜索 / 状态栏
```

<br />

## 📐 关键技术细节

### JSON 解析

实际 Clang `-ftime-trace` 输出使用 **Chrome Trace Event 格式**（与 `chrome://tracing` 兼容）：

```json
{
  "traceEvents": [
    {"ph": "X", "name": "ExecuteCompiler", "dur": 587188, "ts": 13, "pid": 31576, "tid": 12640, ...},
    {"ph": "X", "name": "Frontend", "dur": 557164, ...},
    {"ph": "X", "name": "Backend", "dur": 18159, ...},
    {"ph": "X", "name": "ParseClass", "dur": 182067, "args": {"detail": "std::pair"}},
    {"ph": "X", "name": "InstantiateFunction", "dur": 50877, ...}
  ]
}
```

解析提取逻辑：

```
遍历 traceEvents 数组
  → 过滤 ph=="X" && name=="ExecuteCompiler"
    → 读取 dur 字段（微秒）
      → 除以 1000 转为毫秒
        → 写入 TraceRecord.total_duration_ms
```

### 排序策略

| 列 | 排序依据 | 说明 |
|----|---------|------|
| 文件名 | `QString::localeAwareCompare()` | 按当前 locale 字典序 |
| 耗时 | `double` 直接比较 | 数字大小比较，不受格式化影响 |
| 源路径 | `QString::localeAwareCompare()` | 按当前 locale 字典序 |

### 状态栏统计

```
文件：N | 总耗时：X.X s | 平均：X.XX s
         ↑ float 1位    ↑ float 2位
```

- 统计基于**过滤后可见**的数据，随搜索实时更新
- 通过 `proxy_model_->mapToSource()` 从代理模型反向映射到源数据

<br />

## 🛣 路线图

```
v0.1 ─── ✅ 解析 + 表格 + 搜索过滤（当前）
 │
 ├── v0.2 ─── ⏳ 展开各编译阶段耗时详情
 │             双击文件 → 弹出 Parse/Instantiate/CodeGen 分解
 │
 ├── v0.3 ─── ⏳ 头文件开销分析 + 依赖树
 │
 ├── v0.4 ─── ⏳ 编译时间趋势追踪（多 session 对比）
 │
 └── v1.0 ─── ⏳ CI 集成 + Pro 版本
```

<br />

## 📜 LICENSE

[GNU General Public License v3](LICENSE)
