/*TaskManager.h
任务管理器
负责任务的增删改查、用户登录注册、数据持久化调度。
 使用 锁 保证多线程下的数据安全。
 */

#ifndef ECALENDER_TASKMANAGER_H
#define ECALENDER_TASKMANAGER_H

#include <string>
#include <vector>
#include <mutex>
#include "Task.h"
#include "User.h"

/*
TaskManager 

 主要功能：
 1. 用户管理：登录验证和注册（用户名唯一性检查）
  2. 任务 CRUD：添加任务（含重名+同时间检测）、删除任务、按日期/月查询
  3. 提醒查询：获取当前时间前后 1 分钟内需要提醒的任务
  4. 数据持久化：加载和保存到 JSON 文件
*/

class TaskManager {
public:
    TaskManager() : currentUser_("") {}

    // 设置当前登录用户
    void setCurrentUser(const std::string& username) { currentUser_ = username; }
    std::string getCurrentUser() const { return currentUser_; }

    //用户管理 函数
    //登录
    bool login(const std::string& username, const std::string& password);

   //注册
    bool registerUser(const std::string& username, const std::string& password);

    // 任务管理函数
    //增加任务
    int addTask(const Task& task);
   //删除任务
    bool deleteTask(int id);
    
     //getTasksByDate - 按日期获取任务列表
     

    std::vector<Task> getTasksByDate(const std::string& date) const;

    /*getTasksByMonth - 按月份获取任务列表*/
     
    std::vector<Task> getTasksByMonth(const std::string& yearMonth) const;

    /*
     getUpcomingReminders - 获取当前时间附近需要提醒的任务，算当前时间 ±1 分钟的时间范围，返回所有 remindTime，在此范围内的任务。由后台提醒线程周期调用。
  
     */
    std::vector<Task> getUpcomingReminders() const;

    // 时间比较工具函数

    static bool isRemindTimeAfterStart(const std::string& remindTime,
                                       const std::string& startTime);

    // 数据本地保存函数
    
/*
  loadFromFile - 从 JSON 文件中加载全部数据
加载完成后自动计算 nextId（下一个任务分配的id)
taskFile 任务数据文件路径 
userFile 用户数据文件路径
*/
    void loadFromFile(const std::string& taskFile, const std::string& userFile);

    // 按用户名加载
    void loadFromUserFile(const std::string& baseDir, const std::string& userFile,
                          const std::string& username);

    void saveToFile(const std::string& taskFile, const std::string& userFile);

    // 按用户名保存
    void saveToUserFile(const std::string& baseDir, const std::string& userFile);

    // 访问已经储存的数据 
    const std::vector<Task>& getTasks() const { return tasks; }
    const std::vector<User>& getUsers() const { return users; }

private:
    std::vector<Task> tasks;     // 内存中的全部任务列表
    std::vector<User> users;     // 内存中的全部用户列表
    int nextId = 1;                      // 下一个可用 ID
    std::string currentUser_;      // 当前登录的用户名
    mutable std::mutex mtx;       // 互斥锁，保证线程安全
};

#endif 

