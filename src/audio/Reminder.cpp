//Reminder.cpp
#include "audio/Reminder.h"
#include <iostream>
#include <fstream>
#include <cstdlib>
#include <cmath>
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#include <cstring>
#include <vector>

// 文件作用域辅助函数
// generateMinimalWav - 生成一个简单的 WAV 提示音
 
static void generateMinimalWav(const std::string& path) {
    const int sampleRate = 44100;
    const int durationSec = 1;
    const int numSamples = sampleRate * durationSec;
    const short amplitude = 16000;
    const double freq = 440.0;

    std::vector<short> samples(numSamples);
    for (int i = 0; i < numSamples; i++) {
        double t = static_cast<double>(i) / sampleRate;
        samples[i] = static_cast<short>(amplitude * std::sin(2.0 * M_PI * freq * t));
    }

    int dataBytes = numSamples * static_cast<int>(sizeof(short));
    int fileSize = 36 + dataBytes;

    std::ofstream ofs(path, std::ios::binary);
    if (!ofs) return;

    ofs.write("RIFF", 4);
    ofs.write(reinterpret_cast<const char*>(&fileSize), 4);
    ofs.write("WAVE", 4);
    ofs.write("fmt ", 4);
    int subchunk1Size = 16;
    short audioFormat = 1;
    short numChannels = 1;
    int byteRate = sampleRate * 2;
    short blockAlign = 2;
    short bitsPerSample = 16;
    ofs.write(reinterpret_cast<const char*>(&subchunk1Size), 4);
    ofs.write(reinterpret_cast<const char*>(&audioFormat), 2);
    ofs.write(reinterpret_cast<const char*>(&numChannels), 2);
    ofs.write(reinterpret_cast<const char*>(&sampleRate), 4);
    ofs.write(reinterpret_cast<const char*>(&byteRate), 4);
    ofs.write(reinterpret_cast<const char*>(&blockAlign), 2);
    ofs.write(reinterpret_cast<const char*>(&bitsPerSample), 2);
    ofs.write("data", 4);
    ofs.write(reinterpret_cast<const char*>(&dataBytes), 4);
    ofs.write(reinterpret_cast<const char*>(samples.data()), dataBytes);
}

/**
 * wavPath - 查找 remind.wav 的实际路径
 * 候选路径：assets/remind.wav */
static std::string wavPath() {
    static const char* candidates[] = {
        "assets/remind.wav",
        "../assets/remind.wav",
        "../../assets/remind.wav",
    };
    for (auto* p : candidates) {
        std::ifstream test(p);
        if (test) { test.close(); return p; }
    }
    return "assets/remind.wav";
}

// ensureWavExists - 确保 remind.wav 文件存在
static void ensureWavExists() {
    std::string path = wavPath();
    std::ifstream test(path);
    if (test) { test.close(); return; }

    std::string dir = path.substr(0, path.find_last_of("/\\"));
    if (!dir.empty()) {
#ifdef _WIN32
        std::string mkdirCmd = "if not exist \"" + dir + "\" mkdir \"" + dir + "\"";
#else
        std::string mkdirCmd = "mkdir -p \"" + dir + "\"";
#endif
        system(mkdirCmd.c_str());
    }
    generateMinimalWav(path);
}
//接口
void Reminder::init() {
    ensureWavExists();
}

void Reminder::printReminder(const Task& task) {
    std::string prioStr = (task.priority == 1) ? "高" :
                          (task.priority == 2) ? "中" : "低";
    std::cout << "\n===== ⏰ 任务提醒 =====\n";
    std::cout << "任务：" << task.name << "\n";
    std::cout << "开始时间：" << task.startTime << "\n";
    std::cout << "优先级：" << prioStr << "\n";
    std::cout << "========================\n" << std::endl;
}
//停止上一个音频，防止叠加
void Reminder::killAudio() {
    std::system("killall paplay aplay ffplay mpg123 2>/dev/null");
#ifdef _WIN32
    std::system("taskkill /F /IM powershell.exe 2>nul");
#endif
}

void Reminder::playReminderSound() {
    ensureWavExists();
    std::string path = wavPath();

    // 原始播放链：paplay → aplay → powershell
    if (std::system(("which paplay 2>/dev/null > /dev/null && paplay \"" + path + "\" 2>/dev/null &").c_str()) == 0) {}
    else if (std::system(("which aplay 2>/dev/null > /dev/null && aplay \"" + path + "\" 2>/dev/null &").c_str()) == 0) {}
    else {
        std::system(("powershell.exe -c \"(New-Object Media.SoundPlayer '" + path + "').PlaySync()\" 2>/dev/null &").c_str());
    }
}

void Reminder::checkAndRemind(TaskManager& manager) {
    auto reminders = manager.getUpcomingReminders();
    for (const auto& task : reminders) {
        printReminder(task);
        playReminderSound();
    }
}

