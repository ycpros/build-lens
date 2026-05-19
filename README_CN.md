[English](README.md) | **中文**

# BuildLens 🔍

**C++ 编译性能观测工具 — 从原始 trace 到可执行决策**

[![license](https://img.shields.io/badge/license-GPLv3-blue)](LICENSE)
[![cpp](https://img.shields.io/badge/C%2B%2B-17-00599C)](https://en.cppreference.com/w/cpp/17)
[![qt](https://img.shields.io/badge/Qt-6.8-41CD52)](https://www.qt.io/)
[![platform](https://img.shields.io/badge/platform-Windows%20%7C%20Linux-lightgrey)]()

BuildLens 解析 Clang `-ftime-trace` 输出的 JSON（Chrome Trace Event 格式），
重建编译调用树，执行三类分析，输出结构化 JSON 报告。
不仅告诉你**哪个文件最慢**，还能回答**为什么慢，应该优化什么**。

---

## 🚀 快速开始

### CLI（推荐）

```powershell
# 用 Clang 编译你的项目（生成 -ftime-trace JSON）
clang++ -ftime-trace -c your_code.cpp

# 分析构建目录
.\BuildLensCLI.exe --input .\build-dir\ --output report.json

# 或直接输出到 stdout
.\BuildLensCLI.exe --input .\build-dir\
```

### 桌面 GUI

```powershell
.\BuildLens.exe
# 文件 → 打开目录 → 选择包含 .json 的构建目录
```

### 构建

```bash
cmake -B cmake-build-debug -DCMAKE_PREFIX_PATH="D:/Software/Qt6.8.3/6.8.3/msvc2022_64"
cmake --build cmake-build-debug
ctest --test-dir cmake-build-debug --output-on-failure
```

| 目标 | 依赖 | 产物 |
|------|------|------|
| `BuildLens` | Qt6::Widgets + Core | 桌面 GUI |
| `BuildLensCLI` | Qt6::Core only | 命令行工具 |
| `BuildLensCoreTests` | Qt6::Core only | 核心语义回归测试 |

---

## 📊 三类分析器

```
   Clang -ftime-trace JSON
           │
           ▼
   TraceParser ──── 解析全部事件（ph:"X" / ph:"b"/"e" 对）
           │
           ▼
   TraceGraphBuilder ── 栈算法 → 嵌套调用树
           │
    ┌──────┼──────┐
    ▼      ▼      ▼
 关键路径  热点    瓶颈
```

### 1. CriticalPathAnalyzer — "优化什么？"

使用墙钟语义：选择 inclusive 耗时最大的根节点，
再逐层选择 inclusive 耗时最大的子节点。
父子耗时不再相加，`totalDurationUs` 等于根节点真实耗时。
报告同时输出 `inclusiveDurationUs`、`exclusiveDurationUs` 和 `percentageOfRoot`。

```
ExecuteCompiler ─→ Frontend ─→ Source×13 ─→ ParseClass
```

### 2. HotspotAnalyzer — "哪类事最花时间？"

跨文件按事件名/类别聚合，默认使用 exclusive 耗时。
Clang 自带的 `Total ...` 汇总事件会被过滤，避免重复计数。
`sourceHotspots` 会把 `Source` 事件聚合到具体 header/source 路径。

```
hotspotsByCategory:
  source       46%  ← #include / header 处理
  parse        32%
  instantiate  10%

sourceHotspots:
  D:/Software/LLVM-22.1.0/.../immintrin.h
  D:/Software/Qt6.8.3/.../QtCore/qnamespace.h
```

### 3. BottleneckDetector — "哪个文件不正常？"

统计均值 μ + 标准差 σ，超出 μ + 2σ 的文件标记为瓶颈。

---

## ⚠️ v0.2 → v0.3 兼容性

v0.3 的 CLI JSON 报告有破坏性 schema 变化：

- `criticalPath.path[]` 不再输出旧的 `durationUs` / `percentage`，改为
  `inclusiveDurationUs`、`exclusiveDurationUs`、`percentageOfRoot`。
- `criticalPath.totalDurationUs` 现在等于根节点墙钟耗时，不再把父子耗时相加。
- 热点默认使用 exclusive 耗时，结果带 `metric: "exclusiveDurationUs"`。
- 热点项使用 `durationUs`，不再输出旧的 `totalDurationUs`。
- Clang `Total ...` 汇总事件会被过滤，避免重复计数。
- 新增 `sourceHotspots`，按具体 header/source 路径聚合 `Source` 事件。
- `bottlenecks` 新增 `thresholdMs`。

桌面 GUI 仍是文件耗时表格；v0.3 深度分析以 `BuildLensCLI` JSON 报告为准。

---

## 📁 项目结构

```
build-lens/
├── CMakeLists.txt
├── CHANGELOG.md
├── README.md
├── resources/
│   ├── sample/                    # 简化示例 -ftime-trace JSON
│   └── BuildLensSample/src/       # 真实 Clang fixture trace
└── src/
    ├── main.cpp                   # GUI 入口
    ├── cli/
    │   └── cli_main.cpp           # CLI 入口
    ├── core/                      # 数据层（无 UI 依赖）
    │   ├── TraceEvent.h           # 事件数据结构
    │   ├── TraceRecord.h          # 总耗时记录
    │   ├── TraceParser.h/.cpp     # JSON 解析器
    │   ├── TraceGraph.h           # 调用图数据结构
    │   ├── TraceGraphBuilder.h/.cpp  # 栈算法构建调用树
    │   ├── CriticalPathAnalyzer.h/.cpp # DFS 最长路径
    │   ├── HotspotAnalyzer.h/.cpp     # 热点聚合
    │   ├── BottleneckDetector.h/.cpp  # 异常检测
    │   └── AnalysisReport.h/.cpp      # 统一 JSON 报告
    ├── model/                     # Qt Model/View
    │   └── TraceTableModel.h/.cpp
    └── ui/                        # Qt GUI
        └── MainWindow.h/.cpp
└── tests/
    └── core_tests.cpp             # 核心语义与真实 trace 回归测试
```

---

## 🗺 路线图

```
v0.1 ✅ 桌面 GUI — 表格 + 搜索过滤
v0.2 ✅ CLI + 三分析器管线 — 关键路径/热点/瓶颈 + JSON 报告
v0.3 ✅ 语义修正 — 独占耗时、Total 过滤、Source/header 热点
v0.4 ⏳ CI/CD 集成 — 多 session 对比、构建回归检测
v1.0 ⏳ Web Dashboard — 团队协作、AI 优化建议
```

---

## 📜 License

[GNU General Public License v3](LICENSE)
