# ============================================================
# BuildLens — C++ 编译性能观测工具
# CMakeLists.txt — 构建配置
# ============================================================

cmake_minimum_required(VERSION 3.16)
project(BuildLens VERSION 0.1.0 LANGUAGES CXX)

# ----- 编译标准 -----
# 使用 C++17，Qt 5.15.2 良好支持
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Qt MOC：自动调用 moc 处理 Q_OBJECT 宏，
# 无需手动运行 moc 或将 moc_*.cpp 加入源文件列表。
set(CMAKE_AUTOMOC ON)

# ----- Qt 依赖查找 -----
# 固定 Qt 5.15.2 版本，版本不匹配时 CMake 配置直接报错。
# 在 CLion/VS2022 中通过 CMAKE_PREFIX_PATH 指定 Qt 安装路径，
# 例如：-DCMAKE_PREFIX_PATH=C:/Qt/5.15.2/msvc2019_64/lib/cmake
find_package(Qt5 5.15.2 EXACT REQUIRED COMPONENTS Widgets Core)

# ----- 可执行目标 -----
add_executable(BuildLens
    # 程序入口
    src/main.cpp

    # core — 数据层
    # TraceParser: 解析 Clang -ftime-trace JSON 文件
    # TraceRecord: 单条编译记录的数据结构
    src/core/TraceParser.h
    src/core/TraceParser.cpp
    src/core/TraceRecord.h

    # model — Qt Model/View 适配层
    # TraceTableModel: 将 TraceRecord 列表适配为 QTableView 可用格式
    src/model/TraceTableModel.h
    src/model/TraceTableModel.cpp

    # ui — 界面层
    # MainWindow: 主窗口，包含菜单、表格、搜索框
    src/ui/MainWindow.h
    src/ui/MainWindow.cpp
)

# 头文件搜索路径：所有源文件可通过 #include "core/TraceRecord.h" 等方式引用
target_include_directories(BuildLens PRIVATE src)

# 链接 Qt Widgets 和 Qt Core 模块
target_link_libraries(BuildLens Qt5::Widgets Qt5::Core)
