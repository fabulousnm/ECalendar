#ifndef ECALENDER_SPEECHRECOGNIZER_H
#define ECALENDER_SPEECHRECOGNIZER_H

#include <string>
#include <atomic>

/**
 * SpeechRecognizer - Vosk 语音识别封装
 *
 * 使用方法：
 *   1. 构造对象
 *   2. 调用 loadModel("模型路径") 加载模型
 *   3. 调用 recognizeFile("音频路径") 或 recognizeBuffer(data, len)
 *   4. 调用 getText(json) 提取纯文本结果
 *
 * 音频格式要求：PCM 16kHz, 16-bit, 单声道 (mono)
 * 支持自动检测和转换立体声/非16kHz音频。
 */
class SpeechRecognizer {
public:
    SpeechRecognizer();
    ~SpeechRecognizer();

    /**
     * loadModel - 加载 Vosk 模型
     *
     * 使用共享模型机制：首次调用时加载模型到静态指针，
     * 后续调用直接复用已有模型，避免重复加载（节省 2-3 秒）。
     */
    bool loadModel(const std::string& modelPath);

    /**
     * unloadModel - 释放共享模型（程序退出时调用）
     */
    static void unloadModel();

    std::string recognizeFile(const std::string& wavPath);
    std::string recognizeBuffer(const char* data, int length);
    std::string getText(const std::string& jsonResult);
    bool isLoaded() const { return s_sharedModel_ != nullptr; }
    /** 检查共享模型是否已由后台线程预加载完成 */
    static bool isModelLoaded() { return s_modelLoaded_; }
    std::string lastError() const { return lastError_; }

    // --- 流式识别接口 ---
    /** 获取原始 VoskRecognizer 指针（用于流式喂入） */
    void* getRawRecognizer();
    /** 重置识别器 */
    void resetRecognizer();
    /** 喂入一片 PCM 音频数据 */
    bool acceptWaveform(const char* data, int length);
    /** 获取部分（实时）识别结果 JSON */
    std::string getPartialResult();
    /** 获取最终识别结果 JSON */
    std::string getFinalResult();
    /** 从 JSON 中提取 "partial" 字段文本 */
    std::string extractPartialText(const std::string& jsonResult);

private:
    void* recognizer_;            // 当前识别器（每次识别新建）
    bool loaded_ = false;
    std::string lastError_;
    bool streamMode_ = false;      // 流式模式标志（不自动 reset）

    static void* s_sharedModel_;         // 共享模型指针（所有实例共用）
    static std::string s_modelPath_;
    static std::atomic<bool> s_modelLoaded_; // 后台预加载完成标志
};

#endif

