//实现 Task 类 

#include "core/Task.h"

//含参构造函数 ，初始化列表加快构造速度

Task::Task(int id, const std::string& name, const std::string& startTime,
           int priority, const std::string& category, const std::string& remindTime)
    : id(id), name(name), startTime(startTime), priority(priority),
      category(category), remindTime(remindTime) {}

//operator< - 按开始时间升序比较两个任务

bool Task::operator<(const Task& other) const {
    return startTime < other.startTime;
}

