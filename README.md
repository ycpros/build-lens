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
```

| 目标 | 依赖 | 产物 |
|------|------|------|
| `BuildLens` | Qt6::Widgets + Core | 桌面 GUI |
| `BuildLensCLI` | Qt6::Core only | 命令行工具 |

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

DFS 遍历调用树，找出从根到叶子的最长耗时路径。
**合并连续同名节点**（如嵌套的多层 `DebugType`）。

```
ExecuteCompiler ─→ Frontend (83.9%) ─→ Source×12 ─→ ParseDecl ─→ ParseClass
```

### 2. HotspotAnalyzer — "哪类事最花时间？"

跨文件按事件名/类别聚合，输出 Top-N 热点。

```
hotspotsByCategory:
  source   ████████████████████████████████████████  50%  ← #include 是最大瓶颈
  parse    ████████████████████████████               26%
  driver   ██████████████████                         15%
  debug    ████                                       3%
  codegen  ███                                        3%
```

### 3. BottleneckDetector — "哪个文件不正常？"

统计均值 μ + 标准差 σ，超出 μ + 2σ 的文件标记为瓶颈。

---

## 📁 项目结构

```
build-lens/
├── CMakeLists.txt
├── README.md
├── docs/
│   ├── BuildLens 架构与愿景.md    # 完整架构设计文档
│   └── 商业分析.md                # 商业化路径分析
├── resources/
│   └── sample/                    # 示例 -ftime-trace JSON
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
```

---

## 🗺 路线图

```
v0.1 ✅ 桌面 GUI — 表格 + 搜索过滤
v0.2 ✅ CLI + 三分析器管线 — 关键路径/热点/瓶颈 + JSON 报告
v0.3 ⏳ 语义修正 — 独占耗时、Total 过滤、Source/header 热点
v0.4 ⏳ CI/CD 集成 — 多 session 对比、构建回归检测
v1.0 ⏳ Web Dashboard — 团队协作、AI 优化建议
```

---

## 📜 License

[GNU General Public License v3](LICENSE)
