#pragma once

#include <string>
#include <vector>

namespace chat {

// AI 预设结构
struct Preset {
    std::string name;
    std::string prompt;
    bool is_builtin = false;
    
    bool is_valid() const { return !name.empty() && !prompt.empty(); }
};

// 预设管理器
class PresetManager {
public:
    PresetManager(class Database* db);
    
    // 预设操作
    bool create_preset(int user_id, const std::string& name, const std::string& prompt);
    bool delete_preset(int user_id, const std::string& name);
    Preset get_preset(int user_id, const std::string& name);
    std::vector<Preset> get_all_presets(int user_id);
    
    // 获取内置预设
    static std::vector<Preset> get_builtin_presets();
    
    // 获取 AI 提示词
    std::string get_ai_prompt(int user_id, const std::string& preset_name);
    
private:
    class Database* db_;
    
    // 内置预设配置
    struct BuiltinPreset {
        const char* name;
        const char* prompt;
    };
    
    static const BuiltinPreset builtin_presets_[];
};

} // namespace chat
