#!/bin/bash

# C++ AI Chat Server - 数据库清理脚本
# 用于清理聊天服务器中的所有数据

set -e

DB_PATH="./chat.db"

echo "========================================"
echo "  C++ AI Chat Server - 数据库清理工具"
echo "========================================"
echo ""

# 检查数据库文件是否存在
if [ ! -f "$DB_PATH" ]; then
    echo "✓ 数据库文件不存在，无需清理"
    echo ""
    echo "提示：数据库将在下次启动时自动创建"
    exit 0
fi

echo "📁 数据库文件：$DB_PATH"
echo ""

# 显示清理选项
echo "请选择清理范围："
echo "  1) 仅清理消息数据"
echo "  2) 仅清理用户数据"
echo "  3) 仅清理好友数据"
echo "  4) 清理所有数据（推荐）"
echo "  5) 删除整个数据库文件"
echo "  0) 取消"
echo ""

read -p "请输入选项 (0-5): " choice

case $choice in
    1)
        echo ""
        echo "🗑️  正在清理消息数据..."
        sqlite3 "$DB_PATH" "DELETE FROM messages;"
        echo "✓ 消息数据已清理"
        ;;
    2)
        echo ""
        echo "🗑️  正在清理用户数据..."
        sqlite3 "$DB_PATH" "DELETE FROM users;"
        echo "✓ 用户数据已清理"
        ;;
    3)
        echo ""
        echo "🗑️  正在清理好友数据..."
        sqlite3 "$DB_PATH" "DELETE FROM friend_requests;"
        sqlite3 "$DB_PATH" "DELETE FROM friendships;"
        sqlite3 "$DB_PATH" "DELETE FROM private_messages;"
        echo "✓ 好友数据已清理"
        ;;
    4)
        echo ""
        echo "🗑️  正在清理所有数据..."
        sqlite3 "$DB_PATH" "
            DELETE FROM messages;
            DELETE FROM users;
            DELETE FROM conversations;
            DELETE FROM rooms;
            DELETE FROM user_presets;
            DELETE FROM friendships;
            DELETE FROM friend_requests;
            DELETE FROM private_messages;
        "
        echo "✓ 所有数据已清理"
        ;;
    5)
        echo ""
        echo "🗑️  正在删除数据库文件..."
        rm -f "$DB_PATH"
        echo "✓ 数据库文件已删除"
        echo "  下次启动时将自动创建新数据库"
        ;;
    0)
        echo ""
        echo "已取消操作"
        exit 0
        ;;
    *)
        echo ""
        echo "❌ 无效的选项"
        exit 1
        ;;
esac

echo ""

# 显示数据库大小（如果文件还存在）
if [ -f "$DB_PATH" ]; then
    SIZE=$(du -h "$DB_PATH" | cut -f1)
    echo "📊 当前数据库大小：$SIZE"
fi

echo ""
echo "========================================"
echo "  清理完成！"
echo "========================================"
