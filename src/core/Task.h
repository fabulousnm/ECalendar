/*
 Task.h
定义了class任务，包括数据成员以及成员函数等
 */

#ifndef ECALENDER_TASK_H
#define ECALENDER_TASK_H

#include <string>

/*
表示一条日程任务，包含编号、名称、开始时间、优先级、分类和提醒时间
 重定义了<，使得能够比较现实时间
 优先级和分类均有缺省值
 */
class Task {
public:
    int id = 0;                     // 任务唯一编号（自动分配，0 表示未分配）
    std::string name;               // 任务名称
    std::string startTime;          // 开始时间，格式 "YYYY-MM-DD HH:MM"
    int priority = 2;               // 优先级：1=高（紧急），2=中（普通），3=低（宽松）
    std::string category = "学习";   // 任务分类，默认为"学习"
    std::string remindTime;         // 提醒时间，格式 "YYYY-MM-DD HH:MM"，空字符串表示默认开始时间前15分钟提醒

    Task() = default;//默认构造函数
    Task(int id, const std::string& name, const std::string& startTime,
         int priority, const std::string& category, const std::string& remindTime);

    // 属性访问与下修改函数，便于单独访问与修改数据成员
    int getId() const { return id; }
    void setId(int pid) { id = pid; }

    const std::string& getName() const { return name; }
    void setName(const std::string& n) { name = n; }

    const std::string& getStartTime() const { return startTime; }
    void setStartTime(const std::string& t) { startTime = t; }

    int getPriority() const { return priority; }
    void setPriority(int p) { priority = p; }

    const std::string& getCategory() const { return category; }
    void setCategory(const std::string& c) { category = c; }

    const std::string& getRemindTime() const { return remindTime; }
    void setRemindTime(const std::string& t) { remindTime = t; }
   //重定义<符号，便于现实时间的对比
    bool operator<(const Task& other) const;
};

#endif // ECALENDER_TASK_H

