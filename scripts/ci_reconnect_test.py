#!/usr/bin/env python3
"""
自动化断线重连测试脚本：
1. 连接、注册/登录到服务器
2. 发送消息（验证连接正常）
3. 模拟主动断线（关闭 socket）
4. 等待自动重连（监测连接状态）
5. 重连后继续发送消息验证功能恢复
"""

import asyncio
import json
import subprocess
import sys
import time
import ssl
import re
from typing import Optional

import websockets


def compose_cmd() -> str:
    """检测本地 docker-compose 或 docker compose 命令。"""
    result = subprocess.run(["docker-compose", "--version"], capture_output=True)
    if result.returncode == 0:
        return "docker-compose"
    result = subprocess.run(["docker", "compose", "--version"], capture_output=True)
    if result.returncode == 0:
        return "docker"
    raise RuntimeError("无法找到 docker-compose 或 docker compose 命令")


def find_chat_server_container() -> Optional[str]:
    """查找运行中的 chat-server 容器名称。"""
    result = subprocess.run(
        ["docker", "ps", "-a", "--format", "{{.Names}}"],
        capture_output=True,
        text=True,
    )
    if result.returncode != 0:
        return None
    for line in result.stdout.strip().split("\n"):
        if "chat-server" in line or "chat_server" in line:
            return line
    return None


def ensure_docker_compose_up() -> None:
    """确保 docker-compose 容器已启动。"""
    compose = compose_cmd()
    container = find_chat_server_container()
    
    if container:
        print(f"  -> 容器已存在：{container}")
        return
    
    print(f"  -> 启动容器 (docker compose up -d)")
    if compose == "docker-compose":
        subprocess.run(["docker-compose", "up", "-d"], check=True)
    else:
        subprocess.run(["docker", "compose", "up", "-d"], check=True)
    time.sleep(2)


async def ws_connect(uri: str) -> websockets.WebSocketClientProtocol:
    """建立 WebSocket 连接。"""
    connect_kwargs = {"subprotocols": ["chat"], "open_timeout": 5, "close_timeout": 5}
    if uri.startswith("wss://"):
        connect_kwargs["ssl"] = ssl._create_unverified_context()
    
    return await websockets.connect(uri, **connect_kwargs)


async def ws_register_and_login(
    ws: websockets.WebSocketClientProtocol,
    username: str,
    password: str,
    timeout: float = 5.0,
) -> bool:
    """通过 WebSocket 注册和登录。"""
    try:
        # 发送注册请求
        register_msg = json.dumps({
            "type": "register",
            "username": username,
            "password": password,
        })
        await asyncio.wait_for(ws.send(register_msg), timeout=timeout)
        
        # 接收注册响应
        response = await asyncio.wait_for(ws.recv(), timeout=timeout)
        resp_obj = json.loads(response)
        
        # 如果已存在，直接登录；否则注册成功
        if resp_obj.get("type") == "register" and resp_obj.get("success"):
            print(f"    ✓ 新用户 {username} 注册成功")
        else:
            print(f"    ✓ 用户 {username} 已存在或注册成功")
        
        # 发送登录请求
        login_msg = json.dumps({
            "type": "login",
            "username": username,
            "password": password,
        })
        await asyncio.wait_for(ws.send(login_msg), timeout=timeout)
        
        # 接收登录响应
        response = await asyncio.wait_for(ws.recv(), timeout=timeout)
        resp_obj = json.loads(response)
        
        if resp_obj.get("type") == "login" and resp_obj.get("success"):
            print(f"    ✓ 登录成功")
            return True
        else:
            print(f"    ✗ 登录失败: {resp_obj}")
            return False
    except asyncio.TimeoutError:
        print(f"    ✗ 超时")
        return False
    except Exception as e:
        print(f"    ✗ 异常: {e}")
        return False


async def ws_send_message(
    ws: websockets.WebSocketClientProtocol,
    room: str,
    text: str,
    timeout: float = 5.0,
) -> bool:
    """通过 WebSocket 发送消息。"""
    try:
        msg = json.dumps({
            "type": "message",
            "room": room,
            "text": text,
        })
        await asyncio.wait_for(ws.send(msg), timeout=timeout)
        return True
    except Exception as e:
        print(f"    ✗ 发送消息失败: {e}")
        return False


async def reconnect_test(uri: str) -> int:
    """
    主测试流程：
    1. 连接 → 注册/登录 → 发送消息
    2. 模拟断线（关闭连接）
    3. 尝试重连（最多 10 次，每次间隔 1s）
    4. 重连后发送验证消息
    """
    print("-> 断线重连测试")
    print(f"  -> WebSocket 服务器: {uri}")
    
    username = f"reconnect_test_{int(time.time())}"
    password = "test123456"
    
    # 步骤 1: 连接、注册、登录、发送初始消息
    print("  -> 步骤1：初次连接、注册、登录、发送消息")
    try:
        ws = await ws_connect(uri)
        print("    ✓ 连接成功")
    except Exception as e:
        print(f"    ✗ 连接失败: {e}")
        return 2
    
    if not await ws_register_and_login(ws, username, password):
        await ws.close()
        return 2
    
    # 发送初始消息
    if not await ws_send_message(ws, "lobby", "test_msg_before_disconnect"):
        await ws.close()
        return 2
    print(f"    ✓ 发送初始消息成功")
    
    # 步骤 2: 模拟断线
    print("  -> 步骤2：主动断线")
    disconnect_time = time.time()
    await ws.close()
    print(f"    ✓ 连接已关闭")
    
    # 步骤 3: 尝试重连（模拟客户端自动重连机制）
    print("  -> 步骤3：尝试重连")
    reconnected = False
    reconnect_attempts = 0
    reconnect_delay = 0
    
    for attempt in range(1, 11):
        await asyncio.sleep(1)
        reconnect_attempts += 1
        try:
            ws = await ws_connect(uri)
            reconnected = True
            reconnect_delay = time.time() - disconnect_time
            print(f"    ✓ 第 {attempt} 次尝试重连成功（耗时 {reconnect_delay:.2f}s）")
            break
        except Exception as e:
            print(f"    - 第 {attempt} 次尝试失败: {e}")
    
    if not reconnected:
        print(f"    ✗ 重连失败（经过 {reconnect_attempts} 次尝试）")
        return 2
    
    # 步骤 4: 重连后重新登录、发送消息
    print("  -> 步骤4：重连后登录、发送消息")
    if not await ws_register_and_login(ws, username, password):
        await ws.close()
        return 2
    
    if not await ws_send_message(ws, "lobby", "test_msg_after_reconnect"):
        await ws.close()
        return 2
    print(f"    ✓ 重连后消息发送成功")
    
    await ws.close()
    
    # 验证结果
    print(f"\n✅ 断线重连测试 OK")
    print(f"   - 重连延迟: {reconnect_delay:.2f}s")
    print(f"   - 重连尝试次数: {reconnect_attempts}")
    return 0


async def main():
    """主入口。"""
    uri = "ws://127.0.0.1:8080"
    
    print("=" * 60)
    print("CI 断线重连测试")
    print("=" * 60)
    
    try:
        ensure_docker_compose_up()
        print()
        exit_code = await reconnect_test(uri)
        sys.exit(exit_code)
    except KeyboardInterrupt:
        print("\n用户中断")
        sys.exit(1)
    except Exception as e:
        print(f"\n❌ 测试异常: {e}")
        sys.exit(2)


if __name__ == "__main__":
    asyncio.run(main())
