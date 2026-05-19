**English** | [中文](README_CN.md)

# BuildLens 🔍

**C++ Compilation Performance Observability — from raw traces to actionable decisions**

[![license](https://img.shields.io/badge/license-GPLv3-blue)](LICENSE)
[![cpp](https://img.shields.io/badge/C%2B%2B-17-00599C)](https://en.cppreference.com/w/cpp/17)
[![qt](https://img.shields.io/badge/Qt-6.8-41CD52)](https://www.qt.io/)
[![platform](https://img.shields.io/badge/platform-Windows%20%7C%20Linux-lightgrey)]()

BuildLens parses Clang `-ftime-trace` JSON output (Chrome Trace Event format),
reconstructs the compilation call tree, runs three categories of analysis, and emits
a structured JSON report. It doesn't just tell you **which file is slowest** —
it answers **why it's slow, and what to optimize**.

---

## 🚀 Quick Start

### CLI (Recommended)

```powershell
# Compile your project with Clang (generates -ftime-trace JSON)
clang++ -ftime-trace -c your_code.cpp

# Analyze a build directory
.\BuildLensCLI.exe --input .\build-dir\ --output report.json

# Or print directly to stdout
.\BuildLensCLI.exe --input .\build-dir\
```

### Desktop GUI

```powershell
.\BuildLens.exe
# File → Open Directory → choose a build directory containing .json files
```

### Build

```bash
cmake -B cmake-build-debug -DCMAKE_PREFIX_PATH="D:/Software/Qt6.8.3/6.8.3/msvc2022_64"
cmake --build cmake-build-debug
ctest --test-dir cmake-build-debug --output-on-failure
```

| Target | Dependencies | Artifact |
|--------|-------------|----------|
| `BuildLens` | Qt6::Widgets + Core | Desktop GUI |
| `BuildLensCLI` | Qt6::Core only | CLI tool |
| `BuildLensCoreTests` | Qt6::Core only | Core semantic regression tests |

---

## 📊 Three Analyzers

```
   Clang -ftime-trace JSON
           │
           ▼
   TraceParser ──── parses all events (ph:"X" / ph:"b"/"e" pairs)
           │
           ▼
   TraceGraphBuilder ── stack-based algorithm → nested call tree
           │
    ┌──────┼──────┐
    ▼      ▼      ▼
Critical  Hotspot  Bottleneck
  Path
```

### 1. CriticalPathAnalyzer — "What to optimize?"

Wall-clock semantics: picks the root node with the largest inclusive duration,
then recursively picks the child with the largest inclusive duration at each level.
Parent and child durations are not summed — `totalDurationUs` equals the root node's
real wall-clock time. Reports `inclusiveDurationUs`, `exclusiveDurationUs`, and
`percentageOfRoot` for each node on the path.

```
ExecuteCompiler ─→ Frontend ─→ Source×13 ─→ ParseClass
```

### 2. HotspotAnalyzer — "What category costs the most?"

Aggregates by event name/category across all files, using exclusive duration by default.
Clang's built-in `Total ...` summary events are filtered out to avoid double-counting.
`sourceHotspots` aggregates `Source` events to specific header/source paths.

```
hotspotsByCategory:
  source       46%  ← #include / header processing
  parse        32%
  instantiate  10%

sourceHotspots:
  D:/Software/LLVM-22.1.0/.../immintrin.h
  D:/Software/Qt6.8.3/.../QtCore/qnamespace.h
```

### 3. BottleneckDetector — "Which files are outliers?"

Computes mean μ and standard deviation σ; files exceeding μ + 2σ are flagged as bottlenecks.

---

## ⚠️ v0.2 → v0.3 Compatibility

v0.3 introduces breaking schema changes to the CLI JSON report:

- `criticalPath.path[]` no longer outputs the old `durationUs` / `percentage`; replaced by
  `inclusiveDurationUs`, `exclusiveDurationUs`, `percentageOfRoot`.
- `criticalPath.totalDurationUs` now equals root node wall-clock time instead of summing parent+child.
- Hotspots use exclusive duration by default, tagged with `metric: "exclusiveDurationUs"`.
- Hotspot items use `durationUs`; the old `totalDurationUs` is removed.
- Clang `Total ...` summary events are filtered to avoid double-counting.
- New `sourceHotspots` section aggregates `Source` events by specific header/source path.
- `bottlenecks` now includes `thresholdMs`.

The desktop GUI remains a file-duration table. In-depth v0.3 analysis is driven by
the `BuildLensCLI` JSON report.

---

## 📁 Project Structure

```
build-lens/
├── CMakeLists.txt
├── CHANGELOG.md
├── README.md
├── resources/
│   ├── sample/                    # Simplified example -ftime-trace JSON
│   └── BuildLensSample/src/       # Real Clang fixture traces
└── src/
    ├── main.cpp                   # GUI entry point
    ├── cli/
    │   └── cli_main.cpp           # CLI entry point
    ├── core/                      # Data layer (no UI dependency)
    │   ├── TraceEvent.h           # Event data structures
    │   ├── TraceRecord.h          # Total-duration records
    │   ├── TraceParser.h/.cpp     # JSON parser
    │   ├── TraceGraph.h           # Call graph data structures
    │   ├── TraceGraphBuilder.h/.cpp  # Stack-based call tree construction
    │   ├── CriticalPathAnalyzer.h/.cpp # DFS longest path
    │   ├── HotspotAnalyzer.h/.cpp     # Hotspot aggregation
    │   ├── BottleneckDetector.h/.cpp  # Anomaly detection
    │   └── AnalysisReport.h/.cpp      # Unified JSON report
    ├── model/                     # Qt Model/View
    │   └── TraceTableModel.h/.cpp
    └── ui/                        # Qt GUI
        └── MainWindow.h/.cpp
└── tests/
    └── core_tests.cpp             # Core semantic + real-trace regression tests
```

---

## 🗺 Roadmap

```
v0.1 ✅ Desktop GUI — sortable/filterable table
v0.2 ✅ CLI + three-analyzer pipeline — critical path/hotspot/bottleneck + JSON report
v0.3 ✅ Semantic fixes — exclusive duration, Total filtering, source/header hotspots
v0.4 ⏳ CI/CD integration — multi-session comparison, build regression detection
v1.0 ⏳ Web Dashboard — team collaboration, AI optimization suggestions
```

---

## 📜 License

[GNU General Public License v3](LICENSE)
