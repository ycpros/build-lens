# Changelog

## v0.5.0 (unreleased)

- GUI: analysis panels — Critical Path, Hotspots (by name/category/source), Bottlenecks
- GUI: split layout with table + tabbed detail panels
- GUI: status bar analysis summary
- Core: per-file critical path preservation in AnalysisReport
- CLI: `--version` / `-V` short option
- CLI: `--quiet` / `-q` silent mode
- CLI: help text declares JSON output format

## v0.4.0 (2026-05-26)

- Unified AnalysisPipeline as shared CLI/GUI entry point
- Centralized versioning in BuildLensVersion.h
- Removed legacy TraceRecord; all consumers use AnalysisReport

## v0.3.0

- Semantic fixes: exclusive duration, Total event filtering
- sourceHotspots aggregation by header path
- Core semantic regression tests (8 tests)

## v0.2.0

- CLI tool with `--input`/`--output`/`--threshold`/`--top-n`
- Three-analyzer pipeline: CriticalPath, Hotspot, Bottleneck
- JSON report output (v0.2 schema)

## v0.1.0

- Desktop GUI with sortable, filterable file table
- Clang `-ftime-trace` JSON parsing
