//Storage实现

#include "core/Storage.h"
#include <fstream>
#include <sstream>
#include <algorithm>


/*jsonEscape - JSON 字符串转义
 对字符串中的双引号 " 和反斜杠 \\ 进行转义处理，
确保生成的 JSON 符合标准格式
 */
static std::string jsonEscape(const std::string& s) {
    std::string out;
    out.reserve(s.size() + 2);
    for (char c : s) {
        if (c == '"' || c == '\\') { out += '\\'; }
        out += c;
    }
    return out;
}

/*
writeTask - 将 Task 对象写入输出流
os     输出流
t      待写入的任务
indent 缩进空格数
 */
static void writeTask(std::ostream& os, const Task& t, int indent) {
    std::string pad(indent, ' ');
    os << pad << "{\n";
    os << pad << "  \"id\":" << t.id << ",\n";
    os << pad << "  \"name\":\"" << jsonEscape(t.name) << "\",\n";
    os << pad << "  \"startTime\":\"" << jsonEscape(t.startTime) << "\",\n";
    os << pad << "  \"priority\":" << t.priority << ",\n";
    os << pad << "  \"category\":\"" << jsonEscape(t.category) << "\",\n";
    os << pad << "  \"remindTime\":\"" << jsonEscape(t.remindTime) << "\"\n";
    os << pad << "}";
}
/*
writeUser - 将 User 对象写入输出流
os     输出流
u      待写入的用户
indent 缩进空格数
 */
static void writeUser(std::ostream& os, const User& u, int indent) {
    std::string pad(indent, ' ');
    os << pad << "{\n";
    os << pad << "  \"username\":\"" << jsonEscape(u.username) << "\",\n";
    os << pad << "  \"passwordHash\":\"" << jsonEscape(u.passwordHash) << "\"\n";
    os << pad << "}";
}


//储存函数
bool Storage::saveTasks(const std::string& filename, const std::vector<Task>& tasks) {
    std::ofstream ofs(filename);
    if (!ofs) return false;
    ofs << "{\n  \"tasks\": [\n";
    for (size_t i = 0; i < tasks.size(); i++) {
        writeTask(ofs, tasks[i], 4);
        if (i + 1 < tasks.size()) ofs << ",";
        ofs << "\n";
    }
    ofs << "  ]\n}\n";
    return true;
}

bool Storage::saveUsers(const std::string& filename, const std::vector<User>& users) {
    std::ofstream ofs(filename);
    if (!ofs) return false;
    ofs << "{\n  \"users\": [\n";
    for (size_t i = 0; i < users.size(); i++) {
        writeUser(ofs, users[i], 4);
        if (i + 1 < users.size()) ofs << ",";
        ofs << "\n";
    }
    ofs << "  ]\n}\n";
    return true;
}

bool Storage::saveAll(const std::string& filename,
                      const std::vector<Task>& tasks,
                      const std::vector<User>& users) {
    std::ofstream ofs(filename);
    if (!ofs) return false;
    ofs << "{\n"
        << "  \"tasks\": [\n";
    for (size_t i = 0; i < tasks.size(); i++) {
        writeTask(ofs, tasks[i], 4);
        if (i + 1 < tasks.size()) ofs << ",";
        ofs << "\n";
    }
    ofs << "  ],\n"
        << "  \"users\": [\n";
    for (size_t i = 0; i < users.size(); i++) {
        writeUser(ofs, users[i], 4);
        if (i + 1 < users.size()) ofs << ",";
        ofs << "\n";
    }
    ofs << "  ]\n}\n";
    return true;
}


/* trim - 工具函数，用于去除字符串首尾的空白字符
 s 输入字符串
去除首尾空白后的子串
 */
static std::string trim(const std::string& s) {
    size_t start = s.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    size_t end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
}

/*
skipWS - 工具函数，用于跳过空白字符，返回第一个非空白字符位置
s   源字符串
pos 起始搜索位置
return 第一个非空白字符的位置
 */
static size_t skipWS(const std::string& s, size_t pos) {
    while (pos < s.size() && (s[pos] == ' ' || s[pos] == '\t' || s[pos] == '\n' || s[pos] == '\r'))
        pos++;
    return pos;
}


//parseJsonString - 解析 JSON 字符串值
 
static std::pair<std::string, size_t> parseJsonString(const std::string& s, size_t pos) {
    pos = skipWS(s, pos);
    if (pos >= s.size() || s[pos] != '"') return {"", pos};
    pos++; // 跳过左引号
    std::string val;
    while (pos < s.size() && s[pos] != '"') {
        if (s[pos] == '\\') {
            pos++;
            if (pos < s.size()) {
                if (s[pos] == '"') val += '"';
                else if (s[pos] == '\\') val += '\\';
                else if (s[pos] == 'n') val += '\n';
                else if (s[pos] == 't') val += '\t';
                else val += s[pos];
            }
        } else {
            val += s[pos];
        }
        pos++;
    }
    if (pos < s.size()) pos++; // 跳过右引号
    return {val, pos};
}

//parseJsonInt - 解析 JSON 整数值
 
static std::pair<int, size_t> parseJsonInt(const std::string& s, size_t pos) {
    pos = skipWS(s, pos);
    bool neg = false;
    if (pos < s.size() && s[pos] == '-') { neg = true; pos++; }
    int val = 0;
    while (pos < s.size() && s[pos] >= '0' && s[pos] <= '9') {
        val = val * 10 + (s[pos] - '0');
        pos++;
    }
    if (neg) val = -val;
    return {val, pos};
}

 //工具函数：skipJsonValue - 跳过整个 JSON 值（对象、数组、字符串、数值）

static size_t skipJsonValue(const std::string& s, size_t pos) {
    pos = skipWS(s, pos);
    if (pos >= s.size()) return pos;
    char c = s[pos];
    if (c == '"') {
        auto [_, next] = parseJsonString(s, pos);
        return next;
    }
    if (c == '-' || (c >= '0' && c <= '9')) {
        auto [_, next] = parseJsonInt(s, pos);
        return next;
    }
    if (c == '{') {
        // 跳过花括号对象，记录嵌套深度
        int depth = 0;
        while (pos < s.size()) {
            if (s[pos] == '{') depth++;
            else if (s[pos] == '}') { depth--; if (depth == 0) return pos + 1; }
            else if (s[pos] == '"') {
                // 跳过字符串以避免花括号内引号中的误判
                auto [_, next] = parseJsonString(s, pos);
                pos = next;
                continue;
            }
            pos++;
        }
    }
    if (c == '[') {
        // 跳过方括号数组，记录嵌套深度
        int depth = 0;
        while (pos < s.size()) {
            if (s[pos] == '[') depth++;
            else if (s[pos] == ']') { depth--; if (depth == 0) return pos + 1; }
            else if (s[pos] == '"') {
                auto [_, next] = parseJsonString(s, pos);
                pos = next;
                continue;
            }
            pos++;
        }
    }
    return pos;
}

// findKey -按照key查找
static size_t findKey(const std::string& s, size_t pos, const std::string& key) {
    pos = skipWS(s, pos);
    if (pos >= s.size() || s[pos] != '{') return std::string::npos;
    pos++; // 跳过左花括号 {
    while (true) {
        pos = skipWS(s, pos);
        if (pos >= s.size() || s[pos] == '}') return std::string::npos;
        auto [k, next] = parseJsonString(s, pos);
        if (k.empty()) return std::string::npos;
        next = skipWS(s, next);
        if (next < s.size() && s[next] == ':') next++;
        else return std::string::npos;
        next = skipWS(s, next);
        if (k == key) return next; // 找到了匹配的 key，返回值的起始位置
        // 不匹配则跳过当前值，继续查找下一个键值对
        next = skipJsonValue(s, next);
        next = skipWS(s, next);
        if (next < s.size() && s[next] == ',') next++;
        pos = next;
    }
}

// 类型别名：一个 JSON 对象的键值对集合
using FieldMap = std::vector<std::pair<std::string, std::string>>;

/*
 parseJsonArrayOfObjects - 解析 JSON 对象数组 
从 pos 位置开始解析一个 JSON 数组（以 [ 开头），数组中每个元素是一个 JSON 对象 {}，提取每个对象中的所有键值对返回。
s   源字符串
pos 起始位置（应指向 [）
return 解析出的对象列表，每个对象是一个键值对列表
 */
static std::vector<FieldMap> parseJsonArrayOfObjects(const std::string& s, size_t pos) {
    std::vector<FieldMap> result;
    pos = skipWS(s, pos);
    if (pos >= s.size() || s[pos] != '[') return result;
    pos++; // 跳过左方括号 [
    while (true) {
        pos = skipWS(s, pos);
        if (pos >= s.size() || s[pos] == ']') break;
        if (s[pos] != '{') { pos++; continue; }
        // 定位并截取当前对象的完整内容
        FieldMap obj;
        size_t objEnd = pos + 1;
        int depth = 1;
        while (objEnd < s.size() && depth > 0) {
            if (s[objEnd] == '{') depth++;
            else if (s[objEnd] == '}') depth--;
            else if (s[objEnd] == '"') {
                auto [_, next] = parseJsonString(s, objEnd);
                objEnd = next;
                continue;
            }
            objEnd++;
        }
        std::string objStr = s.substr(pos, objEnd - pos);

        // 在对象内部逐个提取键值对
        size_t op = skipWS(objStr, 1); // 跳过 {
        while (true) {
            op = skipWS(objStr, op);
            if (op >= objStr.size() || objStr[op] == '}') break;
            auto [k, next] = parseJsonString(objStr, op);
            if (k.empty()) break;
            next = skipWS(objStr, next);
            if (next < objStr.size() && objStr[next] == ':') next++;
            else break;
            next = skipWS(objStr, next);
            // 获取值（字符串或整数）
            if (next < objStr.size() && objStr[next] == '"') {
                auto [v, vnext] = parseJsonString(objStr, next);
                obj.emplace_back(k, v);
                next = vnext;
            } else if (objStr[next] == '-' || (objStr[next] >= '0' && objStr[next] <= '9')) {
                auto [v, vnext] = parseJsonInt(objStr, next);
                obj.emplace_back(k, std::to_string(v));
                next = vnext;
            } else {
                next = skipJsonValue(objStr, next);
            }
            next = skipWS(objStr, next);
            if (next < objStr.size() && objStr[next] == ',') next++;
            op = next;
        }
        result.push_back(std::move(obj));
        pos = objEnd;
        pos = skipWS(s, pos);
        if (pos < s.size() && s[pos] == ',') pos++;
    }
    return result;
}

// parseTasksFromJSON - 从 JSON 字符串中解析出任务
static std::vector<Task> parseTasksFromJSON(const std::string& s) {
    size_t pos = findKey(s, 0, "tasks");
    if (pos == std::string::npos) return {};
    auto objects = parseJsonArrayOfObjects(s, pos);
    std::vector<Task> tasks;
    for (const auto& obj : objects) {
        Task t;
        for (const auto& [k, v] : obj) {
            if (k == "id") t.id = std::stoi(v);
            else if (k == "name") t.name = v;
            else if (k == "startTime") t.startTime = v;
            else if (k == "priority") t.priority = std::stoi(v);
            else if (k == "category") t.category = v;
            else if (k == "remindTime") t.remindTime = v;
        }
        // 对非法或空值的字段使用默认值
        if (t.priority < 1 || t.priority > 3) t.priority = 2;
        if (t.category.empty()) t.category = "学习";
        tasks.push_back(t);
    }
    return tasks;
}

// parseUsersFromJSON - 从 JSON 字符串中解析出用户列表

static std::vector<User> parseUsersFromJSON(const std::string& s) {
    size_t pos = findKey(s, 0, "users");
    if (pos == std::string::npos) return {};
    auto objects = parseJsonArrayOfObjects(s, pos);
    std::vector<User> users;
    for (const auto& obj : objects) {
        User u;
        for (const auto& [k, v] : obj) {
            if (k == "username") u.username = v;
            else if (k == "passwordHash") u.passwordHash = v;
        }
        users.push_back(u);
    }
    return users;
}


// Storage 公开接口


std::vector<Task> Storage::loadTasks(const std::string& filename) {
    std::ifstream ifs(filename);
    if (!ifs) return {};
    std::stringstream ss;
    ss << ifs.rdbuf();
    return parseTasksFromJSON(ss.str());
}

std::vector<User> Storage::loadUsers(const std::string& filename) {
    std::ifstream ifs(filename);
    if (!ifs) return {};
    std::stringstream ss;
    ss << ifs.rdbuf();
    return parseUsersFromJSON(ss.str());
}

bool Storage::loadAll(const std::string& filename,
                      std::vector<Task>& outTasks,
                      std::vector<User>& outUsers) {
    std::ifstream ifs(filename);
    if (!ifs) return false;
    std::stringstream ss;
    ss << ifs.rdbuf();
    std::string content = ss.str();
    outTasks = parseTasksFromJSON(content);
    outUsers = parseUsersFromJSON(content);
    return true;
}

