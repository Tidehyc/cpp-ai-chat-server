#include "user_manager.h"
#include "storage/database.h"
#include "utils/logger.h"
#include <openssl/sha.h>
#include <sstream>
#include <iomanip>
#include <vector>
#include <random>
#include <chrono>
#include <cstring>

namespace chat
{

    namespace
    {

        constexpr const char *kPasswordAlgoPrefix = "v2";

        std::string to_hex(const unsigned char *data, size_t len)
        {
            std::stringstream ss;
            for (size_t i = 0; i < len; ++i)
            {
                ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(data[i]);
            }
            return ss.str();
        }

        std::string generate_salt_hex(size_t bytes = 16)
        {
            std::vector<unsigned char> salt(bytes);
            std::random_device rd;
            for (size_t i = 0; i < bytes; ++i)
            {
                salt[i] = static_cast<unsigned char>(rd());
            }
            return to_hex(salt.data(), salt.size());
        }

        std::string build_password_record(const std::string &salt_hex, const std::string &digest_hex)
        {
            return std::string(kPasswordAlgoPrefix) + "$" + salt_hex + "$" + digest_hex;
        }

        bool parse_password_record(const std::string &record, std::string &salt_hex, std::string &digest_hex)
        {
            const std::string prefix = std::string(kPasswordAlgoPrefix) + "$";
            if (record.rfind(prefix, 0) != 0)
            {
                return false;
            }

            size_t salt_start = prefix.size();
            size_t sep = record.find('$', salt_start);
            if (sep == std::string::npos)
            {
                return false;
            }

            salt_hex = record.substr(salt_start, sep - salt_start);
            digest_hex = record.substr(sep + 1);
            return !salt_hex.empty() && !digest_hex.empty();
        }

        bool exec_sql(sqlite3 *db, const char *sql, std::string *error = nullptr)
        {
            char *err_msg = nullptr;
            const int rc = sqlite3_exec(db, sql, nullptr, nullptr, &err_msg);
            if (rc != SQLITE_OK)
            {
                if (error)
                {
                    *error = err_msg ? err_msg : "sqlite exec failed";
                }
                if (err_msg)
                {
                    sqlite3_free(err_msg);
                }
                return false;
            }
            return true;
        }

        bool ensure_schema_meta(sqlite3 *db, std::string *error = nullptr)
        {
            const char *sql = R"(
                CREATE TABLE IF NOT EXISTS schema_meta (
                    key TEXT PRIMARY KEY,
                    value INTEGER NOT NULL
                )
            )";
            return exec_sql(db, sql, error);
        }

        int read_schema_version(sqlite3 *db, const char *key)
        {
            const char *sql = "SELECT value FROM schema_meta WHERE key = ?";
            sqlite3_stmt *stmt = nullptr;
            if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
            {
                return 0;
            }

            sqlite3_bind_text(stmt, 1, key, -1, SQLITE_STATIC);
            int version = 0;
            if (sqlite3_step(stmt) == SQLITE_ROW)
            {
                version = sqlite3_column_int(stmt, 0);
            }
            sqlite3_finalize(stmt);
            return version;
        }

        bool write_schema_version(sqlite3 *db, const char *key, int version, std::string *error = nullptr)
        {
            const char *sql = R"(
                INSERT INTO schema_meta (key, value)
                VALUES (?, ?)
                ON CONFLICT(key) DO UPDATE SET value = excluded.value
            )";

            sqlite3_stmt *stmt = nullptr;
            if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
            {
                if (error)
                {
                    *error = sqlite3_errmsg(db);
                }
                return false;
            }

            sqlite3_bind_text(stmt, 1, key, -1, SQLITE_STATIC);
            sqlite3_bind_int(stmt, 2, version);

            const bool ok = (sqlite3_step(stmt) == SQLITE_DONE);
            if (!ok && error)
            {
                *error = sqlite3_errmsg(db);
            }
            sqlite3_finalize(stmt);
            return ok;
        }

        bool column_exists(sqlite3 *db, const std::string &table_name, const std::string &column_name)
        {
            std::string sql = "PRAGMA table_info(" + table_name + ")";
            sqlite3_stmt *stmt = nullptr;
            if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK)
            {
                return false;
            }

            bool found = false;
            while (sqlite3_step(stmt) == SQLITE_ROW)
            {
                const char *name = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1));
                if (name && column_name == name)
                {
                    found = true;
                    break;
                }
            }
            sqlite3_finalize(stmt);
            return found;
        }

        bool migrate_user_schema(sqlite3 *db, std::string *error = nullptr)
        {
            if (!ensure_schema_meta(db, error))
            {
                return false;
            }

            int version = read_schema_version(db, "users_schema_version");
            if (version < 1)
            {
                if (!column_exists(db, "users", "email"))
                {
                    if (!exec_sql(db, "ALTER TABLE users ADD COLUMN email TEXT DEFAULT NULL", error))
                    {
                        return false;
                    }
                }

                if (!write_schema_version(db, "users_schema_version", 1, error))
                {
                    return false;
                }
            }

            return true;
        }

    } // namespace

    // 简单的密码哈希
    static std::string hash_password(const std::string &password)
    {
        unsigned char hash[SHA256_DIGEST_LENGTH];
        SHA256(reinterpret_cast<const unsigned char *>(password.c_str()), password.size(), hash);
        return to_hex(hash, SHA256_DIGEST_LENGTH);
    }

    static std::string hash_password_with_salt(const std::string &password, const std::string &salt_hex)
    {
        return hash_password(salt_hex + ":" + password);
    }

    UserManager::UserManager(Database *db) : db_(db)
    {
        // 初始化用户表
        const char *sql = R"(
        CREATE TABLE IF NOT EXISTS users (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            username TEXT UNIQUE NOT NULL,
            password_hash TEXT,
            current_preset TEXT DEFAULT 'friend',
            status TEXT DEFAULT 'online',
            session_token TEXT DEFAULT NULL,
            email TEXT DEFAULT NULL,
            created_at INTEGER NOT NULL
        )
    )";

        char *err_msg = nullptr;
        if (sqlite3_exec(db_->get_db(), sql, nullptr, nullptr, &err_msg) != SQLITE_OK)
        {
            Logger::error("Failed to create users table: " + std::string(err_msg));
            sqlite3_free(err_msg);
        }

        std::string migrate_error;
        if (!migrate_user_schema(db_->get_db(), &migrate_error))
        {
            Logger::error("Failed to migrate user schema: " + migrate_error);
        }

        // 服务器启动时重置所有用户状态为 offline
        reset_all_users_offline();
    }

    void UserManager::reset_all_users_offline()
    {
        const char *sql = "UPDATE users SET status = 'offline'";
        char *err_msg = nullptr;

        if (sqlite3_exec(db_->get_db(), sql, nullptr, nullptr, &err_msg) != SQLITE_OK)
        {
            Logger::error("Failed to reset user status: " + std::string(err_msg));
            sqlite3_free(err_msg);
        }
        else
        {
            Logger::info("All users reset to offline status");
        }
    }

    bool UserManager::register_user(const std::string &username, const std::string &password)
    {
        if (username.empty() || password.empty())
        {
            return false;
        }

        const char *sql = R"(
        INSERT INTO users (username, password_hash, status, created_at)
        VALUES (?, ?, 'offline', ?)
    )";

        sqlite3_stmt *stmt = nullptr;
        if (sqlite3_prepare_v2(db_->get_db(), sql, -1, &stmt, nullptr) != SQLITE_OK)
        {
            Logger::error("Failed to prepare register statement");
            return false;
        }

        std::string salt_hex = generate_salt_hex();
        std::string pwd_hash = build_password_record(salt_hex, hash_password_with_salt(password, salt_hex));
        sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 2, pwd_hash.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int64(stmt, 3, std::time(nullptr));

        int rc = sqlite3_step(stmt);
        sqlite3_finalize(stmt);

        if (rc == SQLITE_DONE)
        {
            Logger::info("User registered: " + username + " (status: offline)");
            return true;
        }
        else if (rc == SQLITE_CONSTRAINT)
        {
            Logger::info("Username already exists: " + username);
            return false; // 用户名已存在
        }

        return false;
    }

    LoginResult UserManager::login(const std::string &username, const std::string &password, User &user)
    {
        const char *sql = R"(
        SELECT id, username, password_hash, current_preset, status, created_at
        FROM users
        WHERE username = ?
    )";

        sqlite3_stmt *stmt = nullptr;
        if (sqlite3_prepare_v2(db_->get_db(), sql, -1, &stmt, nullptr) != SQLITE_OK)
        {
            Logger::error("Failed to prepare login statement");
            return LoginResult::USER_NOT_FOUND;
        }

        sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_STATIC);

        if (sqlite3_step(stmt) == SQLITE_ROW)
        {
            user.id = sqlite3_column_int(stmt, 0);
            user.username = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1));
            const unsigned char *hash_ptr = sqlite3_column_text(stmt, 2);
            std::string stored_hash = hash_ptr ? reinterpret_cast<const char *>(hash_ptr) : "";
            user.current_preset = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 3));
            user.status = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 4));
            user.created_at = sqlite3_column_int64(stmt, 5);

            bool password_ok = false;
            bool needs_upgrade = false;

            std::string salt_hex;
            std::string digest_hex;
            if (parse_password_record(stored_hash, salt_hex, digest_hex))
            {
                password_ok = (digest_hex == hash_password_with_salt(password, salt_hex));
            }
            else
            {
                // 兼容旧版本纯 SHA256 存储；首次成功登录后自动升级到带盐格式。
                password_ok = (stored_hash == hash_password(password));
                needs_upgrade = password_ok;
            }

            sqlite3_finalize(stmt);

            if (password_ok)
            {
                if (needs_upgrade)
                {
                    std::string new_salt = generate_salt_hex();
                    std::string upgraded = build_password_record(new_salt, hash_password_with_salt(password, new_salt));

                    const char *update_sql = "UPDATE users SET password_hash = ? WHERE id = ?";
                    sqlite3_stmt *update_stmt = nullptr;
                    if (sqlite3_prepare_v2(db_->get_db(), update_sql, -1, &update_stmt, nullptr) == SQLITE_OK)
                    {
                        sqlite3_bind_text(update_stmt, 1, upgraded.c_str(), -1, SQLITE_TRANSIENT);
                        sqlite3_bind_int(update_stmt, 2, user.id);
                        if (sqlite3_step(update_stmt) == SQLITE_DONE)
                        {
                            Logger::info("Upgraded legacy password hash for user: " + username);
                        }
                        else
                        {
                            Logger::warn("Failed to upgrade legacy password hash for user: " + username);
                        }
                        sqlite3_finalize(update_stmt);
                    }
                }

                Logger::info("User logged in: " + username);
                return LoginResult::SUCCESS;
            }
            else
            {
                Logger::info("Wrong password for user: " + username);
                return LoginResult::WRONG_PASSWORD;
            }
        }

        sqlite3_finalize(stmt);
        Logger::info("User not found: " + username);
        return LoginResult::USER_NOT_FOUND;
    }

    User UserManager::get_user(int user_id)
    {
        User user;
        const char *sql = R"(
        SELECT id, username, current_preset, status, created_at
        FROM users
        WHERE id = ?
    )";

        sqlite3_stmt *stmt = nullptr;
        if (sqlite3_prepare_v2(db_->get_db(), sql, -1, &stmt, nullptr) == SQLITE_OK)
        {
            sqlite3_bind_int(stmt, 1, user_id);

            if (sqlite3_step(stmt) == SQLITE_ROW)
            {
                user.id = sqlite3_column_int(stmt, 0);
                user.username = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1));
                user.current_preset = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 2));
                user.status = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 3));
                user.created_at = sqlite3_column_int64(stmt, 4);
            }
            sqlite3_finalize(stmt);
        }

        return user;
    }

    User UserManager::get_user_by_name(const std::string &username)
    {
        User user;
        const char *sql = R"(
        SELECT id, username, current_preset, status, created_at
        FROM users
        WHERE username = ?
    )";

        sqlite3_stmt *stmt = nullptr;
        if (sqlite3_prepare_v2(db_->get_db(), sql, -1, &stmt, nullptr) == SQLITE_OK)
        {
            sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_STATIC);

            if (sqlite3_step(stmt) == SQLITE_ROW)
            {
                user.id = sqlite3_column_int(stmt, 0);
                user.username = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1));
                user.current_preset = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 2));
                user.status = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 3));
                user.created_at = sqlite3_column_int64(stmt, 4);
            }
            sqlite3_finalize(stmt);
        }

        return user;
    }

    bool UserManager::update_status(int user_id, const std::string &status)
    {
        const char *sql = "UPDATE users SET status = ? WHERE id = ?";
        sqlite3_stmt *stmt = nullptr;

        Logger::info("Updating user " + std::to_string(user_id) + " status to: " + status);

        if (sqlite3_prepare_v2(db_->get_db(), sql, -1, &stmt, nullptr) == SQLITE_OK)
        {
            sqlite3_bind_text(stmt, 1, status.c_str(), -1, SQLITE_STATIC);
            sqlite3_bind_int(stmt, 2, user_id);
            int rc = sqlite3_step(stmt);
            sqlite3_finalize(stmt);
            if (rc == SQLITE_DONE)
            {
                Logger::info("User " + std::to_string(user_id) + " status updated successfully");
            }
            else
            {
                Logger::error("Failed to update user " + std::to_string(user_id) + " status");
            }
            return rc == SQLITE_DONE;
        }

        Logger::error("Failed to prepare update_status statement");
        return false;
    }

    std::string UserManager::get_user_status(int user_id)
    {
        const char *sql = "SELECT status FROM users WHERE id = ?";
        sqlite3_stmt *stmt = nullptr;

        if (sqlite3_prepare_v2(db_->get_db(), sql, -1, &stmt, nullptr) == SQLITE_OK)
        {
            sqlite3_bind_int(stmt, 1, user_id);
            if (sqlite3_step(stmt) == SQLITE_ROW)
            {
                std::string status = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 0));
                sqlite3_finalize(stmt);
                return status;
            }
            sqlite3_finalize(stmt);
        }
        return "offline";
    }

    bool UserManager::update_preset(int user_id, const std::string &preset)
    {
        const char *sql = "UPDATE users SET current_preset = ? WHERE id = ?";
        sqlite3_stmt *stmt = nullptr;

        if (sqlite3_prepare_v2(db_->get_db(), sql, -1, &stmt, nullptr) == SQLITE_OK)
        {
            sqlite3_bind_text(stmt, 1, preset.c_str(), -1, SQLITE_STATIC);
            sqlite3_bind_int(stmt, 2, user_id);
            int rc = sqlite3_step(stmt);
            sqlite3_finalize(stmt);
            return rc == SQLITE_DONE;
        }

        return false;
    }

    std::vector<User> UserManager::get_all_users()
    {
        std::vector<User> users;
        // 返回所有用户
        const char *sql = R"(
        SELECT id, username, current_preset, status, created_at
        FROM users
        ORDER BY username
    )";

        sqlite3_stmt *stmt = nullptr;
        if (sqlite3_prepare_v2(db_->get_db(), sql, -1, &stmt, nullptr) == SQLITE_OK)
        {
            while (sqlite3_step(stmt) == SQLITE_ROW)
            {
                User user;
                user.id = sqlite3_column_int(stmt, 0);
                user.username = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1));
                user.current_preset = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 2));
                user.status = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 3));
                user.created_at = sqlite3_column_int64(stmt, 4);
                users.push_back(user);
            }
            sqlite3_finalize(stmt);
        }

        return users;
    }

    std::vector<User> UserManager::get_online_users()
    {
        std::vector<User> users;
        // 返回所有非 offline 状态的用户
        const char *sql = R"(
        SELECT id, username, current_preset, status, created_at
        FROM users
        WHERE status != 'offline'
        ORDER BY username
    )";

        Logger::debug("Loading online users from database");

        sqlite3_stmt *stmt = nullptr;
        if (sqlite3_prepare_v2(db_->get_db(), sql, -1, &stmt, nullptr) == SQLITE_OK)
        {
            while (sqlite3_step(stmt) == SQLITE_ROW)
            {
                User user;
                user.id = sqlite3_column_int(stmt, 0);
                user.username = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1));
                user.current_preset = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 2));
                user.status = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 3));
                user.created_at = sqlite3_column_int64(stmt, 4);
                users.push_back(user);
            }
            sqlite3_finalize(stmt);
        }

        Logger::debug("Loaded online users count=" + std::to_string(users.size()));
        return users;
    }

    void UserManager::set_user_online(int user_id, bool online)
    {
        if (online)
        {
            online_users_.insert(user_id);
        }
        else
        {
            online_users_.erase(user_id);
        }
    }

    bool UserManager::is_online(int user_id)
    {
        return online_users_.find(user_id) != online_users_.end();
    }

    // 生成会话 token
    std::string UserManager::generate_session_token()
    {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, 15);

        std::stringstream ss;
        for (int i = 0; i < 32; i++)
        {
            ss << std::hex << dis(gen);
        }
        return ss.str();
    }

    // 设置会话 token
    bool UserManager::set_session_token(int user_id, const std::string &token)
    {
        const char *sql = "UPDATE users SET session_token = ? WHERE id = ?";
        sqlite3_stmt *stmt = nullptr;

        Logger::info("Setting session token for user " + std::to_string(user_id));

        if (sqlite3_prepare_v2(db_->get_db(), sql, -1, &stmt, nullptr) == SQLITE_OK)
        {
            sqlite3_bind_text(stmt, 1, token.c_str(), -1, SQLITE_STATIC);
            sqlite3_bind_int(stmt, 2, user_id);
            int rc = sqlite3_step(stmt);
            Logger::debug("Set token result: " + std::string(rc == SQLITE_DONE ? "SUCCESS" : "FAILED"));
            sqlite3_finalize(stmt);
            return rc == SQLITE_DONE;
        }
        Logger::warn("Failed to prepare statement for setting token");
        return false;
    }

    // 验证会话 token
    bool UserManager::verify_session_token(int user_id, const std::string &token)
    {
        const char *sql = "SELECT session_token FROM users WHERE id = ?";
        sqlite3_stmt *stmt = nullptr;

        Logger::debug("Verifying token for user " + std::to_string(user_id) + ", token_len=" + std::to_string(token.size()));

        if (sqlite3_prepare_v2(db_->get_db(), sql, -1, &stmt, nullptr) == SQLITE_OK)
        {
            sqlite3_bind_int(stmt, 1, user_id);
            if (sqlite3_step(stmt) == SQLITE_ROW)
            {
                const char *db_token = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 0));
                std::string db_token_str = db_token ? db_token : "";
                Logger::debug("Token match: " + std::string((token == db_token_str) ? "YES" : "NO"));
                sqlite3_finalize(stmt);
                return (token == db_token_str);
            }
            Logger::debug("No token found in database for user " + std::to_string(user_id));
            sqlite3_finalize(stmt);
        }
        return false;
    }

    // 移除会话 token
    void UserManager::remove_session_token(int user_id)
    {
        const char *sql = "UPDATE users SET session_token = NULL WHERE id = ?";
        sqlite3_stmt *stmt = nullptr;

        if (sqlite3_prepare_v2(db_->get_db(), sql, -1, &stmt, nullptr) == SQLITE_OK)
        {
            sqlite3_bind_int(stmt, 1, user_id);
            sqlite3_step(stmt);
            sqlite3_finalize(stmt);
        }
    }

} // namespace chat
