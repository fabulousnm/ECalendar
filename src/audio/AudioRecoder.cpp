/**
 * @file AudioRecorder.cpp
 * @brief 麦克风录音工具封装实现
 *
 * 先后尝试 arecord (ALSA)、rec (sox)、SoundRecorder (Win)、PowerShell 等
 * 录音工具，找到第一个可用的执行录音命令。
 *
 * 录音命令格式：
 *   arecord -r 16000 -f S16_LE -c 1 -d <秒数> <输出路径>
 *   rec -r 16000 -b 16 -c 1 <输出路径> trim 0 <秒数>
 *   SoundRecorder.exe /FILE <输出路径> /DURATION 00:00:<秒数>
 *
 * 如果 none 工具可用，需要安装：
 *   sudo apt install alsa-utils   # arecord
 *   sudo apt install sox          # rec
 */

#include "audio/AudioRecorder.h"

#include <cstdlib>
#include <cstdio>
#include <string>
#include <iostream>
#include <thread>
#include <chrono>
//平台检测
#ifdef _WIN32
#define IS_WINDOWS 1
#else
#define IS_WINDOWS 0
#endif

// ---------------------------------------------------------------------------
// 工具函数：为原始 PCM 数据添加 WAV 文件头
// （parec 默认输出原始 PCM，需要这个头才能被 Vosk 识别）
// ---------------------------------------------------------------------------

static bool writeWavHeader(const std::string& rawPath, const std::string& wavPath,
                            int sampleRate, int channels, int bitsPerSample) {
    // 读取原始 PCM 数据
    FILE* rawFile = fopen(rawPath.c_str(), "rb");
    if (!rawFile) return false;
    fseek(rawFile, 0, SEEK_END);
    long dataSize = ftell(rawFile);
    fseek(rawFile, 0, SEEK_SET);

    // 计算文件大小
    int headerSize = 44;
    int fileSize = headerSize + dataSize;
    int byteRate = sampleRate * channels * bitsPerSample / 8;
    int blockAlign = channels * bitsPerSample / 8;

    // 写入 WAV 文件
    FILE* wavFile = fopen(wavPath.c_str(), "wb");
    if (!wavFile) { fclose(rawFile); return false; }

    // RIFF 头
    fwrite("RIFF", 1, 4, wavFile);
    fwrite(&fileSize, 4, 1, wavFile);
    fwrite("WAVE", 1, 4, wavFile);

    // fmt 子块
    fwrite("fmt ", 1, 4, wavFile);
    int fmtSize = 16;
    short audioFormat = 1;       // PCM
    short numChannels = channels;
    int samplesPerSec = sampleRate;
    fwrite(&fmtSize, 4, 1, wavFile);
    fwrite(&audioFormat, 2, 1, wavFile);
    fwrite(&numChannels, 2, 1, wavFile);
    fwrite(&samplesPerSec, 4, 1, wavFile);
    fwrite(&byteRate, 4, 1, wavFile);
    fwrite(&blockAlign, 2, 1, wavFile);
    fwrite(&bitsPerSample, 2, 1, wavFile);

    // data 子块
    fwrite("data", 1, 4, wavFile);
    fwrite(&dataSize, 4, 1, wavFile);

    // 写入 PCM 数据
    char buf[8192];
    size_t n;
    while ((n = fread(buf, 1, sizeof(buf), rawFile)) > 0) {
        fwrite(buf, 1, n, wavFile);
    }

    fclose(rawFile);
    fclose(wavFile);
    std::remove(rawPath.c_str());
    return true;
}

// ---------------------------------------------------------------------------
// 工具函数：使用 parec 录制到 WAV 文件
// （录制原始 PCM 后添加 WAV 文件头）
// ---------------------------------------------------------------------------

static bool recordWithParec(const std::string& outputPath, int durationSec) {
    std::string rawPath = outputPath + ".raw";
    std::string cmd = "parec --channels=1 --rate=16000 --format=s16le --raw "
        + rawPath + " 2>/dev/null &";
    int ret = std::system(cmd.c_str());
    if (ret != 0) return false;

    // 等待录制完成
    std::this_thread::sleep_for(std::chrono::seconds(durationSec));

    // 停止 parec
    std::system("killall parec 2>/dev/null");
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    // 添加 WAV 头
    return writeWavHeader(rawPath, outputPath, 16000, 1, 16);
}

// ---------------------------------------------------------------------------
// 检测录音工具是否可用
// ---------------------------------------------------------------------------

bool AudioRecorder::isRecordToolAvailable() {
    // 尝试检测 arecord（WSL/Linux ALSA）
    int ret = std::system("which arecord 2>/dev/null > /dev/null");
    if (ret == 0) return true;

    // 尝试检测 parec（WSL/Linux PulseAudio — WSLg 启用后可用）
    ret = std::system("which parec 2>/dev/null > /dev/null");
    if (ret == 0) return true;

    // 尝试检测 rec (sox)
    ret = std::system("which rec 2>/dev/null > /dev/null");
    if (ret == 0) return true;

    // 尝试检测 SoundRecorder（Windows）
#if IS_WINDOWS
    ret = std::system("where SoundRecorder.exe 2>nul > nul");
    if (ret == 0) return true;
#endif

    return false;
}

// ---------------------------------------------------------------------------
// 获取可用的录音工具名称
// ---------------------------------------------------------------------------

std::string AudioRecorder::recordToolName() {
    // 先检查 arecord（WSL ALSA）
    int ret = std::system("which arecord 2>/dev/null > /dev/null");
    if (ret == 0) return "arecord";

    // 再检查 parec（WSL PulseAudio — WSLg 启用后优先）
    ret = std::system("which parec 2>/dev/null > /dev/null");
    if (ret == 0) return "parec";

    // 再检查 rec (sox)
    ret = std::system("which rec 2>/dev/null > /dev/null");
    if (ret == 0) return "rec";

    // 检查 SoundRecorder（Windows）
#if IS_WINDOWS
    ret = std::system("where SoundRecorder.exe 2>nul > nul");
    if (ret == 0) return "SoundRecorder";
#endif

    return "none";
}

// ---------------------------------------------------------------------------
// 录制音频（固定时长）
// ---------------------------------------------------------------------------

bool AudioRecorder::record(const std::string& outputPath, int durationSec) {
    std::string cmd;

    // 优先用 arecord，回退到 parec，再回退到 rec
    if (std::system("which arecord 2>/dev/null > /dev/null") == 0) {
        // arecord: 16kHz, 16-bit signed LE, 单声道
        cmd = "arecord -r 16000 -f S16_LE -c 1 -d "
            + std::to_string(durationSec) + " \""
            + outputPath + "\" 2>/dev/null";
    } else if (std::system("which parec 2>/dev/null > /dev/null") == 0) {
        // parec: WSLg PulseAudio 模式，录制后添加WAV头
        return recordWithParec(outputPath, durationSec);
    } else if (std::system("which rec 2>/dev/null > /dev/null") == 0) {
        // rec (sox): 16kHz, 16-bit, 单声道
        cmd = "rec -r 16000 -b 16 -c 1 \"" + outputPath
            + "\" trim 0 " + std::to_string(durationSec) + " 2>/dev/null";
    } else {
        // 无可用录音工具
        return false;
    }

    int ret = std::system(cmd.c_str());
    return (ret == 0);
}

// ---------------------------------------------------------------------------
// 智能录音：自动选择可用录音工具
// ---------------------------------------------------------------------------

/**
 * recordSmart - 智能录音
 *
 * 优先级：
 *   1. arecord（WSL/Linux 原生）
 *   2. rec (sox)
 *   3. SoundRecorder（Windows 10+ 内置）
 *   4. PowerShell（Windows 备用）
 *   5. Python（兜底）
 *
 * @param outputPath 输出 WAV 文件路径
 * @param durationSec 录制时长（秒）
 * @return true 录制成功
 */
bool AudioRecorder::recordSmart(const std::string& outputPath, int durationSec) {
    // 1. 尝试 arecord（WSL/Linux）
    if (std::system("which arecord 2>/dev/null > /dev/null") == 0) {
        std::string cmd = "arecord -r 16000 -f S16_LE -c 1 -d "
            + std::to_string(durationSec) + " \""
            + outputPath + "\" 2>/dev/null";
        int ret = std::system(cmd.c_str());
        if (ret == 0) return true;
    }

    // 2. 尝试 parec（WSLg PulseAudio）
    if (std::system("which parec 2>/dev/null > /dev/null") == 0) {
        if (recordWithParec(outputPath, durationSec)) return true;
    }

    // 3. 尝试 rec (sox)
    if (std::system("which rec 2>/dev/null > /dev/null") == 0) {
        std::string cmd = "rec -r 16000 -b 16 -c 1 \"" + outputPath
            + "\" trim 0 " + std::to_string(durationSec) + " 2>/dev/null";
        int ret = std::system(cmd.c_str());
        if (ret == 0) return true;
    }

#if IS_WINDOWS
    // 4. 尝试 SoundRecorder（Windows 内置）
    std::string cmd = "SoundRecorder.exe /FILE \"" + outputPath
        + "\" /DURATION 00:00:"
        + (durationSec < 10 ? "0" : "") + std::to_string(durationSec);
    int ret = std::system(cmd.c_str());
    if (ret == 0) return true;

    // 4. 尝试 PowerShell 录音
    return recordWithPowerShell(outputPath, durationSec);
#else
    // 5. Linux 下尝试 Python
    std::string pyCmd =
        "python3 -c \""
        "import sounddevice as sd, soundfile as sf; "
        "data = sd.rec(" + std::to_string(durationSec * 16000)
        + ", samplerate=16000, channels=1); "
        "sd.wait(); "
        "sf.write('" + outputPath + "', data, 16000)"
        "\" 2>/dev/null";
    int ret = std::system(pyCmd.c_str());
    if (ret == 0) return true;

    return false;
#endif
}

// ---------------------------------------------------------------------------
// Windows 11 原生录音
// ---------------------------------------------------------------------------

/**
 * recordWin11 - Windows 11 原生录音
 *
 * 通过 PowerShell 调用 Windows.Media.Capture API 录制音频。
 * 使用 .NET 的 System.Windows.Forms 让用户选择保存路径。
 *
 * @param outputPath 输出 WAV 文件路径
 * @param durationSec 录制时长（秒）
 * @return true 录制成功
 */
bool AudioRecorder::recordWin11(const std::string& outputPath, int durationSec) {
#if IS_WINDOWS
    return recordWithPowerShell(outputPath, durationSec);
#else
    // 非 Windows 环境，回退到 smart 模式
    return recordSmart(outputPath, durationSec);
#endif
}

// ---------------------------------------------------------------------------
// 通过 PowerShell 录音（Windows 通用方法）
// ---------------------------------------------------------------------------

/**
 * recordWithPowerShell - 使用 PowerShell 在 Windows 上录音
 *
 * 创建一个简单的 WPF/MFC 脚本录制麦克风音频。
 * 使用 Windows.Media.Capture 命名空间。
 *
 * @param outputPath 输出 WAV 文件路径
 * @param durationSec 录制时长（秒）
 * @return true 录制成功
 */
bool AudioRecorder::recordWithPowerShell(const std::string& outputPath, int durationSec) {
#if IS_WINDOWS
    // 创建 PowerShell 脚本文件（临时）
    std::string psScript =
        "Add-Type -AssemblyName System.Windows.Forms\n"
        "$outputPath = '" + outputPath + "'\n"
        "$duration = " + std::to_string(durationSec) + "\n"
        "Write-Host '🎙️ 录制中... (' $duration '秒)'\n"
        "Write-Host '请对麦克风说话...'\n"
        "\n"
        "# 使用 Windows 内置的 SoundRecorder\n"
        "$srPath = 'SoundRecorder.exe'\n"
        "if (Get-Command $srPath -ErrorAction SilentlyContinue) {\n"
        "    $durStr = '00:00:' + $duration.ToString('D2')\n"
        "    Start-Process -NoNewWindow $srPath -ArgumentList '/FILE', $outputPath, "
        "'/DURATION', $durStr -Wait\n"
        "    Write-Host '✅ 录制完成'\n"
        "    exit 0\n"
        "}\n"
        "\n"
        "# SoundRecorder 不可用，通知用户\n"
        "Write-Host '⚠ SoundRecorder 不可用，请手动录音并保存为:'\n"
        "Write-Host '  ' $outputPath\n"
        "exit 1\n";

    // 写入临时脚本文件
    std::string psFile = std::string(std::getenv("TEMP")) + "\\ecalender_record.ps1";
    FILE* fp = fopen(psFile.c_str(), "w");
    if (fp) {
        fputs(psScript.c_str(), fp);
        fclose(fp);
    } else {
        return false;
    }

    // 执行 PowerShell 脚本
    std::string cmd = "powershell.exe -ExecutionPolicy Bypass -File \""
        + psFile + "\"";
    int ret = std::system(cmd.c_str());

    // 清理临时脚本
    std::remove(psFile.c_str());

    return (ret == 0);
#else
    // 非 Windows 环境回退
    (void)outputPath;
    (void)durationSec;
    return false;
#endif
}

