#include "preset_manager.h"
#include "storage/database.h"
#include "utils/logger.h"

namespace chat {

// 内置预设配置
const PresetManager::BuiltinPreset PresetManager::builtin_presets_[] = {
    {"teacher", "你是耐心的老师，擅长用通俗易懂的方式讲解知识，会用生活中的例子帮助理解。"},
    {"coder", "你是资深程序员，代码简洁高效，注释清晰，善于解释技术概念。"},
    {"friend", "你是知心朋友，善于倾听，语气温和，会给予情感支持和建议。"},
    {"interviewer", "你是面试官，会提出专业问题，评估候选人能力，并给予反馈。"},
    {"critic", "你是评论家，提供建设性的批评和建议，帮助改进作品。"},
    {"coach", "你是健身教练，专业且充满鼓励，会制定训练计划和饮食建议。"},
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
