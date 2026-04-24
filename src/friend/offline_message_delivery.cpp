#include "offline_message_delivery.h"

#include <sqlite3.h>

namespace chat
{

    namespace
    {

        bool exec_sql(sqlite3 *db, const char *sql, std::string *error)
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

    } // namespace

    bool deliver_offline_messages_transactional(
        sqlite3 *db,
        const std::string &to_user,
        const std::function<bool(const OfflineMessageRow &)> &sink,
        int *delivered_count,
        std::string *error)
    {
        if (!db)
        {
            if (error)
                *error = "db is null";
            return false;
        }

        if (delivered_count)
        {
            *delivered_count = 0;
        }

        const char *select_sql =
            "SELECT id, from_user, content, timestamp "
            "FROM private_messages "
            "WHERE to_user = ? COLLATE NOCASE AND is_read = 0 "
            "ORDER BY timestamp ASC";

        sqlite3_stmt *select_stmt = nullptr;
        if (sqlite3_prepare_v2(db, select_sql, -1, &select_stmt, nullptr) != SQLITE_OK)
        {
            if (error)
                *error = sqlite3_errmsg(db);
            return false;
        }

        sqlite3_bind_text(select_stmt, 1, to_user.c_str(), -1, SQLITE_STATIC);

        if (!exec_sql(db, "BEGIN IMMEDIATE TRANSACTION", error))
        {
            sqlite3_finalize(select_stmt);
            return false;
        }

        const char *update_sql = "UPDATE private_messages SET is_read = 1 WHERE id = ?";
        sqlite3_stmt *update_stmt = nullptr;
        if (sqlite3_prepare_v2(db, update_sql, -1, &update_stmt, nullptr) != SQLITE_OK)
        {
            if (error)
                *error = sqlite3_errmsg(db);
            sqlite3_finalize(select_stmt);
            exec_sql(db, "ROLLBACK", nullptr);
            return false;
        }

        int delivered = 0;
        int step_result = SQLITE_DONE;
        bool ok = true;

        while ((step_result = sqlite3_step(select_stmt)) == SQLITE_ROW)
        {
            OfflineMessageRow row;
            row.id = sqlite3_column_int(select_stmt, 0);
            const char *from_user = reinterpret_cast<const char *>(sqlite3_column_text(select_stmt, 1));
            const char *content = reinterpret_cast<const char *>(sqlite3_column_text(select_stmt, 2));
            row.from_user = from_user ? from_user : "";
            row.content = content ? content : "";
            row.timestamp = sqlite3_column_int64(select_stmt, 3);

            if (!sink(row))
            {
                if (error)
                    *error = "sink failed";
                ok = false;
                break;
            }

            sqlite3_reset(update_stmt);
            sqlite3_clear_bindings(update_stmt);
            sqlite3_bind_int(update_stmt, 1, row.id);

            if (sqlite3_step(update_stmt) != SQLITE_DONE)
            {
                if (error)
                    *error = sqlite3_errmsg(db);
                ok = false;
                break;
            }

            ++delivered;
        }

        if (step_result != SQLITE_DONE)
        {
            ok = false;
            if (error && error->empty())
            {
                *error = sqlite3_errmsg(db);
            }
        }

        sqlite3_finalize(update_stmt);
        sqlite3_finalize(select_stmt);

        if (!ok)
        {
            exec_sql(db, "ROLLBACK", nullptr);
            return false;
        }

        if (!exec_sql(db, "COMMIT", error))
        {
            exec_sql(db, "ROLLBACK", nullptr);
            return false;
        }

        if (delivered_count)
        {
            *delivered_count = delivered;
        }
        return true;
    }

} // namespace chat
