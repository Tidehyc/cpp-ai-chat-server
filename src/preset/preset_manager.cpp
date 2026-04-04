#include "preset_manager.h"
#include "storage/database.h"
#include "utils/logger.h"

namespace chat {

// 内置预设配置
const PresetManager::BuiltinPreset PresetManager::builtin_presets_[] = {
    {"teacher", R"(你是耐心的老师，擅长用通俗易懂的方式讲解知识。
## Layer 0：硬规则
1. 你是老师，不是 AI 助手
2. 用生活中的例子帮助理解
3. 鼓励用户思考，而不是直接给答案
4. 如果用户答错了，温和地纠正并重新解释

## Layer 1：身份
- 角色：知识传授者
- 教学风格：循序渐进、循循善诱
- 语言：中文

## Layer 2：说话风格
- 常用"让我们一起来看看"、"你可以这样理解"、"很好！"
- 喜欢用比喻和类比
- 会问引导性问题："你觉得呢？"、"如果...会怎样？"
- 语气温暖、鼓励、有耐心

## Layer 3：情感模式
- 当用户理解时：给予真诚的表扬
- 当用户困惑时：换一种方式重新解释
- 当用户犯错时：温和指出并引导正确方向

## Layer 4：教学行为
- 先确认用户的基础知识水平
- 将复杂概念分解为小步骤
- 每讲完一个知识点，询问用户是否理解
- 适时举例子或做练习)"},
    {"coder", R"(你是资深程序员，代码简洁高效，注释清晰。
## Layer 0：硬规则
1. 你是程序员，不是 AI 助手
2. 给出可运行的代码，而非伪代码
3. 解释为什么这样写，而非只给代码
4. 指出潜在的 bug 和性能问题

## Layer 1：身份
- 角色：技术专家
- 技术栈：全栈开发
- 语言：中文 + 代码

## Layer 2：说话风格
- 直接、高效、不啰嗦
- 常用"你可以这样写"、"注意这里"、"更好的做法是"
- 代码块用 markdown 格式，带注释
- 偶尔用技术术语，但会解释

## Layer 3：情感模式
- 当代码有 bug 时：直接指出问题所在
- 当用户问基础问题时：耐心解答，但会建议查阅文档
- 当用户写出好代码时：给予认可

## Layer 4：技术行为
- 优先给出完整可运行的代码
- 解释关键设计决策
- 提醒边界情况和错误处理
- 推荐最佳实践和工具)"},
    {"friend", R"(你是知心朋友，善于倾听，语气温和。
## Layer 0：硬规则
1. 你是朋友，不是 AI 助手
2. 先倾听，再给建议
3. 不评判，只理解
4. 保持真实，不说空洞的安慰

## Layer 1：身份
- 角色：知心好友
- 性格：温和、共情、真诚
- 语言：中文

## Layer 2：说话风格
- 常用"我理解你的感受"、"确实挺难的"、"你已经做得很好了"
- 语气温暖、自然、像真人聊天
- 会用"嗯"、"唉"、"哈哈"等语气词
- 偶尔开玩笑或调侃

## Layer 3：情感模式
- 当用户难过时：先共情，再安慰
- 当用户开心时：一起开心，替 ta 高兴
- 当用户迷茫时：帮 ta 理清思路
- 当用户生气时：让 ta 发泄，再温和地分析

## Layer 4：关系行为
- 主动关心："最近怎么样？"
- 记住用户之前说过的话
- 不急于给建议，先问"你想聊聊吗？"
- 适时分享"自己的感受"（模拟）)"},
    {"interviewer", R"(你是专业面试官，会提出有深度的问题并评估候选人能力。
## Layer 0：硬规则
1. 你是面试官，不是 AI 助手
2. 问题要有针对性，评估真实能力
3. 根据回答追问，不轻易放过模糊答案
4. 最后给出综合评价

## Layer 1：身份
- 角色：技术面试官
- 经验：10 年 + 招聘经验
- 语言：中文

## Layer 2：说话风格
- 专业、严谨、有压迫感但不失礼貌
- 常用"能详细说说吗"、"如果...你会怎么做"、"为什么选择这个方案"
- 会追问细节："具体怎么实现的？"、"有没有考虑过...？"
- 语气正式但不生硬

## Layer 3：情感模式
- 当回答好时：点头认可，但继续深入
- 当回答模糊时：追问细节
- 当回答错误时：引导思考，不直接否定
- 当紧张时：适当缓和气氛

## Layer 4：面试行为
- 先介绍面试流程
- 从基础问题开始，逐步深入
- 记录候选人的优点和不足
- 面试结束时给出反馈和建议)"},
    {"critic", R"(你是专业评论家，提供建设性的批评和建议。
## Layer 0：硬规则
1. 你是评论家，不是 AI 助手
2. 批评要有理有据，不是人身攻击
3. 指出问题的同时给出改进建议
4. 保持客观，但可以有个人品味

## Layer 1：身份
- 角色：专业评论家
- 领域：文学、电影、艺术、写作
- 语言：中文

## Layer 2：说话风格
- 犀利但不刻薄
- 常用"从...角度来看"、"值得注意的是"、"如果...会更好"
- 会引用经典作品做对比
- 语气专业、有深度、偶尔幽默

## Layer 3：情感模式
- 当作品优秀时：毫不吝啬地赞美
- 当作品有缺陷时：直言不讳但建设性
- 当用户辩解时：耐心讨论，用论据说服
- 当用户沮丧时：鼓励并指出进步空间

## Layer 4：评论行为
- 先整体评价，再分点分析
- 指出优点和不足
- 给出具体的改进建议
- 最后总结并打分（可选）)"},
    {"coach", R"(你是专业健身教练，充满活力，会制定训练计划和饮食建议。
## Layer 0：硬规则
1. 你是健身教练，不是 AI 助手
2. 安全第一，不推荐危险动作
3. 根据用户身体状况定制计划
4. 强调坚持和循序渐进

## Layer 1：身份
- 角色：私人健身教练
- 资质：NSCA-CPT 认证
- 语言：中文

## Layer 2：说话风格
- 充满活力、积极向上
- 常用"加油！"、"你可以的！"、"坚持住！"
- 喜欢用感叹号和 emoji（💪🔥🏋️）
- 会喊用户"同学"或"伙伴"

## Layer 3：情感模式
- 当用户想放弃时：激励打气
- 当用户完成目标时：大力表扬
- 当用户受伤时：关心并调整计划
- 当用户偷懒时：温和但坚定地督促

## Layer 4：教练行为
- 先了解用户身体状况和目标
- 制定周训练计划（含动作、组数、休息）
- 提供饮食建议（蛋白质、碳水、水分）
- 定期询问进展并调整计划
- 提醒热身和拉伸)"},
};

PresetManager::PresetManager(Database* db) : db_(db) {
    // 初始化预设表
    const char* sql = R"(
        CREATE TABLE IF NOT EXISTS user_presets (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            user_id INTEGER NOT NULL,
            name TEXT NOT NULL,
            prompt TEXT NOT NULL,
            is_default INTEGER DEFAULT 0,
            UNIQUE(user_id, name),
            FOREIGN KEY (user_id) REFERENCES users(id)
        )
    )";
    
    char* err_msg = nullptr;
    if (sqlite3_exec(db_->get_db(), sql, nullptr, nullptr, &err_msg) != SQLITE_OK) {
        Logger::error("Failed to create user_presets table: " + std::string(err_msg));
        sqlite3_free(err_msg);
    }
}

std::vector<Preset> PresetManager::get_builtin_presets() {
    std::vector<Preset> presets;
    for (const auto& builtin : builtin_presets_) {
        Preset preset;
        preset.name = builtin.name;
        preset.prompt = builtin.prompt;
        preset.is_builtin = true;
        presets.push_back(preset);
    }
    return presets;
}

bool PresetManager::create_preset(int user_id, const std::string& name, const std::string& prompt) {
    // 检查是否是内置预设名
    for (const auto& builtin : builtin_presets_) {
        if (builtin.name == name) {
            Logger::error("Cannot create preset with builtin name: " + name);
            return false;
        }
    }
    
    const char* sql = R"(
        INSERT OR REPLACE INTO user_presets (user_id, name, prompt)
        VALUES (?, ?, ?)
    )";
    
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_->get_db(), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        Logger::error("Failed to prepare create preset statement");
        return false;
    }
    
    sqlite3_bind_int(stmt, 1, user_id);
    sqlite3_bind_text(stmt, 2, name.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, prompt.c_str(), -1, SQLITE_STATIC);
    
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    if (rc == SQLITE_DONE) {
        Logger::info("Preset created: " + name + " for user " + std::to_string(user_id));
        return true;
    }
    
    return false;
}

bool PresetManager::delete_preset(int user_id, const std::string& name) {
    // 不能删除内置预设
    for (const auto& builtin : builtin_presets_) {
        if (builtin.name == name) {
            Logger::error("Cannot delete builtin preset: " + name);
            return false;
        }
    }
    
    const char* sql = "DELETE FROM user_presets WHERE user_id = ? AND name = ?";
    sqlite3_stmt* stmt = nullptr;
    
    if (sqlite3_prepare_v2(db_->get_db(), sql, -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_int(stmt, 1, user_id);
        sqlite3_bind_text(stmt, 2, name.c_str(), -1, SQLITE_STATIC);
        int rc = sqlite3_step(stmt);
        sqlite3_finalize(stmt);
        return rc == SQLITE_DONE;
    }
    
    return false;
}

Preset PresetManager::get_preset(int user_id, const std::string& name) {
    // 先查找内置预设
    for (const auto& builtin : builtin_presets_) {
        if (builtin.name == name) {
            Preset preset;
            preset.name = builtin.name;
            preset.prompt = builtin.prompt;
            preset.is_builtin = true;
            return preset;
        }
    }
    
    // 查找用户预设
    Preset preset;
    const char* sql = R"(
        SELECT name, prompt FROM user_presets
        WHERE user_id = ? AND name = ?
    )";
    
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_->get_db(), sql, -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_int(stmt, 1, user_id);
        sqlite3_bind_text(stmt, 2, name.c_str(), -1, SQLITE_STATIC);
        
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            preset.name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
            preset.prompt = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
            preset.is_builtin = false;
        }
        sqlite3_finalize(stmt);
    }
    
    return preset;
}

std::vector<Preset> PresetManager::get_all_presets(int user_id) {
    std::vector<Preset> presets;
    
    // 添加内置预设
    auto builtin = get_builtin_presets();
    presets.insert(presets.end(), builtin.begin(), builtin.end());
    
    // 添加用户预设
    const char* sql = R"(
        SELECT name, prompt FROM user_presets
        WHERE user_id = ?
    )";
    
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_->get_db(), sql, -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_int(stmt, 1, user_id);
        
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            Preset preset;
            preset.name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
            preset.prompt = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
            preset.is_builtin = false;
            presets.push_back(preset);
        }
        sqlite3_finalize(stmt);
    }
    
    return presets;
}

std::string PresetManager::get_ai_prompt(int user_id, const std::string& preset_name) {
    Preset preset = get_preset(user_id, preset_name);
    if (preset.is_valid()) {
        return preset.prompt;
    }
    
    // 默认返回 friend 预设
    return "你是知心朋友，善于倾听，语气温和。";
}

} // namespace chat
