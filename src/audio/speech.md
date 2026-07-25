常用离线语音识别模型对比：


| 特性 | **Vosk** | **whisper.cpp** | **PocketSphinx** |
|------|----------|-----------------|-----------------|
| **底层技术** | Kaldi (HMM+DNN) | OpenAI Whisper (Transformer) | 传统 HMM |
| **语言** | C++ (C API) | C/C++ | C |
| **中文支持** | ✅ 原生支持 | ✅ 多语言 | ⚠️ 需额外中文模型 |
| **模型大小** | 42 MB (小) / 1.3 GB (大) | 75 MB (tiny) ~ 3.1 GB (large) | ~50 MB |
| **运行时内存** | ~300 MB (小模型) | ~1-8 GB | ~100 MB |
| **CPU 速度** | 实时率 < 0.1 (极快) | 实时率 0.5-5 (较慢) | 实时率 < 0.1 (极快) |
| **准确率（中文）** | 中等（命令够用） | **高**（最佳） | 低 |
| **流式识别** | ✅ 支持（流式 API） | ❌ 需等整段音频 | ✅ 支持 |
| **安装难度** | ⭐ 简单（pip install） | ⭐⭐⭐ 需编译 | ⭐⭐ apt install |
| **GPU 加速** | ❌ CPU only | ✅ CUDA/Metal/Vulkan | ❌ |
| **活跃维护** | ✅ 持续更新 | ✅ 非常活跃 | ❌ 基本停更 |


由于vosk开源，是易于安装的第三方库，且轻量，api完善，初步打算安装vosk进行语音识别
初步代码框架：
Ecalender/
├── third_party/
│   └── vosk/
│       ├── libvosk.so          # 预编译库
│       └── vosk_api.h          # C API 头文件
├── model/
│   └── vosk-model-small-cn-0.22/  # 中文模型
├── src/audio/
│            ├── SpeechRecognizer.h      # 语音识别封装类
│            ├── SpeechRecognizer.cpp
│            ├── AudioRecorder.h         # 录音模块
│            ├── AudioRecorder.cpp
│            ├── main.cpp
│            └── ...
└── CMakeLists.txt
```

对于语音识别后的字符串解析，由于自然语言的多样性，只用正则化表达很难正确识别出内容，因此正则化表达作为备用方案，首要方案使用deepseek进行解析返回格式化内容，此处应该把deepseek api放在Ecalender主目录的.env环境变量中，格式为DEEPSEEK_API_KEY=sk-****************
