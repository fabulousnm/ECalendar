/**
 * @file SpeechRecognizer.cpp
 * @brief Vosk 离线语音识别封装实现
 *
 * 使用 Vosk C API 识别语音。
 * 共享模型机制：首次加载后复用，后续无加载延迟。
 * 要求音频为 16kHz, 16-bit, 单声道 (mono)。
 */

#include "audio/SpeechRecognizer.h"
#include "vosk_api.h"

#include <iostream>
#include <fstream>
#include <vector>
#include <cstring>
#include <cstdint>
#include <algorithm>

// ---- 共享模型（静态成员）----
void* SpeechRecognizer::s_sharedModel_ = nullptr;
std::string SpeechRecognizer::s_modelPath_;
std::atomic<bool> SpeechRecognizer::s_modelLoaded_{false};

SpeechRecognizer::SpeechRecognizer()
    : recognizer_(nullptr), loaded_(false) {}

SpeechRecognizer::~SpeechRecognizer() {
    if (recognizer_) {
        vosk_recognizer_free(static_cast<VoskRecognizer*>(recognizer_));
        recognizer_ = nullptr;
    }
    // 析构时不释放 s_sharedModel_（由 unloadModel 在程序退出时释放）
}

void SpeechRecognizer::unloadModel() {
    if (s_sharedModel_) {
        vosk_model_free(static_cast<VoskModel*>(s_sharedModel_));
        s_sharedModel_ = nullptr;
        s_modelLoaded_ = false;
    }
    s_modelPath_.clear();
    s_modelLoaded_ = false;
}

bool SpeechRecognizer::loadModel(const std::string& modelPath) {
    // 检查是否已有加载好的共享模型
    if (s_sharedModel_ && s_modelPath_ == modelPath) {
        // 模型已加载，只需新建识别器
        if (recognizer_) {
            vosk_recognizer_free(static_cast<VoskRecognizer*>(recognizer_));
        }
        recognizer_ = static_cast<void*>(
            vosk_recognizer_new(static_cast<VoskModel*>(s_sharedModel_), 16000.0f));
        if (!recognizer_) {
            lastError_ = "识别器创建失败";
            loaded_ = false;
            return false;
        }
        loaded_ = true;
        s_modelLoaded_ = true;

        // 诊断：打印 VmRSS 内存占用
        {
            std::ifstream statusFile("/proc/self/status");
            if (statusFile.is_open()) {
                std::string line;
                while (std::getline(statusFile, line)) {
                    if (line.compare(0, 6, "VmRSS:") == 0) {
                        std::cerr << "[Vosk] Reusing loaded model. " << line << std::endl;
                        break;
                    }
                }
                statusFile.close();
            }
        }

        return true;
    }

    // 首次加载：释放旧模型，创建新模型
    if (s_sharedModel_) {
        vosk_model_free(static_cast<VoskModel*>(s_sharedModel_));
        s_sharedModel_ = nullptr;
    }
    if (recognizer_) {
        vosk_recognizer_free(static_cast<VoskRecognizer*>(recognizer_));
        recognizer_ = nullptr;
    }

    void* newModel = static_cast<void*>(vosk_model_new(modelPath.c_str()));
    if (!newModel) {
        lastError_ = "模型加载失败: " + modelPath;
        loaded_ = false;
        return false;
    }

    s_sharedModel_ = newModel;
    s_modelPath_ = modelPath;

    recognizer_ = static_cast<void*>(
        vosk_recognizer_new(static_cast<VoskModel*>(s_sharedModel_), 16000.0f));
    if (!recognizer_) {
        vosk_model_free(static_cast<VoskModel*>(s_sharedModel_));
        s_sharedModel_ = nullptr;
        lastError_ = "识别器创建失败";
        loaded_ = false;
        return false;
    }

    loaded_ = true;
    s_modelLoaded_ = true;

    // 诊断：打印 VmRSS 内存占用
    {
        std::ifstream statusFile("/proc/self/status");
        if (statusFile.is_open()) {
            std::string line;
            while (std::getline(statusFile, line)) {
                if (line.compare(0, 6, "VmRSS:") == 0) {
                    // line format: "VmRSS:  2048000 kB"
                    std::cerr << "[Vosk] Model loaded. " << line << std::endl;
                    break;
                }
                if (line.compare(0, 6, "VmHWM:") == 0) {
                    std::cerr << "[Vosk] Peak memory. " << line << std::endl;
                }
            }
            statusFile.close();
        }
    }

    return true;
}

static int16_t read16LE(const char* data) {
    return static_cast<int16_t>(data[0] | (data[1] << 8));
}

std::string SpeechRecognizer::recognizeFile(const std::string& wavPath) {
    if (!loaded_ || !recognizer_) {
        lastError_ = "模型未加载";
        return R"({"text": ""})";
    }

    // 重置识别器状态
    vosk_recognizer_reset(static_cast<VoskRecognizer*>(recognizer_));

    std::ifstream file(wavPath, std::ios::binary);
    if (!file.is_open()) {
        lastError_ = "无法打开文件: " + wavPath;
        return R"({"text": ""})";
    }

    char header[12];
    file.read(header, 12);
    if (std::strncmp(header, "RIFF", 4) != 0 || std::strncmp(header + 8, "WAVE", 4) != 0) {
        file.close();
        lastError_ = "非标准WAV文件格式";
        return R"({"text": ""})";
    }

    int sampleRate = 0, channels = 0, bitsPerSample = 0;
    bool fmtFound = false;
    char chunkId[4];
    uint32_t chunkSize;

    while (file.read(chunkId, 4)) {
        file.read(reinterpret_cast<char*>(&chunkSize), 4);
        if (std::strncmp(chunkId, "fmt ", 4) == 0) {
            char fmtBuf[16];
            file.read(fmtBuf, std::min<uint32_t>(chunkSize, 16));
            auto uc = [](char c) -> unsigned int { return static_cast<unsigned char>(c); };
            channels = uc(fmtBuf[2]) | (uc(fmtBuf[3]) << 8);
            sampleRate = uc(fmtBuf[4]) | (uc(fmtBuf[5]) << 8) | (uc(fmtBuf[6]) << 16) | (uc(fmtBuf[7]) << 24);
            bitsPerSample = uc(fmtBuf[14]) | (uc(fmtBuf[15]) << 8);
            fmtFound = true;
            if (chunkSize > 16) file.seekg(chunkSize - 16, std::ios::cur);
        } else if (std::strncmp(chunkId, "data", 4) == 0) {
            if (!fmtFound) {
                file.close();
                lastError_ = "缺少 fmt chunk";
                return R"({"text": ""})";
            }

            std::vector<char> buffer(static_cast<size_t>(chunkSize));
            file.read(buffer.data(), chunkSize);
            file.close();

            // 格式检查
            if (sampleRate != 16000 || channels != 1 || bitsPerSample != 16) {
                std::string convertedPath = wavPath + ".converted.wav";
                std::string ffmpegCmd = "ffmpeg -y -i \"" + wavPath
                    + "\" -ac 1 -ar 16000 -sample_fmt s16 \""
                    + convertedPath + "\" 2>/dev/null";
                int ffmpegRet = std::system(ffmpegCmd.c_str());

                if (ffmpegRet == 0) {
                    std::string result = recognizeFile(convertedPath);
                    std::remove(convertedPath.c_str());
                    return result;
                } else {
                    lastError_ = "音频格式不匹配，自动转换失败";
                    return R"({"text": ""})";
                }
            }

            // 喂入 Vosk
            const int CHUNK = 8000;
            for (int offset = 0; offset < static_cast<int>(buffer.size()); offset += CHUNK) {
                int len = std::min(CHUNK, static_cast<int>(buffer.size()) - offset);
                vosk_recognizer_accept_waveform(
                    static_cast<VoskRecognizer*>(recognizer_),
                    buffer.data() + offset, len);
            }

            const char* result = vosk_recognizer_final_result(
                static_cast<VoskRecognizer*>(recognizer_));

            return result ? std::string(result) : R"({"text": ""})";
        } else {
            file.seekg(chunkSize, std::ios::cur);
        }
    }

    file.close();
    lastError_ = "未找到 data chunk";
    return R"({"text": ""})";
}

std::string SpeechRecognizer::recognizeBuffer(const char* data, int length) {
    if (!loaded_ || !recognizer_ || !data || length <= 0) {
        return R"({"text": ""})";
    }

    vosk_recognizer_reset(static_cast<VoskRecognizer*>(recognizer_));

    const int CHUNK_SIZE = 8000;
    for (int offset = 0; offset < length; offset += CHUNK_SIZE) {
        int chunkLen = (offset + CHUNK_SIZE <= length) ? CHUNK_SIZE : (length - offset);
        vosk_recognizer_accept_waveform(
            static_cast<VoskRecognizer*>(recognizer_),
            data + offset, chunkLen);
    }

    const char* result = vosk_recognizer_final_result(
        static_cast<VoskRecognizer*>(recognizer_));

    return result ? std::string(result) : R"({"text": ""})";
}

std::string SpeechRecognizer::getText(const std::string& jsonResult) {
    size_t pos = jsonResult.find("\"text\"");
    if (pos == std::string::npos) return "";
    pos = jsonResult.find(':', pos);
    if (pos == std::string::npos) return "";
    pos = jsonResult.find('"', pos);
    if (pos == std::string::npos) return "";
    pos++;
    size_t end = jsonResult.find('"', pos);
    if (end == std::string::npos) return "";
    return jsonResult.substr(pos, end - pos);
}

// ===================================================================
// 流式识别接口实现
// ===================================================================

void* SpeechRecognizer::getRawRecognizer() {
    return recognizer_;
}

void SpeechRecognizer::resetRecognizer() {
    if (recognizer_) {
        vosk_recognizer_reset(static_cast<VoskRecognizer*>(recognizer_));
    }
}

bool SpeechRecognizer::acceptWaveform(const char* data, int length) {
    if (!recognizer_ || !data || length <= 0) return false;
    vosk_recognizer_accept_waveform(
        static_cast<VoskRecognizer*>(recognizer_),
        data, length);
    return true;
}

std::string SpeechRecognizer::getPartialResult() {
    if (!recognizer_) return "";
    const char* result = vosk_recognizer_partial_result(
        static_cast<VoskRecognizer*>(recognizer_));
    return result ? std::string(result) : "";
}

std::string SpeechRecognizer::getFinalResult() {
    if (!recognizer_) return "";
    const char* result = vosk_recognizer_final_result(
        static_cast<VoskRecognizer*>(recognizer_));
    return result ? std::string(result) : "";
}

std::string SpeechRecognizer::extractPartialText(const std::string& jsonResult) {
    if (jsonResult.empty()) return "";
    // 查找 "partial" : "..."
    size_t pos = jsonResult.find("\"partial\"");
    if (pos == std::string::npos) return "";
    pos = jsonResult.find('"', pos + 9);
    if (pos == std::string::npos) return "";
    pos++;
    size_t end = jsonResult.find('"', pos);
    if (end == std::string::npos) return "";
    return jsonResult.substr(pos, end - pos);
}

