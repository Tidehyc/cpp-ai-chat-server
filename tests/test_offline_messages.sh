#!/bin/bash

# 离线消息测试脚本
# 测试场景：
# 1. 用户A和用户B都在线，A发送消息给B - 应该立即收到
# 2. 用户B下线
# 3. 用户A发送消息给B - 应该存储为离线消息
# 4. 用户B重新上线 - 应该收到离线消息

SERVER="ws://localhost:8080"

echo "=== 离线消息测试 ==="
echo ""

# 启动服务器（如果未运行）
if ! pgrep -f "chat_server" > /dev/null; then
    echo "启动服务器..."
    cd /home/tide/workplace/cpp-ai-chat-server/build
    ./chat_server &
    sleep 2
fi

echo "测试需要手动进行："
echo ""
echo "1. 打开两个终端窗口"
echo ""
echo "2. 在终端1，用户A登录："
echo "   ./ws_client"
echo "   /login userA password123"
echo ""
echo "3. 在终端2，用户B登录："
echo "   ./ws_client"
echo "   /login userB password123"
echo ""
echo "4. 用户A发送消息给用户B："
echo "   /pm userB 你好，这是第一条消息"
echo ""
echo "5. 用户B下线（Ctrl+C）"
echo ""
echo "6. 用户A继续发送消息："
echo "   /pm userB 这是离线消息1"
echo "   /pm userB 这是离线消息2"
echo ""
echo "7. 用户B重新登录："
echo "   ./ws_client"
echo "   /login userB password123"
echo ""
echo "8. 检查用户B是否收到离线消息"
echo ""
echo "查看服务器日志："
echo "   tail -f /home/tide/workplace/cpp-ai-chat-server/build/chat_server.log | grep '离线消息'"
