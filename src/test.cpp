// test.cpp
// 完整的测试程序，用于测试 Storage、TaskManager、Task、User 的所有功能

#include "core/TaskManager.h"
#include "core/Storage.h"
#include "core/Task.h"
#include "core/User.h"
#include <iostream>
#include <iomanip>
#include <vector>
#include <string>
#include <cassert>

// 颜色输出辅助（可选）
#define COLOR_GREEN   "\033[32m"
#define COLOR_RED     "\033[31m"
#define COLOR_YELLOW  "\033[33m"
#define COLOR_BLUE    "\033[34m"
#define COLOR_RESET   "\033[0m"

// 测试计数器
static int testsPassed = 0;
static int testsFailed = 0;

// 测试断言宏
#define TEST_ASSERT(condition, message) \
    do { \
        if (condition) { \
            std::cout << COLOR_GREEN << "[PASS] " << COLOR_RESET << message << std::endl; \
            testsPassed++; \
        } else { \
            std::cout << COLOR_RED << "[FAIL] " << COLOR_RESET << message << std::endl; \
            testsFailed++; \
        } \
    } while(0)

#define TEST_ASSERT_EQUAL(expected, actual, message) \
    do { \
        if ((expected) == (actual)) { \
            std::cout << COLOR_GREEN << "[PASS] " << COLOR_RESET << message << std::endl; \
            testsPassed++; \
        } else { \
            std::cout << COLOR_RED << "[FAIL] " << COLOR_RESET << message \
                      << " (expected: " << (expected) << ", actual: " << (actual) << ")" << std::endl; \
            testsFailed++; \
        } \
    } while(0)

// 打印分隔线
void printSeparator(const std::string& title = "") {
    std::cout << "\n" << COLOR_BLUE << "========================================" << COLOR_RESET << std::endl;
    if (!title.empty()) {
        std::cout << COLOR_BLUE << "  " << title << COLOR_RESET << std::endl;
        std::cout << COLOR_BLUE << "========================================" << COLOR_RESET << std::endl;
    }
}

// 打印任务列表
void printTasks(const std::vector<Task>& tasks, const std::string& title = "Tasks") {
    std::cout << COLOR_YELLOW << "\n--- " << title << " (" << tasks.size() << " items) ---" << COLOR_RESET << std::endl;
    for (const auto& t : tasks) {
        std::cout << "  ID:" << t.id 
                  << " | " << t.name
                  << " | " << t.startTime
                  << " | Priority:" << t.priority
                  << " | Category:" << t.category
                  << " | Remind:" << (t.remindTime.empty() ? "(none)" : t.remindTime)
                  << std::endl;
    }
}

// 打印用户列表
void printUsers(const std::vector<User>& users, const std::string& title = "Users") {
    std::cout << COLOR_YELLOW << "\n--- " << title << " (" << users.size() << " items) ---" << COLOR_RESET << std::endl;
    for (const auto& u : users) {
        std::cout << "  Username:" << u.username 
                  << " | Hash:" << u.passwordHash.substr(0, 16) << "..." << std::endl;
    }
}

// ============================================================
// 测试用例
// ============================================================

// 1. 测试 Task 类
void testTaskClass() {
    printSeparator("Testing Task Class");
    
    // 默认构造
    Task defaultTask;
    TEST_ASSERT(defaultTask.id == 0, "Default task id = 0");
    TEST_ASSERT(defaultTask.name.empty(), "Default task name is empty");
    TEST_ASSERT(defaultTask.priority == 2, "Default priority = 2");
    TEST_ASSERT(defaultTask.category == "学习", "Default category = '学习'");
    
    // 含参构造
    Task t(1, "Meeting", "2026-07-23 14:00", 1, "工作", "2026-07-23 13:50");
    TEST_ASSERT(t.id == 1, "Parameterized constructor sets id correctly");
    TEST_ASSERT(t.name == "Meeting", "Parameterized constructor sets name correctly");
    TEST_ASSERT(t.startTime == "2026-07-23 14:00", "Parameterized constructor sets startTime correctly");
    TEST_ASSERT(t.priority == 1, "Parameterized constructor sets priority correctly");
    TEST_ASSERT(t.category == "工作", "Parameterized constructor sets category correctly");
    TEST_ASSERT(t.remindTime == "2026-07-23 13:50", "Parameterized constructor sets remindTime correctly");
    
    // Getters/Setters
    t.setId(999);
    TEST_ASSERT(t.getId() == 999, "setId/getId works");
    
    t.setName("New Name");
    TEST_ASSERT(t.getName() == "New Name", "setName/getName works");
    
    t.setPriority(3);
    TEST_ASSERT(t.getPriority() == 3, "setPriority/getPriority works");
    
    // operator<
    Task t1(1, "A", "2026-07-23 10:00", 2, "学习", "");
    Task t2(2, "B", "2026-07-23 14:00", 2, "学习", "");
    TEST_ASSERT(t1 < t2, "operator< works (earlier < later)");
    TEST_ASSERT(!(t2 < t1), "operator< works (later < earlier is false)");
}

// 2. 测试 User 类
void testUserClass() {
    printSeparator("Testing User Class");
    
    // 默认构造
    User defaultUser;
    TEST_ASSERT(defaultUser.username.empty(), "Default username is empty");
    TEST_ASSERT(defaultUser.passwordHash.empty(), "Default passwordHash is empty");
    
    // 含参构造 + 密码哈希
    User u1("alice", "password123");
    TEST_ASSERT(u1.username == "alice", "Constructor sets username correctly");
    TEST_ASSERT(!u1.passwordHash.empty(), "Constructor hashes password");
    TEST_ASSERT(u1.passwordHash.length() == 64, "SHA256 hash length = 64");
    
    // 密码验证
    TEST_ASSERT(u1.checkPassword("password123"), "checkPassword works with correct password");
    TEST_ASSERT(!u1.checkPassword("wrongpassword"), "checkPassword rejects wrong password");
    
    // 不同用户相同密码产生不同哈希（用户名不同不参与哈希计算，但这里只是验证哈希功能）
    User u2("bob", "password123");
    TEST_ASSERT(u1.passwordHash == u2.passwordHash, "Same password produces same hash (as expected)");
}

// 3. 测试 Storage 类 (save + load)
void testStorage() {
    printSeparator("Testing Storage Class");
    
    std::string taskFile = "test_tasks.json";
    std::string userFile = "test_users.json";
    std::string allFile = "test_all.json";
    
    // 准备测试数据
    std::vector<Task> tasks;
    tasks.emplace_back(1, "Task A", "2026-07-23 10:00", 1, "工作", "2026-07-23 09:50");
    tasks.emplace_back(2, "Task B", "2026-07-23 14:30", 2, "学习", "2026-07-23 14:20");
    tasks.emplace_back(3, "Task C", "2026-07-24 09:00", 3, "运动", "");
    
    std::vector<User> users;
    users.emplace_back("alice", "pass123");
    users.emplace_back("bob", "secure456");
    
    // ---- saveTasks / loadTasks ----
    bool saved = Storage::saveTasks(taskFile, tasks);
    TEST_ASSERT(saved, "saveTasks returns true");
    
    auto loadedTasks = Storage::loadTasks(taskFile);
    TEST_ASSERT_EQUAL(tasks.size(), loadedTasks.size(), "loadTasks loads correct number of tasks");
    if (loadedTasks.size() == tasks.size()) {
        for (size_t i = 0; i < tasks.size(); i++) {
            TEST_ASSERT(loadedTasks[i].id == tasks[i].id, "Task id matches");
            TEST_ASSERT(loadedTasks[i].name == tasks[i].name, "Task name matches");
            TEST_ASSERT(loadedTasks[i].startTime == tasks[i].startTime, "Task startTime matches");
            TEST_ASSERT(loadedTasks[i].priority == tasks[i].priority, "Task priority matches");
            TEST_ASSERT(loadedTasks[i].category == tasks[i].category, "Task category matches");
            TEST_ASSERT(loadedTasks[i].remindTime == tasks[i].remindTime, "Task remindTime matches");
        }
    }
    
    // ---- saveUsers / loadUsers ----
    saved = Storage::saveUsers(userFile, users);
    TEST_ASSERT(saved, "saveUsers returns true");
    
    auto loadedUsers = Storage::loadUsers(userFile);
    TEST_ASSERT_EQUAL(users.size(), loadedUsers.size(), "loadUsers loads correct number of users");
    if (loadedUsers.size() == users.size()) {
        for (size_t i = 0; i < users.size(); i++) {
            TEST_ASSERT(loadedUsers[i].username == users[i].username, "Username matches");
            TEST_ASSERT(loadedUsers[i].passwordHash == users[i].passwordHash, "Password hash matches");
        }
    }
    
    // ---- saveAll / loadAll ----
    bool savedAll = Storage::saveAll(allFile, tasks, users);
    TEST_ASSERT(savedAll, "saveAll returns true");
    
    std::vector<Task> loadedAllTasks;
    std::vector<User> loadedAllUsers;
    bool loadedAll = Storage::loadAll(allFile, loadedAllTasks, loadedAllUsers);
    TEST_ASSERT(loadedAll, "loadAll returns true");
    TEST_ASSERT_EQUAL(tasks.size(), loadedAllTasks.size(), "loadAll loads correct tasks count");
    TEST_ASSERT_EQUAL(users.size(), loadedAllUsers.size(), "loadAll loads correct users count");
    
    // ---- 边界情况 ----
    auto emptyTasks = Storage::loadTasks("nonexistent_file.json");
    TEST_ASSERT(emptyTasks.empty(), "loadTasks returns empty for nonexistent file");
    
    auto emptyUsers = Storage::loadUsers("nonexistent_file.json");
    TEST_ASSERT(emptyUsers.empty(), "loadUsers returns empty for nonexistent file");
    
    // 清理测试文件
    std::remove(taskFile.c_str());
    std::remove(userFile.c_str());
    std::remove(allFile.c_str());
}

// 4. 测试 TaskManager 核心功能
void testTaskManager() {
    printSeparator("Testing TaskManager Core Functions");
    
    TaskManager tm;
    
    // ---- 用户注册 ----
    bool reg1 = tm.registerUser("alice", "password123");
    TEST_ASSERT(reg1, "registerUser succeeds for new user");
    
    bool reg2 = tm.registerUser("alice", "different"); // 重复用户名
    TEST_ASSERT(!reg2, "registerUser fails for duplicate username");
    
    bool reg3 = tm.registerUser("bob", "secure456");
    TEST_ASSERT(reg3, "registerUser succeeds for another new user");
    
    // ---- 用户登录 ----
    bool login1 = tm.login("alice", "password123");
    TEST_ASSERT(login1, "login succeeds with correct credentials");
    
    bool login2 = tm.login("alice", "wrong");
    TEST_ASSERT(!login2, "login fails with wrong password");
    
    bool login3 = tm.login("nonexistent", "password123");
    TEST_ASSERT(!login3, "login fails for nonexistent user");
    
    // ---- 添加任务 ----
    Task t1(0, "Meeting", "2026-07-23 14:00", 1, "工作", "2026-07-23 13:50");
    int id1 = tm.addTask(t1);
    TEST_ASSERT(id1 > 0, "addTask returns positive id");
    TEST_ASSERT_EQUAL(1, id1, "First task gets id 1");
    
    // 添加重复任务（同名+同时间）
    Task t1_dup(0, "Meeting", "2026-07-23 14:00", 2, "学习", "");
    int id_dup = tm.addTask(t1_dup);
    TEST_ASSERT_EQUAL(-1, id_dup, "addTask returns -1 for duplicate task");
    
    // 添加提醒时间晚于开始时间的任务（应失败）
    Task t_invalid(0, "Invalid Task", "2026-07-23 14:00", 2, "学习", "2026-07-23 15:00");
    int id_invalid = tm.addTask(t_invalid);
    TEST_ASSERT_EQUAL(-2, id_invalid, "addTask returns -2 for invalid remind time");
    
    // 添加更多任务
    Task t2(0, "Study", "2026-07-23 10:00", 2, "学习", "2026-07-23 09:55");
    int id2 = tm.addTask(t2);
    TEST_ASSERT_EQUAL(2, id2, "Second task gets id 2");
    
    Task t3(0, "Exercise", "2026-07-24 09:00", 3, "运动", "");
    int id3 = tm.addTask(t3);
    TEST_ASSERT_EQUAL(3, id3, "Third task gets id 3");
    
    // 验证任务数量
    const auto& allTasks = tm.getTasks();
    TEST_ASSERT_EQUAL(3, (int)allTasks.size(), "Total tasks count = 3");
    printTasks(allTasks, "All Tasks");
    
    // ---- 按日期查询 ----
    auto tasksByDate = tm.getTasksByDate("2026-07-23");
    TEST_ASSERT_EQUAL(2, (int)tasksByDate.size(), "getTasksByDate returns 2 tasks for 2026-07-23");
    printTasks(tasksByDate, "Tasks on 2026-07-23");
    
    // ---- 按月查询 ----
    auto tasksByMonth = tm.getTasksByMonth("2026-07");
    TEST_ASSERT_EQUAL(3, (int)tasksByMonth.size(), "getTasksByMonth returns 3 tasks for 2026-07");
    printTasks(tasksByMonth, "Tasks in 2026-07");
    
    // ---- 更新提醒时间 ----
    bool updated = tm.updateTaskRemindTime(id2, "2026-07-23 09:50");
    TEST_ASSERT(updated, "updateTaskRemindTime succeeds");
    
    // 验证更新
    bool found = false;
    for (const auto& t : tm.getTasks()) {
        if (t.id == id2 && t.remindTime == "2026-07-23 09:50") {
            found = true;
            break;
        }
    }
    TEST_ASSERT(found, "Remind time updated correctly");
    
    // ---- 删除任务 ----
    bool deleted = tm.deleteTask(id3);
    TEST_ASSERT(deleted, "deleteTask succeeds");
    TEST_ASSERT_EQUAL(2, (int)tm.getTasks().size(), "After deletion, tasks count = 2");
    
    bool deletedAgain = tm.deleteTask(id3);
    TEST_ASSERT(!deletedAgain, "deleteTask fails for already deleted task");
    
    // ---- 显示最终任务列表 ----
    printTasks(tm.getTasks(), "Final Tasks");
    printUsers(tm.getUsers(), "Final Users");
}

// 5. 测试数据持久化 (TaskManager + Storage 集成)
void testTaskManagerPersistence() {
    printSeparator("Testing TaskManager Persistence");
    
    std::string taskFile = "tm_test_tasks.json";
    std::string userFile = "tm_test_users.json";
    
    // ----- 场景1: 保存数据 -----
    {
        TaskManager tm;
        tm.registerUser("alice", "pass123");
        tm.registerUser("bob", "pass456");
        
        Task t1(0, "Task 1", "2026-07-23 10:00", 1, "工作", "2026-07-23 09:50");
        Task t2(0, "Task 2", "2026-07-23 14:00", 2, "学习", "");
        tm.addTask(t1);
        tm.addTask(t2);
        
        tm.saveToFile(taskFile, userFile);
        std::cout << "Saved " << tm.getTasks().size() << " tasks and " 
                  << tm.getUsers().size() << " users to files." << std::endl;
    }
    
    // ----- 场景2: 加载数据到新的 TaskManager -----
    {
        TaskManager tm2;
        tm2.loadFromFile(taskFile, userFile);
        
        const auto& tasks = tm2.getTasks();
        const auto& users = tm2.getUsers();
        
        TEST_ASSERT_EQUAL(2, (int)tasks.size(), "Loaded 2 tasks");
        TEST_ASSERT_EQUAL(2, (int)users.size(), "Loaded 2 users");
        
        // 验证任务数据完整性
        bool found1 = false, found2 = false;
        for (const auto& t : tasks) {
            if (t.name == "Task 1" && t.startTime == "2026-07-23 10:00") found1 = true;
            if (t.name == "Task 2" && t.startTime == "2026-07-23 14:00") found2 = true;
        }
        TEST_ASSERT(found1 && found2, "Loaded tasks have correct data");
        
        // 验证用户数据完整性
        bool foundUser1 = false, foundUser2 = false;
        for (const auto& u : users) {
            if (u.username == "alice") foundUser1 = true;
            if (u.username == "bob") foundUser2 = true;
        }
        TEST_ASSERT(foundUser1 && foundUser2, "Loaded users have correct data");
        
        printTasks(tasks, "Loaded Tasks");
        printUsers(users, "Loaded Users");
    }
    
    // 清理测试文件
    std::remove(taskFile.c_str());
    std::remove(userFile.c_str());
}

// 6. 测试 isRemindTimeAfterStart 工具函数
void testRemindTimeCheck() {
    printSeparator("Testing isRemindTimeAfterStart");
    
    // 提醒时间早于开始时间 → false
    bool r1 = TaskManager::isRemindTimeAfterStart("2026-07-23 09:00", "2026-07-23 10:00");
    TEST_ASSERT(!r1, "Remind earlier than start -> false");
    
    // 提醒时间等于开始时间 → false
    bool r2 = TaskManager::isRemindTimeAfterStart("2026-07-23 10:00", "2026-07-23 10:00");
    TEST_ASSERT(!r2, "Remind equals start -> false");
    
    // 提醒时间晚于开始时间 → true
    bool r3 = TaskManager::isRemindTimeAfterStart("2026-07-23 11:00", "2026-07-23 10:00");
    TEST_ASSERT(r3, "Remind later than start -> true");
    
    // 跨天测试
    bool r4 = TaskManager::isRemindTimeAfterStart("2026-07-24 09:00", "2026-07-23 18:00");
    TEST_ASSERT(r4, "Remind next day later than start -> true");
    
    bool r5 = TaskManager::isRemindTimeAfterStart("2026-07-22 23:00", "2026-07-23 01:00");
    TEST_ASSERT(!r5, "Remind previous day earlier than start -> false");
    
    // 跨月测试
    bool r6 = TaskManager::isRemindTimeAfterStart("2026-08-01 00:00", "2026-07-31 23:59");
    TEST_ASSERT(r6, "Remind next month later -> true");
    
    std::cout << "All remind time checks passed!" << std::endl;
}

// 7. 测试边界和错误处理
void testEdgeCases() {
    printSeparator("Testing Edge Cases");
    
    TaskManager tm;
    
    // 空任务列表查询
    auto emptyDate = tm.getTasksByDate("2026-07-23");
    TEST_ASSERT(emptyDate.empty(), "getTasksByDate returns empty for no tasks");
    
    auto emptyMonth = tm.getTasksByMonth("2026-07");
    TEST_ASSERT(emptyMonth.empty(), "getTasksByMonth returns empty for no tasks");
    
    // 添加任务时，remindTime 为空字符串（不提醒）
    Task t_no_remind(0, "No Remind", "2026-07-23 10:00", 2, "学习", "");
    int id = tm.addTask(t_no_remind);
    TEST_ASSERT(id > 0, "Task with empty remindTime can be added");
    
    // 更新不存在的任务
    bool updateFail = tm.updateTaskRemindTime(99999, "2026-07-23 12:00");
    TEST_ASSERT(!updateFail, "updateTaskRemindTime fails for non-existent task");
    
    // 删除不存在的任务
    bool deleteFail = tm.deleteTask(99999);
    TEST_ASSERT(!deleteFail, "deleteTask fails for non-existent task");
    
    // 密码哈希一致性
    User u1("testuser", "mypassword");
    User u2("testuser", "mypassword");
    TEST_ASSERT(u1.passwordHash == u2.passwordHash, "Same password produces same hash");
    
    // 不同密码产生不同哈希
    User u3("testuser", "different");
    TEST_ASSERT(u1.passwordHash != u3.passwordHash, "Different passwords produce different hashes");
}

// ============================================================
// 主函数
// ============================================================

int main() {
    std::cout << COLOR_BLUE << "\n"
              << "╔══════════════════════════════════════════╗\n"
              << "║      ECalender 单元测试程序             ║\n"
              << "║      Testing Storage, TaskManager, ...  ║\n"
              << "╚══════════════════════════════════════════╝\n"
              << COLOR_RESET << std::endl;
    
    // 运行所有测试
    testTaskClass();
    testUserClass();
    testStorage();
    testTaskManager();
    testTaskManagerPersistence();
    testRemindTimeCheck();
    testEdgeCases();
    
    // 输出测试结果汇总
    printSeparator("Test Summary");
    std::cout << "Total tests: " << (testsPassed + testsFailed) << std::endl;
    std::cout << COLOR_GREEN << "Passed: " << testsPassed << COLOR_RESET << std::endl;
    std::cout << COLOR_RED << "Failed: " << testsFailed << COLOR_RESET << std::endl;
    
    if (testsFailed == 0) {
        std::cout << COLOR_GREEN << "\n🎉 All tests passed! 🎉\n" << COLOR_RESET << std::endl;
        return 0;
    } else {
        std::cout << COLOR_RED << "\n❌ Some tests failed.\n" << COLOR_RESET << std::endl;
        return 1;
    }
}