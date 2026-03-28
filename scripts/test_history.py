#!/usr/bin/env python3
"""测试消息历史模块"""

import sqlite3
import time
import os

DB_PATH = "./chat.db"

def init_test_db():
    """初始化测试数据库"""
    conn = sqlite3.connect(DB_PATH)
    c = conn.cursor()
    
    # 创建消息表
    c.execute('''
        CREATE TABLE IF NOT EXISTS messages (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            from_user TEXT NOT NULL,
            to_user TEXT NOT NULL,
            room_name TEXT,
            content TEXT NOT NULL,
            message_type TEXT DEFAULT 'text',
            timestamp INTEGER NOT NULL
        )
    ''')
    
    # 插入测试数据
    test_messages = [
        ('alice', 'AI', '#general', '你好', 'text', int(time.time()) - 3600),
        ('AI', 'alice', '#general', '你好！有什么可以帮助你的？', 'ai_response', int(time.time()) - 3500),
        ('bob', 'AI', '#code', '写个快速排序', 'text', int(time.time()) - 1800),
        ('AI', 'bob', '#code', 'def quicksort(arr):...', 'ai_response', int(time.time()) - 1700),
        ('alice', 'bob', '#general', '在吗？', 'text', int(time.time()) - 600),
        ('bob', 'alice', '#general', '在的！', 'text', int(time.time()) - 500),
    ]
    
    c.executemany('''
        INSERT INTO messages (from_user, to_user, room_name, content, message_type, timestamp)
        VALUES (?, ?, ?, ?, ?, ?)
    ''', test_messages)
    
    conn.commit()
    conn.close()
    print("✅ 测试数据库初始化完成")

def test_get_history():
    """测试获取历史消息"""
    conn = sqlite3.connect(DB_PATH)
    c = conn.cursor()
    
    # 测试 1：获取房间历史
    print("\n=== 测试 1: 获取 #code 房间历史 ===")
    c.execute('''
        SELECT from_user, content, timestamp
        FROM messages
        WHERE room_name = '#code'
        ORDER BY timestamp DESC
        LIMIT 10
    ''')
    for row in c.fetchall():
        print(f"  [{row[2]}] {row[0]}: {row[1]}")
    
    # 测试 2：获取用户历史
    print("\n=== 测试 2: 获取 alice 的消息 ===")
    c.execute('''
        SELECT room_name, content, timestamp
        FROM messages
        WHERE from_user = 'alice' OR to_user = 'alice'
        ORDER BY timestamp DESC
        LIMIT 10
    ''')
    for row in c.fetchall():
        print(f"  [{row[2]}] #{row[0]} {row[1]}")
    
    # 测试 3：关键词搜索
    print("\n=== 测试 3: 搜索包含'快速排序'的消息 ===")
    c.execute('''
        SELECT from_user, room_name, content
        FROM messages
        WHERE content LIKE '%快速排序%'
    ''')
    for row in c.fetchall():
        print(f"  [{row[1]}] {row[0]}: {row[2]}")
    
    conn.close()

def test_export():
    """测试导出功能"""
    conn = sqlite3.connect(DB_PATH)
    c = conn.cursor()
    
    print("\n=== 测试 4: 导出 #general 房间记录 (TXT 格式) ===")
    c.execute('''
        SELECT from_user, content, timestamp
        FROM messages
        WHERE room_name = '#general'
        ORDER BY timestamp
    ''')
    
    for row in c.fetchall():
        time_str = time.strftime('%Y-%m-%d %H:%M:%S', time.localtime(row[2]))
        print(f"  [{time_str}] {row[0]}: {row[1]}")
    
    print("\n=== 测试 5: 导出 JSON 格式示例 ===")
    c.execute('''
        SELECT from_user, to_user, content, timestamp
        FROM messages
        LIMIT 3
    ''')
    
    print("  {")
    print("    \"messages\": [")
    rows = c.fetchall()
    for i, row in enumerate(rows):
        comma = "," if i < len(rows) - 1 else ""
        print(f"      {{\"from\": \"{row[0]}\", \"to\": \"{row[1]}\", \"content\": \"{row[2]}\", \"timestamp\": {row[3]}}}{comma}")
    print("    ]")
    print("  }")
    
    conn.close()

if __name__ == "__main__":
    os.chdir("/home/tide/workplace/cpp-ai-chat-server/build")
    
    print("=" * 60)
    print("阶段 4.3 消息历史模块 - 基础测试")
    print("=" * 60)
    
    # 初始化测试数据库
    init_test_db()
    
    # 运行测试
    test_get_history()
    test_export()
    
    print("\n" + "=" * 60)
    print("✅ 测试完成！")
    print("=" * 60)
    print("\n说明：")
    print("  - 测试的是数据库查询逻辑")
    print("  - C++ 模块已编译通过")
    print("  - 还需要集成到服务器和实现命令")
