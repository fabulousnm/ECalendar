/**
 * @file NLProcessor.h
 * @brief 自然语言处理器 — 将语音识别的自然语言文本解析为结构化任务
 *
 * 两阶段解析策略：
 *   第一阶段：DeepSeek Chat API（精确解析，需要 libcurl 和 API Key）
 *   第二阶段：正则表达式（零依赖 fallback，支持常见中文时间表达）
 *
 * 先尝试 DeepSeek API，失败后自动回退到正则解析。
 * 通过 valid 字段标识解析是否成功。
 */

#ifndef ECALENDER_NLPROCESSOR_H
#define ECALENDER_NLPROCESSOR_H

#include <string>

/**
 * ParsedTask - 自然语言解析后的结构化任务
 *
 * 由 NLProcessor::parse() 返回，包含从语音文本中提取的各字段。
 * valid 字段标识解析是否成功，使用时需先检查。
 */
struct ParsedTask {
    std::string name;        // 任务名称
    std::string startTime;   // 开始时间 "YYYY-MM-DD HH:MM"
    int priority = 2;        // 1=高 2=中 3=低
    std::string category;    // "学习" "娱乐" "生活"
    std::string remindTime;  // 提醒时间（可选），格式 "YYYY-MM-DD HH:MM"
    bool valid = false;      // 解析是否成功
};

/**
 * NLProcessor - 自然语言处理器（静态方法）
 *
 * 提供 parse() 方法将口语化文本转为结构化任务。
 * 自动选择解析引擎：DeepSeek（有 key + curl）→ 正则（fallback）
 *
 * 使用示例：
 *   ParsedTask result = NLProcessor::parse("明天下午三点写作业高优先级");
 *   if (result.valid) { ... 使用 result.name, result.startTime ... }
 */
class NLProcessor {
public:
    /**
     * parse - 解析自然语言文本为结构化任务
     *
     * 先尝试 DeepSeek Chat API 解析（需要 HAS_CURL 且环境变量 DEEPSEEK_API_KEY 存在）。
     * 如果 API 不可用或解析失败，自动回退到正则解析。
     *
     * @param text 用户的自然语言输入（如"明天下午三点写作业高优先级"）
     * @return ParsedTask 结构化任务（检查 valid 字段确认是否成功）
     */
    static ParsedTask parse(const std::string& text);

private:
    // ---- DeepSeek API 解析 ----
    // 使用 DeepSeek Chat API 精确提取任务字段
    // @param text 用户输入文本
    // @return 解析结果（valid=false 表示失败）
    static ParsedTask parseWithDeepSeek(const std::string& text);

    // ---- 正则 Fallback 解析 ----
    // 零依赖的中文时间表达式正则解析
    // 支持：今天/明天/后天/上午/下午/晚上/N点/N点半
    // 优先级：高/中/低    分类：学习/娱乐/生活
    static ParsedTask parseWithRegex(const std::string& text);

    // ---- 辅助方法 ----
    // 从 DeepSeek API 返回的 JSON 中提取 content 字段
    // @param json API 原始响应字符串
    // @return content 字符串（可能包含嵌套 JSON）
    static std::string extractContentFromResponse(const std::string& json);

    // 解析 content 中的 JSON 字符串为 ParsedTask
    // @param jsonContent content 字段内容（如 {"name":"任务","startTime":"...",...}）
    // @return 解析结果
    static ParsedTask parseJsonContent(const std::string& jsonContent);

    // 将中文时间描述转为 "YYYY-MM-DD HH:MM" 格式
    // @param text 包含时间描述的中文字符串
    // @return 解析成功后返回时间字符串，失败返回空字符串
    static std::string resolveChineseTime(const std::string& text);

    // 从文本中提取优先级关键词
    // @param text 用户输入
    // @return 1/2/3（高/中/低），默认返回 2
    static int extractPriority(const std::string& text);

    // 从文本中提取分类关键词
    // @param text 用户输入
    // @return "学习"/"娱乐"/"生活"，默认返回"学习"
    static std::string extractCategory(const std::string& text);

    // 从文本中提取任务名称
    // 去除已知的时间、优先级、分类关键词后，剩余的部分作为任务名
    // @param text 用户输入
    // @param startTime 已解析的开始时间
    // @return 提取的任务名称
    static std::string extractTaskName(const std::string& text,
                                       const std::string& startTime);

    // 获取当前日期字符串 "YYYY-MM-DD"
    static std::string todayDateStr();

    // 获取 N 天后的日期字符串
    static std::string dateOffset(int days);
};

#endif // ECALENDER_NLPROCESSOR_H

