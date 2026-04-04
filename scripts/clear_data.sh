#!/bin/bash

# C++ AI Chat Server - 一键清理数据库
# 使用方法：./scripts/clear_data.sh

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"

# 可能的数据库路径
DB_PATHS=(
    "$PROJECT_DIR/build/chat.db"
    "$PROJECT_DIR/chat.db"
    "$PROJECT_DIR/data/chat.db"
)

echo "🗑️  清理聊天服务器数据..."

FOUND_DB=""
for db in "${DB_PATHS[@]}"; do
    if [ -f "$db" ]; then
        FOUND_DB="$db"
        break
    fi
done

if [ -n "$FOUND_DB" ]; then
    echo "📁 找到数据库：$FOUND_DB"
    
    # 清理所有表数据
    sqlite3 "$FOUND_DB" "
        DELETE FROM messages;
        DELETE FROM users;
        DELETE FROM conversations;
        DELETE FROM rooms;
        DELETE FROM user_presets;
        DELETE FROM friendships;
        DELETE FROM friend_requests;
        DELETE FROM private_messages;
        
        -- 重置自增 ID
        DELETE FROM sqlite_sequence;
    "
    echo "✓ 所有数据已清理"
    
    SIZE=$(du -h "$FOUND_DB" | cut -f1)
    echo "📊 当前数据库大小：$SIZE"
else
    echo "✓ 数据库文件不存在（无需清理）"
fi

echo "✅ 完成"
