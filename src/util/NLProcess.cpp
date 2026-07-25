/**
 * @file NLProcessor.cpp
 * @brief 自然语言处理器实现
 *
 * 两阶段策略：
 *   1. DeepSeek Chat API（libcurl）— 精确解析
 *   2. 正则表达式（纯字符串处理）— 零依赖 fallback
 *
 * DeepSeek API 需要：
 *   - libcurl4-openssl-dev（编译前安装）
 *   - 环境变量 DEEPSEEK_API_KEY（运行时提供，从 .env 自动加载）
 *
 * 如果不满足上述条件，自动使用正则 fallback。
 */

#include "util/NLProcess.h"

#include <cstdlib>
#include <cstring>
#include <ctime>
#include <cstdio>
#include <string>
#include <algorithm>
#include <sstream>
#include <iostream>
#include <regex>

// ================================================================
// HAS_CURL 编译标志
// 在 CMakeLists.txt 中通过 find_package(CURL) 设置
// ================================================================
#ifndef HAS_CURL
#define HAS_CURL 0
#endif

#if HAS_CURL
#include <curl/curl.h>
#endif

// ================================================================
// DeepSeek API 常量
// ================================================================
static const char* DEEPSEEK_API_URL = "https://api.deepseek.com/chat/completions";
static const char* DEEPSEEK_MODEL = "deepseek-chat";

// ================================================================
// libcurl 写回调
// ================================================================
#if HAS_CURL
static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    size_t totalSize = size * nmemb;
    std::string* str = static_cast<std::string*>(userp);
    str->append(static_cast<char*>(contents), totalSize);
    return totalSize;
}
#endif

// ================================================================
// 公开接口：parse()
// ================================================================

ParsedTask NLProcessor::parse(const std::string& text) {
    if (text.empty()) {
        ParsedTask result;
        result.valid = false;
        return result;
    }

    // 第一阶段：尝试 DeepSeek API
    ParsedTask result = parseWithDeepSeek(text);
    if (result.valid) {
        return result;
    }

    // 第二阶段：正则 fallback
    result = parseWithRegex(text);
    return result;
}

// ================================================================
// DeepSeek API 解析
// ================================================================

ParsedTask NLProcessor::parseWithDeepSeek(const std::string& text) {
    ParsedTask result;
    result.valid = false;

    const char* apiKey = std::getenv("DEEPSEEK_API_KEY");
    if (!apiKey || strlen(apiKey) == 0) {
        return result;
    }

    // 通过系统 curl 命令调用 DeepSeek API（比 libcurl 更可靠）
    // 构建请求体 JSON（转义用户文本中的特殊字符）
    std::string escapedText = text;
    auto replaceAll = [](std::string& s, const std::string& from, const std::string& to) {
        size_t pos = 0;
        while ((pos = s.find(from, pos)) != std::string::npos) {
            s.replace(pos, from.length(), to);
            pos += to.length();
        }
    };
    replaceAll(escapedText, "\\", "\\\\");
    replaceAll(escapedText, "\"", "\\\"");
    replaceAll(escapedText, "\n", "\\n");
    replaceAll(escapedText, "\r", "\\r");
    replaceAll(escapedText, "\t", "\\t");

    // 获取今天的日期
    time_t now = time(nullptr);
    struct tm* tm_now = localtime(&now);
    char todayBuf[20];
    strftime(todayBuf, sizeof(todayBuf), "%Y-%m-%d", tm_now);
    std::string todayStr(todayBuf);

    // 构建 JSON 请求体
    std::string postData = "{";
    postData += "\"model\":\"" + std::string(DEEPSEEK_MODEL) + "\",";
    postData += "\"messages\":[";
    postData += "{\"role\":\"system\",\"content\":\"你是一个日程管理助手。"
        "从用户的自然语言中提取任务信息，返回JSON格式："
        "{\\\"name\\\":\\\"任务名称\\\",\\\"startTime\\\":\\\"YYYY-MM-DD HH:MM\\\",\\\"priority\\\":1,\\\"category\\\":\\\"学习\\\"}"
        "。注意：今天是" + todayStr + "。如果用户说\\\"明天\\\"则日期+1天，\\\"后天\\\"则+2天。"
        "时间用24小时制。priority: 1=高优先级, 2=中优先级, 3=低优先级。"
        "category: 学习|娱乐|生活。"
        "如果用户提到提醒时间（如\\\"提前N分钟\\\"），计算后填入remindTime字段。"
        "只返回JSON，不要包含其他文字。\"}";
    postData += ",";
    postData += "{\"role\":\"user\",\"content\":\"" + escapedText + "\"}";
    postData += "],";
    postData += "\"temperature\":0.1";
    postData += "}";

    // 构建 curl 命令
    std::string cmd = "curl -s -w \\n%{http_code} ";
    cmd += "-H 'Content-Type: application/json' ";
    cmd += "-H 'Authorization: Bearer " + std::string(apiKey) + "' ";
    cmd += "-d '" + postData + "' ";
    cmd += "'" + std::string(DEEPSEEK_API_URL) + "' 2>/dev/null";

    // 执行 curl 并读取输出
    FILE* fp = popen(cmd.c_str(), "r");
    if (!fp) return result;

    std::string response;
    char buf[4096];
    size_t n;
    while ((n = fread(buf, 1, sizeof(buf), fp)) > 0) {
        response.append(buf, n);
    }
    int exitCode = pclose(fp);
    if (exitCode != 0 || response.empty()) return result;

    // 提取 JSON content（跳过 HTTP 状态码）
    std::string jsonContent;
    size_t braceStart = response.find('{');
    if (braceStart != std::string::npos) {
        size_t braceEnd = response.rfind('}');
        if (braceEnd != std::string::npos && braceEnd > braceStart) {
            jsonContent = response.substr(braceStart, braceEnd - braceStart + 1);
        }
    }
    if (jsonContent.empty()) return result;

    // 从 DeepSeek 响应中提取 choices[0].message.content
    // 响应格式：{"choices":[{"message":{"content":"{...}"}}]}
    std::string contentMarker = "\"content\":\"";
    size_t contentStart = jsonContent.find(contentMarker);
    if (contentStart == std::string::npos) {
        // 尝试带空格的格式
        contentMarker = "\"content\" : \"";
        contentStart = jsonContent.find(contentMarker);
        if (contentStart == std::string::npos) return result;
    }
    contentStart += contentMarker.size();

    // 提取 content 值（处理转义）
    std::string rawContent;
    bool esc = false;
    for (size_t i = contentStart; i < jsonContent.size(); i++) {
        char c = jsonContent[i];
        if (esc) {
            if (c == '"') rawContent += '"';
            else if (c == '\\') rawContent += '\\';
            else if (c == 'n') rawContent += '\n';
            else { rawContent += '\\'; rawContent += c; }
            esc = false;
        } else if (c == '\\') {
            esc = true;
        } else if (c == '"') {
            break;
        } else {
            rawContent += c;
        }
    }
    if (rawContent.empty()) return result;

    // 解析 content 中的 JSON
    result = parseJsonContent(rawContent);
    return result;
}

// ================================================================
// JSON 响应解析
// ================================================================

std::string NLProcessor::extractContentFromResponse(const std::string& json) {
    std::string marker = "\"content\"";
    size_t pos = json.find(marker);
    if (pos == std::string::npos) return "";

    pos = json.find(':', pos);
    if (pos == std::string::npos) return "";
    pos = json.find('"', pos);
    if (pos == std::string::npos) return "";
    pos++;

    std::string content;
    bool escaped = false;
    for (size_t i = pos; i < json.length(); ++i) {
        char c = json[i];
        if (escaped) {
            if (c == '"') content += '"';
            else if (c == '\\') content += '\\';
            else if (c == 'n') content += '\n';
            else if (c == 't') content += '\t';
            else { content += '\\'; content += c; }
            escaped = false;
        } else if (c == '\\') {
            escaped = true;
        } else if (c == '"') {
            break;
        } else {
            content += c;
        }
    }
    return content;
}

ParsedTask NLProcessor::parseJsonContent(const std::string& jsonContent) {
    ParsedTask result;
    result.valid = false;

    auto extractStr = [&](const std::string& key) -> std::string {
        std::string sk = "\"" + key + "\":\"";
        size_t start = jsonContent.find(sk);
        if (start == std::string::npos) {
            sk = "\"" + key + "\":'";
            start = jsonContent.find(sk);
            if (start == std::string::npos) {
                sk = "\"" + key + "\" : \"";
                start = jsonContent.find(sk);
                if (start == std::string::npos) {
                    sk = "\"" + key + "\" : '";
                    start = jsonContent.find(sk);
                    if (start == std::string::npos) return "";
                }
            }
        }
        start += sk.length();
        char quote = sk.back();
        size_t end = jsonContent.find(quote, start);
        while (end != std::string::npos && end > 0 && jsonContent[end-1] == '\\')
            end = jsonContent.find(quote, end + 1);
        if (end == std::string::npos) return "";
        return jsonContent.substr(start, end - start);
    };

    auto extractInt = [&](const std::string& key) -> int {
        std::string sk = "\"" + key + "\":";
        size_t start = jsonContent.find(sk);
        if (start == std::string::npos) {
            sk = "\"" + key + "\" : ";
            start = jsonContent.find(sk);
            if (start == std::string::npos) return -1;
        }
        start += sk.length();
        while (start < jsonContent.length() && jsonContent[start] == ' ') start++;
        size_t end = start;
        while (end < jsonContent.length() && jsonContent[end] >= '0' && jsonContent[end] <= '9') end++;
        if (end == start) return -1;
        return std::stoi(jsonContent.substr(start, end - start));
    };

    result.name = extractStr("name");
    result.startTime = extractStr("startTime");
    result.category = extractStr("category");

    int prio = extractInt("priority");
    if (prio >= 1 && prio <= 3) result.priority = prio;
    else result.priority = 2;

    result.remindTime = extractStr("remindTime");

    if (!result.name.empty() && !result.startTime.empty()) {
        result.valid = true;
    }
    return result;
}

// ================================================================
// 工具函数：中文数字解析
// ================================================================

static int singleChineseDigit(const std::string& text, size_t start) {
    if (start >= text.length()) return -1;
    unsigned char c = static_cast<unsigned char>(text[start]);
    if (c >= '0' && c <= '9') return (c - '0');
    static const char* digits[] = {"零","一","二","三","四","五","六","七","八","九"};
    static int vals[] = {0,1,2,3,4,5,6,7,8,9};
    for (int i = 0; i < 10; ++i) {
        size_t len = strlen(digits[i]);
        if (start + len <= text.length() && text.substr(start, len) == digits[i])
            return vals[i];
    }
    return -1;
}

static bool isChineseDigit(const std::string& text, size_t pos) {
    size_t consumed = 0;
    int val = 0;
    // Try to parse a number
    if (pos >= text.length()) return false;
    unsigned char c = static_cast<unsigned char>(text[pos]);
    if (c >= '0' && c <= '9') return true;
    static const char* digits[] = {"零","一","二","三","四","五","六","七","八","九","十"};
    for (int i = 0; i < 11; ++i) {
        size_t len = strlen(digits[i]);
        if (pos + len <= text.length() && text.substr(pos, len) == digits[i])
            return true;
    }
    return false;
}

static int parseChineseInt(const std::string& text, size_t start, size_t* consumed) {
    if (start >= text.length()) return -1;
    if (consumed) *consumed = 0;

    unsigned char c = static_cast<unsigned char>(text[start]);

    // 阿拉伯数字
    if (c >= '0' && c <= '9') {
        int val = (c - '0');
        size_t len = 1;
        if (start + 1 < text.length() && text[start+1] >= '0' && text[start+1] <= '9') {
            val = val * 10 + (text[start+1] - '0');
            len = 2;
        }
        if (consumed) *consumed = len;
        return val;
    }

    // 中文数字
    // 十 / 十一~十九
    if (start + 3 <= text.length() && text.substr(start, 3) == "十") {
        if (start + 6 > text.length() || !isChineseDigit(text, start + 3)) {
            if (consumed) *consumed = 3;
            return 10;
        }
        int unit = singleChineseDigit(text, start + 3);
        if (unit < 0) return -1;
        if (consumed) *consumed = 6;
        return 10 + unit;
    }

    // 二十~九十九
    static const char* tens[] = {"二","三","四","五","六","七","八","九"};
    for (int i = 0; i < 8; ++i) {
        size_t len = strlen(tens[i]);
        if (start + len + 3 <= text.length() &&
            text.substr(start, len) == tens[i] &&
            text.substr(start + len, 3) == "十") {
            int tensDigit = i + 2;
            if (start + len + 6 > text.length() || !isChineseDigit(text, start + len + 3)) {
                if (consumed) *consumed = len + 3;
                return tensDigit * 10;
            }
            int unit = singleChineseDigit(text, start + len + 3);
            if (unit < 0) return -1;
            if (consumed) *consumed = len + 6;
            return tensDigit * 10 + unit;
        }
    }

    // 单数字
    static const char* digits[] = {"零","一","二","三","四","五","六","七","八","九"};
    static int dvals[] = {0,1,2,3,4,5,6,7,8,9};
    for (int i = 0; i < 10; ++i) {
        size_t len = strlen(digits[i]);
        if (start + len <= text.length() && text.substr(start, len) == digits[i]) {
            if (consumed) *consumed = len;
            return dvals[i];
        }
    }

    return -1;
}

// ================================================================
// 正则 Fallback 解析
// ================================================================

ParsedTask NLProcessor::parseWithRegex(const std::string& text) {
    ParsedTask result;
    result.valid = false;

    std::string remaining = text;

    // === 1. 提取日期偏移 ===
    int dayOffset = 0;
    if (remaining.find("后天") != std::string::npos) {
        dayOffset = 2;
        remaining = remaining.substr(remaining.find("后天") + 6);
    } else if (remaining.find("明天") != std::string::npos) {
        dayOffset = 1;
        remaining = remaining.substr(remaining.find("明天") + 6);
    } else if (remaining.find("今天") != std::string::npos) {
        remaining = remaining.substr(remaining.find("今天") + 6);
    }

    // === 2. 提取时间 ===
    int hour = -1, minute = 0;
    size_t timeStart = std::string::npos, timeEnd = std::string::npos;

    // 找时段词并移除
    int ampm = 0;
    struct { const char* word; int val; } ampmList[] = {
        {"凌晨", -1}, {"早上", -1}, {"上午", -1},
        {"中午", 0}, {"下午", 1}, {"晚上", 1}, {"夜里", 1}
    };
    for (int i = 0; i < 7; i++) {
        size_t p = remaining.find(ampmList[i].word);
        if (p != std::string::npos) {
            ampm = ampmList[i].val;
            remaining = remaining.substr(0, p) + remaining.substr(p + strlen(ampmList[i].word));
            break;
        }
    }

    // 查找时间描述
    for (size_t i = 0; i + 3 <= remaining.size(); i++) {
        bool hasDot = (remaining.substr(i, 3) == "点");
        bool hasFullColon = (i + 3 <= remaining.size() && remaining.substr(i, 3) == "\xEF\xBC\x9A"); // 全角 ：
        bool hasHalfColon = (remaining[i] == ':');
        if (!hasDot && !hasFullColon && !hasHalfColon) continue;

        // 向前找数字：扫描所有可能起始位置，取消耗字节最长（即能覆盖最多字符）的匹配
        // 例如 "十二点" 中 "二"（3字节）和 "十二"（6字节）都能匹配到 "点"，
        // 取 "十二"（12），不取 "二"（2）
        int digit1 = -1;
        size_t digit1Bytes = 0;
        size_t longestMatch = 0;
        for (size_t j = 0; j < i; j++) {
            size_t consumed = 0;
            int d = parseChineseInt(remaining, j, &consumed);
            if (d >= 0 && consumed > 0 && j + consumed == i && consumed > longestMatch) {
                digit1 = d;
                digit1Bytes = consumed;
                timeStart = j;
                longestMatch = consumed;
            }
        }
        if (digit1 < 0) continue;

        hour = digit1;
        timeEnd = i + (hasDot ? 3 : (hasFullColon ? 3 : 1));

        // 检查 "点半" 或 "点Y分"
        if (hasDot && i + 6 <= remaining.size() && remaining.substr(i + 3, 3) == "半") {
            minute = 30;
            timeEnd = i + 6;
        } else if (hasDot && i + 6 <= remaining.size()) {
            size_t consumed = 0;
            int digit2 = parseChineseInt(remaining, i + 3, &consumed);
            if (digit2 >= 0) {
                minute = digit2;
                timeEnd = i + 3 + consumed;
                if (timeEnd + 3 <= remaining.size() && remaining.substr(timeEnd, 3) == "分")
                    timeEnd += 3;
            }
        } else if ((hasFullColon || hasHalfColon) && i + (hasFullColon ? 3 : 1) < remaining.size()) {
            size_t consumed = 0;
            int digit2 = parseChineseInt(remaining, i + (hasFullColon ? 3 : 1), &consumed);
            if (digit2 >= 0 && digit2 < 60) {
                minute = digit2;
                timeEnd = i + (hasFullColon ? 3 : 1) + consumed;
            }
        }

        // 调整上午/下午
        if (ampm == 1 && hour < 12) hour += 12;
        else if (ampm == -1 && hour == 12) hour = 0;

        break;
    }

    if (timeStart != std::string::npos && timeEnd != std::string::npos)
        remaining = remaining.substr(0, timeStart) + remaining.substr(timeEnd);

    if (hour < 0) hour = 9;

    time_t now = time(nullptr);
    struct tm* tm_now = localtime(&now);
    tm_now->tm_mday += dayOffset;
    tm_now->tm_hour = hour;
    tm_now->tm_min = minute;
    tm_now->tm_sec = 0;
    mktime(tm_now);

    char dateBuf[20];
    strftime(dateBuf, sizeof(dateBuf), "%Y-%m-%d %H:%M", tm_now);
    result.startTime = dateBuf;

    // === 3. 提取提醒时间（"提前N分钟/小时"）===
    {
        size_t pos = remaining.find("提前");
        if (pos != std::string::npos) {
            size_t after = pos + 6;
            size_t consumed = 0;
            int num = parseChineseInt(remaining, after, &consumed);
            if (num >= 0 && consumed > 0) {
                int offsetMin = 0;
                if (remaining.substr(after + consumed, 6) == "分钟")
                    offsetMin = -num;
                else if (remaining.substr(after + consumed, 6) == "小时")
                    offsetMin = -num * 60;

                if (offsetMin != 0) {
                    time_t startT = mktime(tm_now); // tm_now was already adjusted
                    time_t remindT = startT + offsetMin * 60;
                    struct tm* rt = localtime(&remindT);
                    char rb[20];
                    strftime(rb, sizeof(rb), "%Y-%m-%d %H:%M", rt);
                    result.remindTime = rb;
                }
            }
            // 移除提醒文字
            size_t endPos = remaining.find_first_of("，,", pos);
            if (endPos != std::string::npos) endPos += 3;
            else endPos = remaining.size();
            remaining = remaining.substr(0, pos) + remaining.substr(endPos);
        }
    }

    // === 4. 提取优先级 ===
    if (remaining.find("高") != std::string::npos &&
        (remaining.find("优先级") != std::string::npos || remaining.find("优先") != std::string::npos ||
         remaining.find("重要") != std::string::npos || remaining.find("紧急") != std::string::npos))
        result.priority = 1;
    else if (remaining.find("低") != std::string::npos &&
             (remaining.find("优先级") != std::string::npos || remaining.find("优先") != std::string::npos))
        result.priority = 3;

    const char* prioKws[] = {"高优先级", "中优先级", "低优先级", "高优先", "中优先", "低优先",
                            "优先级", "优先", nullptr};
    for (int k = 0; prioKws[k] != nullptr; ++k) {
        std::string kw(prioKws[k]);
        size_t p;
        while ((p = remaining.find(kw)) != std::string::npos)
            remaining = remaining.substr(0, p) + remaining.substr(p + kw.size());
    }

    // === 5. 提取分类 ===
    struct { const char* kws[15]; const char* cat; } catMap[] = {
        {{"学习", "作业", "上课", "考试", "复习", "预习", "看书", "读书",
          "写论文", "做实验", "图书馆", "自习", "听课", "学习会", ""}, "学习"},
        {{"娱乐", "游戏", "玩", "电影", "休息", "逛街", "旅游", "打球",
          "运动", "锻炼", "健身", "跑步", "游泳", "乒乓球", ""}, "娱乐"},
        {{"生活", "家务", "购物", "买菜", "做饭", "洗衣服", "打扫", "整理",
          "睡觉", "起床", "洗澡", "出门", "回家", "刷牙", ""}, "生活"},
    };
    bool found = false;
    for (auto& entry : catMap) {
        for (int k = 0; k < 15; ++k) {
            if (entry.kws[k][0] == '\0') break;
            if (remaining.find(entry.kws[k]) != std::string::npos) {
                result.category = entry.cat;
                found = true;
                break;
            }
        }
        if (found) break;
    }
    if (!found) result.category = "学习";

    // 移除分类关键词
    const char* catKws[] = {"学习", "娱乐", "生活", nullptr};
    for (int k = 0; catKws[k] != nullptr; ++k) {
        std::string kw(catKws[k]);
        size_t p;
        while ((p = remaining.find(kw)) != std::string::npos)
            remaining = remaining.substr(0, p) + remaining.substr(p + kw.size());
    }

    // === 6. 清理任务名称 ===
    auto trim = [](std::string& s) {
        while (!s.empty() && (s[0] == ' ' || s[0] == '\t' || s[0] == '\r' || s[0] == '\n'))
            s = s.substr(1);
        while (!s.empty() && (s.back() == ' ' || s.back() == '\t' || s.back() == '\r' || s.back() == '\n'))
            s.pop_back();
    };
    trim(remaining);

    const char* connectors[] = {"去", "的", "要", nullptr};
    for (int c = 0; connectors[c] != nullptr; ++c) {
        std::string con(connectors[c]);
        while (remaining.size() >= con.size() && remaining.substr(0, con.size()) == con)
            remaining = remaining.substr(con.size());
        trim(remaining);
    }

    result.name = remaining.empty() ? "未命名任务" : remaining;
    result.valid = true;
    return result;
}

// ================================================================
// 中文时间解析（resolveChineseTime — 保留备用，暂未被 parseWithRegex 调用）
// ================================================================

/**
 * resolveChineseTime - 将中文时间描述转为标准格式
 */
static int chineseDigitToInt_old(const std::string& text, size_t start) {
    if (start >= text.length()) return -1;
    char c = text[start];
    if (c >= '0' && c <= '9') return (c - '0');
    static const char* chineseDigits[] = {"零","一","二","三","四","五","六","七","八","九","十"};
    static int values[] = {0,1,2,3,4,5,6,7,8,9,10};
    for (int i = 0; i <= 10; ++i) {
        size_t len = strlen(chineseDigits[i]);
        if (text.substr(start, len) == chineseDigits[i]) return values[i];
    }
    return -1;
}

static int parseChineseNumber_old(const std::string& text, size_t start, size_t& consumed) {
    if (start >= text.length()) return -1;
    consumed = 0;
    int first = chineseDigitToInt_old(text, start);
    if (first < 0) return -1;
    unsigned char uc = static_cast<unsigned char>(text[start]);
    bool isChineseNum = (uc >= 0x80);
    if (isChineseNum) {
        if (first == 10) {
            const char* chTen = "十";
            size_t tenLen = strlen(chTen);
            if (text.substr(start, tenLen) == chTen) {
                consumed = tenLen;
                int second = chineseDigitToInt_old(text, start + tenLen);
                if (second >= 1 && second <= 9) {
                    unsigned char uc2 = static_cast<unsigned char>(text[start + tenLen]);
                    if (uc2 >= 0x80) {
                        const char* twoChar = "二";
                        consumed += strlen(twoChar);
                        return 10 + second;
                    }
                }
                return 10;
            }
        }
        const char* cnNum[] = {"零","一","二","三","四","五","六","七","八","九"};
        for (int i = 0; i <= 9; ++i) {
            size_t len = strlen(cnNum[i]);
            if (text.substr(start, len) == cnNum[i]) {
                consumed = len;
                return i;
            }
        }
    } else {
        consumed = 1;
        int val = first;
        if (start + 1 < text.length() && text[start+1] >= '0' && text[start+1] <= '9') {
            val = val * 10 + (text[start+1] - '0');
            consumed = 2;
        }
        return val;
    }
    return -1;
}

std::string NLProcessor::resolveChineseTime(const std::string& text) {
    std::string dateStr;
    if (text.find("今天") != std::string::npos) dateStr = todayDateStr();
    else if (text.find("明天") != std::string::npos) dateStr = dateOffset(1);
    else if (text.find("后天") != std::string::npos) dateStr = dateOffset(2);
    else if (text.find("昨天") != std::string::npos) dateStr = dateOffset(-1);
    else dateStr = todayDateStr();

    int hour = -1, minute = 0;
    size_t dotPos = text.find("点");
    int rawHour = -1;
    if (dotPos != std::string::npos) {
        int searchStart = static_cast<int>(dotPos) - 12;
        if (searchStart < 0) searchStart = 0;
        for (int len = 12; len >= 1; --len) {
            int start = static_cast<int>(dotPos) - len;
            if (start < 0) continue;
            size_t consumed = 0;
            int num = parseChineseNumber_old(text, static_cast<size_t>(start), consumed);
            if (num >= 0 && static_cast<size_t>(start) + consumed == dotPos) {
                rawHour = num;
                break;
            }
        }
        if (rawHour < 0) {
            for (int i = static_cast<int>(dotPos) - 1; i >= 0; --i) {
                size_t consumed = 0;
                int num = parseChineseNumber_old(text, static_cast<size_t>(i), consumed);
                if (num >= 0 && static_cast<size_t>(i) + consumed == dotPos) {
                    rawHour = num;
                    break;
                }
            }
        }
    }

    bool hasPeriod = false;
    if (text.find("上午") != std::string::npos || text.find("早上") != std::string::npos) {
        hasPeriod = true;
        if (rawHour >= 0) {
            hour = rawHour;
            if (hour == 12) hour = 0;
            if (hour > 12) hour = 12;
        }
    } else if (text.find("下午") != std::string::npos) {
        hasPeriod = true;
        if (rawHour >= 0) {
            if (rawHour >= 1 && rawHour <= 5) hour = 12 + rawHour;
            else if (rawHour == 12) hour = 12;
            else if (rawHour > 12 && rawHour <= 23) hour = rawHour;
            else hour = rawHour;
        }
    } else if (text.find("晚上") != std::string::npos) {
        hasPeriod = true;
        if (rawHour >= 0) {
            if (rawHour >= 1 && rawHour <= 11) hour = 12 + rawHour;
            else if (rawHour == 12) hour = 0;
            else if (rawHour > 12 && rawHour <= 23) hour = rawHour;
            else hour = rawHour;
        }
    } else if (text.find("中午") != std::string::npos) {
        hasPeriod = true;
        if (rawHour >= 0) hour = rawHour;
        else hour = 12;
    }

    if (!hasPeriod && rawHour >= 0) {
        hour = rawHour;
        if (hour > 23) hour = 23;
    }

    size_t halfPos = text.find("半");
    if (halfPos != std::string::npos && dotPos != std::string::npos) {
        if (halfPos > dotPos && halfPos - dotPos <= 3) minute = 30;
    }

    if (hour < 0) hour = 9;

    char timeBuf[20];
    snprintf(timeBuf, sizeof(timeBuf), "%s %02d:%02d", dateStr.c_str(), hour, minute);
    return std::string(timeBuf);
}

int NLProcessor::extractPriority(const std::string& text) {
    if (text.find("高") != std::string::npos &&
        (text.find("优先级") != std::string::npos || text.find("优先") != std::string::npos ||
         text.find("重要") != std::string::npos || text.find("紧急") != std::string::npos))
        return 1;
    if (text.find("低") != std::string::npos &&
        (text.find("优先级") != std::string::npos || text.find("优先") != std::string::npos))
        return 3;
    return 2;
}

std::string NLProcessor::extractCategory(const std::string& text) {
    if (text.find("娱乐") != std::string::npos || text.find("游戏") != std::string::npos ||
        text.find("玩") != std::string::npos || text.find("电影") != std::string::npos ||
        text.find("休息") != std::string::npos) return "娱乐";
    if (text.find("生活") != std::string::npos || text.find("家务") != std::string::npos ||
        text.find("购物") != std::string::npos || text.find("买菜") != std::string::npos ||
        text.find("做饭") != std::string::npos || text.find("运动") != std::string::npos ||
        text.find("锻炼") != std::string::npos || text.find("睡觉") != std::string::npos)
        return "生活";
    return "学习";
}

std::string NLProcessor::extractTaskName(const std::string& text, const std::string& startTime) {
    std::string result = text;
    const char* keywords[] = {
        "高优先级", "中优先级", "低优先级", "高优先", "中优先", "低优先",
        "优先", "优先级", "今天", "明天", "后天", "昨天",
        "上午", "下午", "晚上", "早上", "中午",
        "学习", "娱乐", "生活", "点半",
        "点", "小时", "分钟", nullptr
    };
    for (int i = 0; keywords[i] != nullptr; ++i) {
        std::string kw = keywords[i];
        size_t pos;
        while ((pos = result.find(kw)) != std::string::npos) result.erase(pos, kw.length());
    }
    for (int i = 0; i <= 23; ++i) {
        size_t pos;
        while ((pos = result.find(std::to_string(i))) != std::string::npos)
            result.erase(pos, std::to_string(i).length());
    }
    const char* cnDigits[] = {"零","一","二","三","四","五","六","七","八","九","十","十一","十二",
                             "十三","十四","十五","十六","十七","十八","十九","二十",
                             "二十一","二十二","二十三","二十四",nullptr};
    for (int d = 0; cnDigits[d] != nullptr; ++d) {
        size_t pos;
        while ((pos = result.find(cnDigits[d])) != std::string::npos)
            result.erase(pos, strlen(cnDigits[d]));
    }

    std::string cleaned;
    bool lastWasSpace = false;
    for (size_t i = 0; i < result.length(); ++i) {
        char c = result[i];
        if (c == ' ' || c == '\t') {
            if (!lastWasSpace) { cleaned += ' '; lastWasSpace = true; }
        } else { cleaned += c; lastWasSpace = false; }
    }
    size_t first = cleaned.find_first_not_of(" \t");
    size_t last = cleaned.find_last_not_of(" \t");
    if (first == std::string::npos || last == std::string::npos) return text;
    result = cleaned.substr(first, last - first + 1);
    return result.empty() ? text : result;
}

std::string NLProcessor::todayDateStr() {
    std::time_t t = std::time(nullptr);
    std::tm* tm = std::localtime(&t);
    char buf[20];
    snprintf(buf, sizeof(buf), "%04d-%02d-%02d", tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday);
    return std::string(buf);
}

std::string NLProcessor::dateOffset(int days) {
    std::time_t t = std::time(nullptr);
    t += days * 86400;
    std::tm* tm = std::localtime(&t);
    char buf[20];
    snprintf(buf, sizeof(buf), "%04d-%02d-%02d", tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday);
    return std::string(buf);
}

