/*Reminder.h
 提示音模块
 使用 remind.wav 作为提醒音效。
 用户可用自己的 remind.wav 替换 assets/ 目录下的文件。*/
#ifndef ECALENDER_REMINDER_H
#define ECALENDER_REMINDER_H

#include "core/Task.h"
#include "core/TaskManager.h"

/**
 * Reminder - 提醒工具类
 *
 * 所有方法均为静态，可直接调用。
 * 主要功能：
 *   1. 在控制台打印任务提醒信息
 *   2. 播放/中断提醒
 *   3. 周期性检测到期提醒并触发
 */
class Reminder {
public:
    static void printReminder(const Task& task);
    static void playReminderSound();
    static void killAudio();
    static void checkAndRemind(TaskManager& manager);
    static void init();

private:
    // 所有辅助函数已改为static，无需在此声明
};

#endif // ECALENDER_REMINDER_H

