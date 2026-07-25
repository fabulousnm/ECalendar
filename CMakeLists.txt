cmake_minimum_required(VERSION 3.14)
project(ECalendar VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)   # Qt MOC 自动处理（Q_OBJECT 宏）

# ────────────────────────────────────────────────────────────
# 依赖检测
# ────────────────────────────────────────────────────────────
find_package(Qt5 REQUIRED COMPONENTS Widgets)

# ────────────────────────────────────────────────────────────
# 头文件路径
# src/ 作为根目录，代码中的 #include "core/..." / "gui/..." 可以正常解析
# ────────────────────────────────────────────────────────────
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)

# ────────────────────────────────────────────────────────────
# 核心模块（无 GUI、无 Qt 依赖）
# ────────────────────────────────────────────────────────────
set(CORE_SOURCES
    src/core/Storage.cpp
    src/core/Task.cpp
    src/core/TaskManager.cpp
    src/core/User.cpp
    src/util/Hash.cpp
)

# ────────────────────────────────────────────────────────────
# GUI 模块（依赖 Qt5::Widgets）
# ────────────────────────────────────────────────────────────
set(GUI_SOURCES
    src/gui/main_gui.cpp
    src/gui/MainWindow.cpp
    src/gui/LoginDialog.cpp
    src/gui/TaskDialog.cpp
    src/gui/ReminderPopup.cpp
    ${CORE_SOURCES}
)

# ────────────────────────────────────────────────────────────
# 目标 1：ecalendar —— GUI 主程序
# ────────────────────────────────────────────────────────────
add_executable(ecalendar ${GUI_SOURCES})
target_link_libraries(ecalendar
    Qt5::Widgets
    pthread
)

# 编译定制定位在 build/ecalendar，与 style.qss 路径一致
set_target_properties(ecalendar PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
)

# 拷贝 style.qss 到可执行文件同级目录
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/src/gui/style.qss"
    "${CMAKE_BINARY_DIR}/style.qss"
    COPYONLY
)

# ════════════════════════════════════════════════════════════
# ⚠ 已知问题（代码中存在但 CMake 无法自动修复）：
#
# 1. main_gui.cpp 第 14 行 #include "audio/SpeechRecognizer.h"
#    → src/audio/ 目录不存在，编译会报 "No such file or directory"
#    → 解决方案：创建 src/audio/SpeechRecognizer.h（可以放空头文件），
#      或者注释掉这一行
#
# 2. MainWindow.cpp 构造函数中调用了 Reminder::init()
#    → 未找到 Reminder 类的定义，编译报错
#    → 解决方案：补充 Reminder 类的声明与实现
#
# 3. MainWindow.h 缺少 m_recordState、m_tempWavPath、m_hasArecord、
#    m_stopFeeding、m_nlInput、m_aiAddBtn、m_recordBtn 等成员声明
#    → 链接时会报未定义变量错误
#
# 以上问题到编写 CMakeLists.txt 时仍未解决，需要用户自行补充遗漏的代码。
# ════════════════════════════════════════════════════════════

# ────────────────────────────────────────────────────────────
# 目标 2：test —— 命令行单元测试（无 GUI、无 Qt）
# ────────────────────────────────────────────────────────────
add_executable(test
    src/test.cpp
    ${CORE_SOURCES}
)
target_link_libraries(test pthread)

# ────────────────────────────────────────────────────────────
# 目标 3：test_hash —— SHA256 哈希测试工具（命令行）
# ────────────────────────────────────────────────────────────
add_executable(test_hash
    src/util/test_hash.cpp
    src/util/Hash.cpp
)

# ────────────────────────────────────────────────────────────
# 打印编译器信息
# ────────────────────────────────────────────────────────────
message(STATUS "Qt5 include: ${Qt5Widgets_INCLUDE_DIRS}")
message(STATUS "Qt5 libs:    ${Qt5Widgets_LIBRARIES}")
message(STATUS "")
message(STATUS "Targets:")
message(STATUS "  ecalendar  — GUI 主程序")
message(STATUS "  test       — 命令行单元测试")
message(STATUS "  test_hash  — SHA256 哈希测试")
