# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build Commands

```bash
# Configure (Qt path is hardcoded in CMakeLists.txt; adjust if your Qt differs)
cmake -B cmake-build-debug -DCMAKE_PREFIX_PATH="D:/Software/Qt6.8.3/6.8.3/msvc2022_64"

# Build
cmake --build cmake-build-debug

# The project already has a pre-configured cmake-build-debug directory.
# compile_commands.json is at cmake-build-debug/compile_commands.json.
```

## Architecture

**BuildLens** is a Qt6 desktop app that parses Clang `-ftime-trace` JSON output and displays per-file compile durations in a sortable, filterable table.

Three layers under `src/`:

| Layer | Directory | Role |
|-------|-----------|------|
| Core | `src/core/` | Data structures (`TraceRecord`) and JSON parsing (`TraceParser`) — no Qt UI dependencies |
| Model | `src/model/` | `TraceTableModel` adapts `std::vector<TraceRecord>` to `QAbstractTableModel` for Qt's Model/View |
| UI | `src/ui/` | `MainWindow` (QMainWindow) — menu bar, search box, QTableView, status bar |

Data flow: `*.cpp.json` files → `TraceParser::ParseDirectory()` → `std::vector<TraceRecord>` → `TraceTableModel` → `QSortFilterProxyModel` (case-insensitive filename filter) → `QTableView`

## Key Implementation Details

- **JSON parsing**: Looks for `ph=="X" && name=="ExecuteCompiler"` in the `traceEvents` array (Chrome Trace Event format). The `dur` field is in microseconds; code divides by 1000 to get milliseconds.
- **Sample data** in `resources/sample/` uses a simplified format (`cat: "phase"`, `name: "Total"`) that does **not** match the real parser logic. Real Clang output uses `ph: "X"`. The samples are for UI demo only.
- **Recursive scanning**: `QDirIterator::Subdirectories` — a single directory pick loads all `.json` files from the entire subdirectory tree.
- **Default sort**: descending by duration, applied in `SetRecords()` before `endResetModel()`.
- **Status bar stats** computed from visible (post-filter) rows via `proxy_model_->mapToSource()`.
- **Signal wire-up**: `QLineEdit::textChanged` → `OnFilterChanged` → `proxy_model_->setFilterFixedString()` + `UpdateStatusBar()`.
- **Column alignment**: The duration column uses `Qt::AlignRight | Qt::AlignVCenter` via `Qt::TextAlignmentRole`.
- Hardcoded Qt path in `CMakeLists.txt` line 26 — change this if building on a different machine.
