# ============================================================
# BuildLens — C++ 编译性能观测工具
# CMakeLists.txt — 构建配置
# ============================================================

cmake_minimum_required(VERSION 3.16)
project(BuildLens VERSION 0.5.0 LANGUAGES CXX)

# ----- 编译标准 -----
# 使用 C++17。
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 如果用 Clang 编译，生成 -ftime-trace JSON，可用于 BuildLens 自测。
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ftime-trace")
endif()

# Qt MOC：自动调用 moc 处理 Q_OBJECT 宏。
set(CMAKE_AUTOMOC ON)

# ----- Qt 依赖查找 -----
# 通过 CMAKE_PREFIX_PATH 指定 Qt 安装路径，
# 例如：-DCMAKE_PREFIX_PATH=C:/Qt/5.15.2/msvc2019_64
set(CMAKE_PREFIX_PATH "D:/Software/Qt6.8.3/6.8.3/msvc2022_64")
find_package(Qt6 EXACT REQUIRED COMPONENTS Widgets Core)

# ----- 共享 core 源文件（桌面 UI 和 CLI 共用）-----
set(BUILDLENS_CORE_SOURCES
    # 版本信息
    src/core/BuildLensVersion.h

    # 数据模型
    src/core/FileSummary.h
    src/core/TraceParser.h
    src/core/TraceParser.cpp

    # v0.2 新增 — 事件与调用图
    src/core/TraceEvent.h
    src/core/TraceGraph.h
    src/core/TraceGraphBuilder.h
    src/core/TraceGraphBuilder.cpp

    # v0.2 新增 — 分析器
    src/core/CriticalPathAnalyzer.h
    src/core/CriticalPathAnalyzer.cpp
    src/core/HotspotAnalyzer.h
    src/core/HotspotAnalyzer.cpp
    src/core/BottleneckDetector.h
    src/core/BottleneckDetector.cpp

    # v0.2 新增 — 报告结构
    src/core/AnalysisReport.h
    src/core/AnalysisReport.cpp

    # v0.4 新增 — 统一分析管线
    src/core/AnalysisPipeline.h
    src/core/AnalysisPipeline.cpp
)

# ============================================================
# 目标 1：BuildLens（桌面 Qt GUI）
# ============================================================
add_executable(BuildLens
    src/main.cpp

    ${BUILDLENS_CORE_SOURCES}

    # model — Qt Model/View 适配层
    src/model/TraceTableModel.h
    src/model/TraceTableModel.cpp

    # ui — 界面层
    src/ui/MainWindow.h
    src/ui/MainWindow.cpp
    src/ui/CriticalPathPanel.h
    src/ui/CriticalPathPanel.cpp
    src/ui/HotspotPanel.h
    src/ui/HotspotPanel.cpp
    src/ui/BottleneckPanel.h
    src/ui/BottleneckPanel.cpp
)

target_include_directories(BuildLens PRIVATE src)
target_link_libraries(BuildLens Qt6::Widgets Qt6::Core)

# ============================================================
# 目标 2：BuildLensCLI（命令行工具，无 GUI 依赖）
# ============================================================
add_executable(BuildLensCLI
    src/cli/cli_main.cpp

    ${BUILDLENS_CORE_SOURCES}
)

target_include_directories(BuildLensCLI PRIVATE src)
target_link_libraries(BuildLensCLI Qt6::Core)

# ============================================================
# 目标 3：BuildLensCoreTests（核心语义测试）
# ============================================================
enable_testing()

add_executable(BuildLensCoreTests
    tests/core_tests.cpp

    ${BUILDLENS_CORE_SOURCES}
)

target_include_directories(BuildLensCoreTests PRIVATE src)
target_link_libraries(BuildLensCoreTests Qt6::Core)
target_compile_definitions(BuildLensCoreTests PRIVATE
    BUILDLENS_SOURCE_DIR="${CMAKE_SOURCE_DIR}"
)

add_test(NAME BuildLensCoreTests COMMAND BuildLensCoreTests)

add_test(
    NAME BuildLensCLIFixtureSmoke
    COMMAND BuildLensCLI
            --input "${CMAKE_SOURCE_DIR}/resources/BuildLensSample/src"
            --output "${CMAKE_BINARY_DIR}/buildlens-v0.5-fixture.json"
)
