// TaskManager.cpp



#include "core/TaskManager.h"
#include "core/Storage.h"
#include <algorithm>
#include <ctime>
#include <chrono>
#include <sstream>
#include <cstdio>

// 时间比较

static std::vector<int> parseDateTime(const std::string& dt) {
    std::vector<int> parts;
    if (dt.size() < 16) return parts;  // "YYYY-MM-DD HH:MM" = 16 字符
    int y, m, d, h, min;
    if (std::sscanf(dt.c_str(), "%d-%d-%d %d:%d", &y, &m, &d, &h, &min) == 5) {
        parts.push_back(y);
        parts.push_back(m);
        parts.push_back(d);
        parts.push_back(h);
        parts.push_back(min);
    }
    return parts;
}


 //isRemindTimeAfterStart - 比较提醒时间是否晚于开始时间

bool TaskManager::isRemindTimeAfterStart(const std::string& remindTime,
                                          const std::string& startTime) {
    auto r = parseDateTime(remindTime);
    auto s = parseDateTime(startTime);
    if (r.size() != 5 || s.size() != 5) {
        // 解析失败
        return remindTime > startTime;
    }
    for (int i = 0; i < 5; ++i) {
        if (r[i] > s[i]) return true;   // 提醒时间偏大 → 晚于
        if (r[i] < s[i]) return false;  // 提醒时间偏小 → 早于
    }
    return false;  // 全部相等 → 不晚于
}


// 用户管理


bool TaskManager::login(const std::string& username, const std::string& password) {
    std::lock_guard<std::mutex> lock(mtx);
    for (const auto& u : users) {
        // 遍历用户列表，比对用户名和密码哈希
        if (u.username == username && u.checkPassword(password))
            return true;
    }
    return false;
}

bool TaskManager::registerUser(const std::string& username, const std::string& password) {
    std::lock_guard<std::mutex> lock(mtx);
    // 检查用户名是否已被占用
    for (const auto& u : users) {
        if (u.username == username)
            return false; // 用户名已存在，注册失败
    }
    // 创建新用户
    users.emplace_back(username, password);
    return true;
}


// 任务管理

int TaskManager::addTask(const Task& task) {
    std::lock_guard<std::mutex> lock(mtx);
    // 检查唯一性：不允许同名 、同开始时间的任务
    for (const auto& t : tasks) {
        if (t.name == task.name && t.startTime == task.startTime)
            return -1; // 重复任务
    }
    // 如果设置了提醒时间，检查提醒时间不能晚于开始时间
    // 必须比较完整的日期时间（年月日时分），不能只比较时间部分
    if (!task.remindTime.empty()) {
        if (isRemindTimeAfterStart(task.remindTime, task.startTime)) {
            return -2; // -2 提醒时间无效
        }
    }
    // 分配新 ID 并加入列表
    Task t = task;
    t.id = nextId++;
    tasks.push_back(t);
    return t.id;
}

bool TaskManager::deleteTask(int id) {
    std::lock_guard<std::mutex> lock(mtx);
    auto it = std::find_if(tasks.begin(), tasks.end(),
                           [id](const Task& t) { return t.id == id; });
    if (it == tasks.end()) return false; // 未找到
    tasks.erase(it);
    return true;
}

bool TaskManager::updateTaskRemindTime(int id, const std::string& newTime) {
    std::lock_guard<std::mutex> lock(mtx);
    for (auto& t : tasks) {
        if (t.id == id) {
            t.remindTime = newTime;
            return true;
        }
    }
    return false;
}

std::vector<Task> TaskManager::getTasksByDate(const std::string& date) const {
    std::lock_guard<std::mutex> lock(mtx);
    std::vector<Task> result;
    for (const auto& t : tasks) {
        // date 格式 "YYYY-MM-DD"，startTime 格式 "YYYY-MM-DD HH:MM"
        // 比较前 10 个字符判断是否同一天
        if (t.startTime.size() >= 10 && t.startTime.substr(0, 10) == date)
            result.push_back(t);
    }
    // 按开始时间升序排序
    std::sort(result.begin(), result.end());
    return result;
}

std::vector<Task> TaskManager::getTasksByMonth(const std::string& yearMonth) const {
    std::lock_guard<std::mutex> lock(mtx);
    std::vector<Task> result;
    for (const auto& t : tasks) {
        // 比较YYYY-MM判断是否同一个月
        if (t.startTime.size() >= 7 && t.startTime.substr(0, 7) == yearMonth)
            result.push_back(t);
    }
    std::sort(result.begin(), result.end());
    return result;
}

std::vector<Task> TaskManager::getUpcomingReminders() const {
    std::lock_guard<std::mutex> lock(mtx);
    // 获取当前系统时间
    auto now = std::chrono::system_clock::now();
    auto now_c = std::chrono::system_clock::to_time_t(now);
    std::tm* tm_now = std::localtime(&now_c);

    // 当前时间的格式化字符串
    char nowBuf[20];
    std::strftime(nowBuf, sizeof(nowBuf), "%Y-%m-%d %H:%M", tm_now);
    std::string nowStr(nowBuf);

    // 计算前后 1 分钟的时间点，用于划定提醒触发窗口
    time_t now_t = std::mktime(tm_now);
    time_t oneMinAgo_t = now_t - 60;   // 1 分钟前
    time_t oneMinLater_t = now_t + 60; // 1 分钟后

    std::tm* tm_ago = std::localtime(&oneMinAgo_t);
    std::tm* tm_later = std::localtime(&oneMinLater_t);

    char agoBuf[20], laterBuf[20];
    std::strftime(agoBuf, sizeof(agoBuf), "%Y-%m-%d %H:%M", tm_ago);
    std::strftime(laterBuf, sizeof(laterBuf), "%Y-%m-%d %H:%M", tm_later);

    std::string agoStr(agoBuf);   // 窗口起始
    std::string laterStr(laterBuf); // 窗口结束

    // 筛选提醒时间在范围内的任务
    std::vector<Task> result;
    for (const auto& t : tasks) {
        if (t.remindTime.empty()) continue; // 未设提醒则跳过
        if (t.remindTime >= agoStr && t.remindTime <= laterStr)
            result.push_back(t);
    }
    return result;
}

//数据本地储存

void TaskManager::loadFromFile(const std::string& taskFile, const std::string& userFile) {
    std::lock_guard<std::mutex> lock(mtx);//锁防止出错
    // 从 JSON 文件中加载数据
    tasks = Storage::loadTasks(taskFile);
    users = Storage::loadUsers(userFile);

    // 确保新分配的任务 ID 不会与原数据冲突
    nextId = 1;
    for (const auto& t : tasks) {
        if (t.id >= nextId) nextId = t.id + 1;
    }
}
//包裹函数
void TaskManager::saveToFile(const std::string& taskFile, const std::string& userFile) {
    std::lock_guard<std::mutex> lock(mtx);
    Storage::saveTasks(taskFile, tasks);
    Storage::saveUsers(userFile, users);
}



//储存用户以及任务的函数
void TaskManager::loadFromUserFile(const std::string& baseDir,
                                     const std::string& userFile,
                                     const std::string& username) {
    setCurrentUser(username);
    std::string taskFile = baseDir + "/tasks_" + username + ".json";
    loadFromFile(taskFile, userFile);
}

void TaskManager::saveToUserFile(const std::string& baseDir,
                                  const std::string& userFile) {
    if (currentUser_.empty()) return;
    std::lock_guard<std::mutex> lock(mtx);
    std::string taskFile = baseDir + "/tasks_" + currentUser_ + ".json";
    Storage::saveTasks(taskFile, tasks);
    Storage::saveUsers(userFile, users);
}

