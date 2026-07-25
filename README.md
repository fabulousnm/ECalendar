# ECalendar — 智能日程管理器

> 一个功能丰富的日程管理桌面应用，支持语音输入、AI 自然语言解析和离线语音识别。
>
> 计算机编程实践课程大作业。

---

## ✨ 功能特性

- **🔐 用户管理** — 登录 / 注册，密码经 SHA256 哈希加密存储
- **📋 任务管理** — 增删改查，含任务名称、时间、优先级、分类、提醒时间
- **📅 天 / 月视图切换** — 单日和月度的任务概览
- **⏰ 智能提醒** — 弹窗通知 + 自定义提示音，支持「5分钟后再提醒」
- **🎤 语音输入** — 麦克风录音，Vosk 离线识别转文字
- **🤖 AI 自然语言添加** — 输入或说出"明天中午十二点吃饭，提前半小时提醒我"，自动解析为结构化任务
- **🎵 自定义提示音** — 把你的 `remind.wav` 放到 `assets/` 下即可

---

## 📁 项目结构

```
ECalendar/
├── CMakeLists.txt                  # CMake 构建配置
├── .gitignore
├── README.md
│
├── src/
│   ├── core/                       # 核心业务逻辑
│   │   ├── Task.h/cpp              # 任务数据模型
│   │   ├── User.h/cpp              # 用户模型（SHA256 密码）
│   │   ├── TaskManager.h/cpp       # 任务管理：增删改查、持久化、提醒检测
│   │   └── Storage.h/cpp           # 自定义 JSON 序列化 / 反序列化
│   │
│   ├── gui/                        # Qt5 图形界面
│   │   ├── main_gui.cpp            # 应用入口
│   │   ├── MainWindow.h/cpp        # 主窗口：工具栏、表格、状态栏
│   │   ├── LoginDialog.h/cpp       # 登录 / 注册对话框
│   │   ├── TaskDialog.h/cpp        # 创建 / 编辑任务对话框
│   │   ├── ReminderPopup.h/cpp     # 提醒弹窗（支持延后）
│   │   └── style.qss               # 全局 Qt 样式表
│   │
│   ├── audio/                      # 音频 & 语音模块
│   │   ├── Reminder.h/cpp          # 提醒音效播放
│   │   ├── SpeechRecognizer.h/cpp  # Vosk 离线语音识别
│   │   └── AudioRecoder.h/cpp      # 麦克风录音工具
│   │
│   └── util/                       # 工具模块
│       ├── Hash.h/cpp              # 纯 C++ SHA256 实现
│       └── NLProcess.h/cpp         # 自然语言 → 结构化任务解析
│                                   # （DeepSeek API + 正则回退）
│
├── assets/
│   └── remind.wav                  # 自定义提醒音（可替换）
│
├── model/                          # Vosk 语音模型（gitignored，约 2GB）
│   └── vosk-model-cn-0.22/
│
├── third_party/vosk/               # Vosk C API 头文件 & 动态库
│   ├── vosk_api.h
│   └── libvosk.so
│
└── build/                          # 构建输出目录（gitignored）
```

---

## 🛠️ 构建步骤

### 依赖安装

| 依赖 | 版本 | 安装命令（WSL / Ubuntu） |
|---|---|---|
| CMake | ≥ 3.10 | `sudo apt install cmake` |
| Qt5 (Widgets + Multimedia) | 5.x | `sudo apt install qtbase5-dev qtmultimedia5-dev` |
| libcurl | 7.x | `sudo apt install libcurl4-openssl-dev` *（可选，用于 AI 解析）* |
| Vosk 模型 | — | 见 [模型配置](#-vosk-语音识别模型配置) |
| 编译工具链 | — | `sudo apt install build-essential` |

### 编译

```bash
cd ECalendar
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
```

### 运行

```bash
./ecalender_gui
```

在 WSLg 下 GUI 会原生显示。纯 Linux 环境需要 X Server。

---

## 🧠 DeepSeek AI 自然语言解析

支持输入口语化描述，自动解析为结构化任务。例如：

> `明天中午十二点吃饭，提前半小时提醒我`

解析流程：
1. **优先调用 DeepSeek Chat API**（精确解析）
2. **API 不可用时自动回退到正则解析**

### 配置 API Key

在项目根目录创建 `.env` 文件：

```bash
echo 'DEEPSEEK_API_KEY=***' > .env
```

> ⚠️ `.env` 已被 `.gitignore` 忽略，不会提交到仓库。

---

## 🎙️ Vosk 语音识别模型配置

1. 下载中文语音模型：

```bash
cd ECalendar/model
wget https://alphacephei.com/vosk/models/vosk-model-cn-0.22.zip
unzip vosk-model-cn-0.22.zip
rm vosk-model-cn-0.22.zip
```

> 模型必须放在 `model/vosk-model-cn-0.22/` 路径下，代码按此路径查找。

2. 确保录音工具已安装：

```bash
sudo apt install alsa-utils        # arecord
# 或
sudo apt install pulseaudio-utils  # parec（WSLg 推荐）
```

3. 在应用中点击 🎤 按钮开始 / 停止录音，语音会实时转写为文字。

---

## 🔔 自定义提醒音

将你的 `remind.wav` 放到项目根目录的 `assets/remind.wav`，触发提醒时自动播放。

支持 PCM 16-bit 编码，任意采样率（使用 `paplay` / `aplay` 播放）。

---

## 📝 数据存储

所有用户数据以 JSON 格式存储在 `~/.ecalender/` 下：

```
~/.ecalender/
├── users.json               # 注册用户列表（用户名 + 密码哈希）
└── tasks_<用户名>.json       # 各用户独立的任务数据
```

---

## 🧪 运行测试

内含命令行单元测试：

```bash
# 编译测试程序
cd build
cmake .. -DENABLE_QT=OFF
make test
./test
```

SHA256 哈希测试：

```bash
make test_hash
./test_hash "hello"
```

---

## 📦 依赖汇总

| 库 | 用途 | 是否必须 |
|---|---|---|
| Qt5 Widgets | GUI 框架 | ✅ 是 |
| Qt5 Multimedia | 音频播放 | ✅ 是 |
| Vosk | 离线语音识别 | ✅ 是（仅需运行时模型） |
| libcurl | DeepSeek API 调用 | 可选 |
| pthread | 多线程支持 | ✅ 是（系统自带） |

---

## 🔒 安全说明

- 密码使用纯 C++ 实现的 SHA256 哈希存储 — **不保存明文**
- API Key 从 `.env` 文件加载（已被 gitignore）— **不硬编码**
- 用户数据存储在本地 JSON 文件 — **不上传网络**

---

## 📄 许可证

本项目为大学课程作业，仅供学习参考。

---

## 🙏 致谢

- [Vosk](https://alphacephei.com/vosk/) — 离线语音识别引擎
- [DeepSeek](https://deepseek.com/) — 自然语言解析 API
- [Qt](https://www.qt.io/) — 跨平台 GUI 框架
