# @file CMakeLists.txt
# @brief Ecalender 项目构建配置（CMake）
#
# 使用 CMake 构建系统管理项目的编译。
# 项目采用模块化结构：核心逻辑编译为静态库 ecore，
# CLI 应用程序链接此静态库。
#
# 构建步骤：
#   mkdir build && cd build
#   cmake .. -G "MinGW Makefiles"
#   mingw32-make
#
# 可选：启用 Qt GUI（需要安装 Qt5）：
#   cmake .. -DENABLE_QT=ON
#
# 可选：启用 DeepSeek API（需要安装 libcurl）：
#   sudo apt install libcurl4-openssl-dev  (WSL/Linux)
#   cmake .. -DENABLE_CURL=ON

cmake_minimum_required(VERSION 3.10)
project(Ecalender LANGUAGES CXX)

# 使用 C++17 标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# --- 可选模块：libcurl（用于 DeepSeek API 调用）---
option(ENABLE_CURL "Enable DeepSeek API via libcurl" ON)
if(ENABLE_CURL)
    find_package(CURL QUIET)
endif()

# --- 源文件分组 ---
# 核心模块：任务管理、用户管理、数据序列化
set(CORE_SOURCES
    src/core/Task.cpp
    src/core/User.cpp
    src/core/Storage.cpp
    src/core/TaskManager.cpp
)

# 工具模块：SHA256 哈希、自然语言处理器
set(UTIL_SOURCES
    src/util/Hash.cpp
    src/util/NLProcess.cpp
)

# 音频模块：提醒播放、语音识别、麦克风录音
set(AUDIO_SOURCES
    src/audio/Reminder.cpp
    src/audio/SpeechRecognizer.cpp
    src/audio/AudioRecoder.cpp
)


# --- 构建核心静态库 ---
# 将核心逻辑和工具编译为静态库，供 CLI 和 GUI 共同使用
# Vosk 第三方库路径
set(VOSK_DIR "${CMAKE_SOURCE_DIR}/third_party/vosk")

# 构建核心静态库
add_library(ecore STATIC ${CORE_SOURCES} ${UTIL_SOURCES} ${AUDIO_SOURCES})
target_include_directories(ecore PUBLIC src ${VOSK_DIR})

# 仅在 vosk 库存在时链接（Linux/WSL 编译环境）
if(EXISTS "${VOSK_DIR}/libvosk.so")
    target_link_directories(ecore PUBLIC ${VOSK_DIR})
    target_link_libraries(ecore PUBLIC vosk pthread)
endif()

# libcurl 支持（用于 DeepSeek API 调用）
# 需要先安装 libcurl4-openssl-dev
if(ENABLE_CURL AND CURL_FOUND)
    target_link_libraries(ecore PUBLIC CURL::libcurl)
    target_compile_definitions(ecore PUBLIC HAS_CURL=1)
    message(STATUS "libcurl found: DeepSeek API enabled")
else()
    target_compile_definitions(ecore PUBLIC HAS_CURL=0)
    if(ENABLE_CURL)
        message(STATUS "libcurl not found: DeepSeek API disabled (install libcurl4-openssl-dev to enable)")
    endif()
endif()

# --- 可选：Qt GUI 构建 ---
# 启用后需要安装 Qt5（Widgets + Multimedia 模块）
# Windows 可通过在线安装器或 vcpkg 安装
option(ENABLE_QT "Build Qt GUI" ON)
# 自动链接模型目录到构建目录（方便运行时通过相对路径 model/ 访问）
if(NOT EXISTS "${CMAKE_BINARY_DIR}/model")
    file(CREATE_LINK "${CMAKE_SOURCE_DIR}/model" "${CMAKE_BINARY_DIR}/model" SYMBOLIC)
endif()
# 如果 build/ 不在项目根目录，同时创建项目根目录的快捷链接
get_filename_component(PROJ_ROOT "${CMAKE_SOURCE_DIR}" DIRECTORY)
if(NOT "${CMAKE_BINARY_DIR}" STREQUAL "${CMAKE_SOURCE_DIR}/build")
    if(NOT EXISTS "${CMAKE_SOURCE_DIR}/build/model")
        file(CREATE_LINK "${CMAKE_SOURCE_DIR}/model" "${CMAKE_SOURCE_DIR}/build/model" SYMBOLIC)
    endif()
endif()

if(ENABLE_QT)
    # 启用 Qt MOC（元对象编译器），自动处理 Q_OBJECT 宏
    set(CMAKE_AUTOMOC ON)
    # 启用 Qt UIC（UI编译器）和 RCC（资源编译器），后续扩展使用
    set(CMAKE_AUTOUIC ON)
    set(CMAKE_AUTORCC ON)

    find_package(Qt5 COMPONENTS Widgets Multimedia REQUIRED)
    set(GUI_SOURCES
        src/gui/main_gui.cpp
        src/gui/LoginDialog.cpp
        src/gui/MainWindow.cpp
        src/gui/TaskDialog.cpp
        src/gui/ReminderPopup.cpp
    )
    set(GUI_HEADERS
        src/gui/LoginDialog.h
        src/gui/MainWindow.h
        src/gui/TaskDialog.h
        src/gui/ReminderPopup.h
    )
    add_executable(ecalender_gui ${GUI_SOURCES} ${GUI_HEADERS})
    target_include_directories(ecalender_gui PRIVATE src)
    if(EXISTS "${VOSK_DIR}/libvosk.so")
        target_link_libraries(ecalender_gui PRIVATE ecore Qt5::Widgets Qt5::Multimedia vosk pthread m)
    else()
        target_link_libraries(ecalender_gui PRIVATE ecore Qt5::Widgets Qt5::Multimedia)
    endif()
    if(WIN32)
        # Windows 下复制样式表到构建目录
        configure_file(src/gui/style.qss ${CMAKE_BINARY_DIR}/style.qss COPYONLY)
    endif()
endif()

