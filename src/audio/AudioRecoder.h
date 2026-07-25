/**
 * @file AudioRecorder.h
 * @brief 麦克风录音工具封装
 *
 * 通过系统命令调用 arecord/rec/SoundRecorder 录制麦克风音频。
 * 兼容 WSL (Linux) 环境，也支持 Windows 端录制的 WAV 文件。
 *
 * 录音格式固定为：WAV, 16kHz, 16-bit, 单声道 (mono)
 * 这是 Vosk 语音识别要求的音频格式。
 *
 * 新增方法：
 *   recordSmart() — 智能选择可用录音工具
 *   recordWin11() — Windows 11 原生录音（PowerShell）
 *   recordInteractive() — 交互式录音（按 Enter 启停）
 */

#ifndef ECALENDER_AUDIORECORDER_H
#define ECALENDER_AUDIORECORDER_H

#include <string>

/**
 * AudioRecorder - 录音工具封装（静态方法）
 *
 * 使用 arecord (ALSA) / rec (sox) / PowerShell / Python 等
 * 命令行工具录制音频，按优先级自动选择。
 *
 * 如果 WSL 环境中没有录音硬件（麦克风直通），
 * 可以在 Windows 端用 recordSmart 或 recordWin11 录制。
 */
class AudioRecorder {
public:
    // 录制麦克风音频到 WAV 文件（固定时长模式）
    // @param outputPath 输出文件路径
    // @param durationSec 录制时长（秒），默认 5 秒
    // @return true 录制成功
    static bool record(const std::string& outputPath, int durationSec = 5);

    /**
     * recordSmart - 智能录音：自动选择可用录音工具
     *
     * 优先级：arecord(WSL) > SoundRecorder(Win10+) > PowerShell(Win11) > Python
     * 依次尝试，找到可用的工具执行录音。
     *
     * @param outputPath 输出 WAV 文件路径
     * @param durationSec 录制时长（秒），默认 5 秒
     * @return true 录制成功
     */
    static bool recordSmart(const std::string& outputPath, int durationSec = 5);

    /**
     * recordWin11 - Windows 11 原生录音
     *
     * 通过 PowerShell 调用 Windows 原生录音能力。
     * 使用 .NET 的 System.Windows.Forms 保存对话框让用户选择保存路径，
     * 或直接写入指定路径。
     *
     * @param outputPath 输出 WAV 文件路径
     * @param durationSec 录制时长（秒），默认 5 秒
     * @return true 录制成功
     */
    static bool recordWin11(const std::string& outputPath, int durationSec = 5);

    // 检查录音工具（arecord 或 rec）是否可用
    static bool isRecordToolAvailable();

    // 获取实际可用的录音工具名称
    static std::string recordToolName();

private:
    AudioRecorder() = delete;  // 静态类，禁止实例化

    // Windows 下通过 PowerShell 录制（使用 Windows.Media.Capture API）
    static bool recordWithPowerShell(const std::string& outputPath, int durationSec);
};

#endif // ECALENDER_AUDIORECORDER_H

