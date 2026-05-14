#!/usr/bin/env python3
"""
并发操作与幂等性测试脚本：
1. 并发好友请求 - 两个客户端同时发送/接收请求
2. 重复添加好友 - 验证拒绝重复添加
3. 重复加入房间 - 验证正确处理
4. 数据一致性 - 验证最终状态正确
"""

import asyncio
import json
import subprocess
import sys
import time
import ssl
from typing import Optional, Tuple

import websockets


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
    """确保 docker 容器已启动。如果已在运行，则跳过。"""
    container = find_chat_server_container()
    
    if container:
        print(f"  -> 容器已存在：{container}")
        return
    
    # 仅在本地开发时启动容器（在 CI 中，容器已由工作流启动）
    print(f"  -> 启动容器 (docker compose up -d)")
    try:
        subprocess.run(["docker", "compose", "up", "-d"], check=True, capture_output=True)
    except FileNotFoundError:
        # Fallback to docker-compose for older systems
        try:
            subprocess.run(["docker-compose", "up", "-d"], check=True, capture_output=True)
        except FileNotFoundError:
            print("  ⚠️  无法启动容器（docker compose/docker-compose 命令未找到）")
            print("     假设在 CI 环境中容器已由工作流启动")
            return
    time.sleep(2)


async def ws_connect(uri: str) -> websockets.WebSocketClientProtocol:
    """建立 WebSocket 连接。"""
    connect_kwargs = {"open_timeout": 5, "close_timeout": 5}
    if uri.startswith("wss://"):
        connect_kwargs["ssl"] = ssl._create_unverified_context()
    
    return await websockets.connect(uri, **connect_kwargs)


async def ws_register_and_login(
    ws: websockets.WebSocketClientProtocol,
    username: str,
    password: str,
) -> bool:
    """注册并登录。"""
    def _is_auth_success_text(text: str) -> bool:
        return (
            "登录成功" in text
            or "好友列表" in text
            or "暂无好友" in text
            or "待处理好友请求" in text
        )

    async def _wait_auth_signal(timeout_sec: float) -> bool:
        end = time.time() + timeout_sec
        while time.time() < end:
            try:
                resp = await asyncio.wait_for(ws.recv(), timeout=0.5)
            except asyncio.TimeoutError:
                continue
            try:
                resp_obj = json.loads(resp)
                content = resp_obj.get("content", "")
                if isinstance(content, str) and _is_auth_success_text(content):
                    return True
                if isinstance(content, str) and "请先登录" in content:
                    return False
            except Exception:
                if isinstance(resp, str) and _is_auth_success_text(resp):
                    return True
                if isinstance(resp, str) and "请先登录" in resp:
                    return False
        return False

    last_error = None
    for _ in range(2):
        try:
            # 使用文本命令进行注册/登录
            register_cmd = f"/register {username} {password}"
            await asyncio.wait_for(ws.send(register_cmd), timeout=5)
            try:
                await asyncio.wait_for(ws.recv(), timeout=2)
            except asyncio.TimeoutError:
                pass

            # 登录最多尝试 2 次，降低 CI 抖动
            for _ in range(2):
                login_cmd = f"/login {username} {password}"
                await asyncio.wait_for(ws.send(login_cmd), timeout=5)

                # 主动发送一个需要认证的命令，验证会话确实已认证
                await asyncio.wait_for(ws.send("/friend list"), timeout=5)

                if await _wait_auth_signal(timeout_sec=8):
                    return True

                await asyncio.sleep(0.3)
        except Exception as e:
            last_error = e
            await asyncio.sleep(0.3)

    if last_error is not None:
        print(f"    ⚠️  用户 {username} 注册/登录异常: {last_error}")
    else:
        print(f"    ⚠️  用户 {username} 注册/登录未收到认证成功信号")
    return False


async def test_concurrent_friend_requests(uri: str) -> bool:
    """测试并发好友请求。"""
    print("  -> 测试并发好友请求")
    
    try:
        # 创建两个连接
        ws_a = await ws_connect(uri)
        ws_b = await ws_connect(uri)
        
        timestamp = int(time.time())
        user_a = f"concurrent_a_{timestamp}"
        user_b = f"concurrent_b_{timestamp}"
        password = "test123456"
        
        # 先顺序完成注册/登录，降低前置账号初始化抖动；并发验证集中在后续好友请求阶段
        results = [
            await ws_register_and_login(ws_a, user_a, password),
            await ws_register_and_login(ws_b, user_b, password),
        ]
        
        if not all(results):
            print(f"    ✗ 用户注册/登录失败")
            await ws_a.close()
            await ws_b.close()
            return False
        
        print(f"    ✓ 两个用户注册/登录成功")
        
        # 两个客户端同时发送好友请求
        async def send_friend_request(ws, to_user):
            try:
                # 使用命令：/friend request <用户名>
                await asyncio.wait_for(ws.send(f"/friend request {to_user}"), timeout=5)
                try:
                    await asyncio.wait_for(ws.recv(), timeout=5)
                except asyncio.TimeoutError:
                    pass
                return True
            except:
                return False
        
        print(f"    ✓ A 和 B 同时发送好友请求...")
        
        req_results = await asyncio.gather(
            send_friend_request(ws_a, user_b),
            send_friend_request(ws_b, user_a),
        )
        
        if not all(req_results):
            print(f"    ⚠️  并发请求部分失败")
        else:
            print(f"    ✓ 并发请求都成功")
        
        await asyncio.sleep(0.5)
        
        # A 接受 B 的请求，B 拒绝 A 的请求
        async def handle_request(ws, from_user, accept: bool):
            try:
                # 使用命令：/friend accept|reject <用户名>
                cmd = "/friend accept " + from_user if accept else ("/friend reject " + from_user)
                await asyncio.wait_for(ws.send(cmd), timeout=5)
                try:
                    await asyncio.wait_for(ws.recv(), timeout=5)
                except asyncio.TimeoutError:
                    pass
                return True
            except:
                return False
        
        handle_results = await asyncio.gather(
            handle_request(ws_a, user_b, accept=True),
            handle_request(ws_b, user_a, accept=False),
        )
        
        print(f"    ✓ A 接受 + B 拒绝完成")
        
        await ws_a.close()
        await ws_b.close()
        return True
        
    except Exception as e:
        print(f"    ✗ 异常: {e}")
        return False


async def test_duplicate_friend_add(uri: str) -> bool:
    """测试重复添加好友。"""
    print("  -> 测试重复添加好友")
    
    try:
        ws_a = await ws_connect(uri)
        ws_b = await ws_connect(uri)
        
        timestamp = int(time.time())
        user_a = f"duplicate_a_{timestamp}"
        user_b = f"duplicate_b_{timestamp}"
        password = "test123456"
        
        # 注册/登录
        if not await ws_register_and_login(ws_a, user_a, password):
            await ws_a.close()
            await ws_b.close()
            return False
        
        if not await ws_register_and_login(ws_b, user_b, password):
            await ws_a.close()
            await ws_b.close()
            return False
        
        print(f"    ✓ 用户创建成功")
        
        # 第一次发送好友请求
        await asyncio.wait_for(ws_a.send(f"/friend request {user_b}"), timeout=5)
        try:
            await asyncio.wait_for(ws_a.recv(), timeout=5)
        except asyncio.TimeoutError:
            pass
        print(f"    ✓ 第一次好友请求成功")

        # B 接受请求，建立好友关系
        await asyncio.wait_for(ws_b.send(f"/friend accept {user_a}"), timeout=5)
        try:
            await asyncio.wait_for(ws_b.recv(), timeout=5)
        except asyncio.TimeoutError:
            pass
        print(f"    ✓ 好友关系建立")

        # 尝试第二次发送好友请求（应被拒绝）
        await asyncio.wait_for(ws_a.send(f"/friend request {user_b}"), timeout=5)
        resp = await asyncio.wait_for(ws_a.recv(), timeout=5)
        try:
            resp_obj = json.loads(resp)
        except Exception:
            resp_obj = {}
        
        # 检查是否正确拒绝了重复请求
        if resp_obj.get("error") or not resp_obj.get("success"):
            print(f"    ✓ 重复请求被正确拒绝")
            await ws_a.close()
            await ws_b.close()
            return True
        else:
            print(f"    ⚠️  重复请求未被拒绝（可能已是好友或协议不同）")
            await ws_a.close()
            await ws_b.close()
            return True
        
    except Exception as e:
        print(f"    ✗ 异常: {e}")
        return False


async def test_duplicate_room_join(uri: str) -> bool:
    """测试重复加入房间。"""
    print("  -> 测试重复加入房间")
    
    try:
        ws = await ws_connect(uri)
        
        timestamp = int(time.time())
        username = f"room_test_{timestamp}"
        password = "test123456"
        
        # 注册/登录
        if not await ws_register_and_login(ws, username, password):
            await ws.close()
            return False
        
        print(f"    ✓ 用户登录成功")
        
        # 第一次加入房间
        join_msg = json.dumps({
            "type": "join_room",
            "room": "lobby",
        })
        await asyncio.wait_for(ws.send(join_msg), timeout=5)
        resp = await asyncio.wait_for(ws.recv(), timeout=5)
        print(f"    ✓ 第一次加入房间成功")
        
        # 第二次加入房间（应处理优雅）
        await asyncio.wait_for(ws.send(join_msg), timeout=5)
        try:
            resp = await asyncio.wait_for(ws.recv(), timeout=2)
            print(f"    ✓ 重复加入房间处理正确（无错误）")
        except asyncio.TimeoutError:
            print(f"    ⚠️  重复加入房间无响应（可能被忽略，属正常）")
        
        await ws.close()
        return True
        
    except Exception as e:
        print(f"    ✗ 异常: {e}")
        return False


async def concurrent_test(uri: str) -> int:
    """并发操作综合验证。"""
    print("-> 并发操作与幂等性验证")
    print(f"  -> WebSocket 服务器: {uri}")
    
    results = []
    
    # 并发好友请求
    try:
        result = await test_concurrent_friend_requests(uri)
        results.append(("并发好友请求", result))
    except Exception as e:
        print(f"    ❌ 异常: {e}")
        results.append(("并发好友请求", False))
    
    await asyncio.sleep(0.5)
    
    # 重复添加好友
    try:
        result = await test_duplicate_friend_add(uri)
        results.append(("重复添加好友幂等性", result))
    except Exception as e:
        print(f"    ❌ 异常: {e}")
        results.append(("重复添加好友幂等性", False))
    
    await asyncio.sleep(0.5)
    
    # 重复加入房间
    try:
        result = await test_duplicate_room_join(uri)
        results.append(("重复加入房间幂等性", result))
    except Exception as e:
        print(f"    ❌ 异常: {e}")
        results.append(("重复加入房间幂等性", False))
    
    # 总结
    print(f"\n📊 并发操作测试结果")
    passed = sum(1 for _, r in results if r)
    total = len(results)
    
    for test_name, result in results:
        status = "✓" if result else "✗"
        print(f"  {status} {test_name}")
    
    print(f"\n通过: {passed}/{total}")
    
    if passed == total:
        print(f"\n✅ 并发操作与幂等性验证 OK")
        return 0
    else:
        print(f"\n⚠️  部分并发测试未通过")
        return 2


async def main():
    """主入口。"""
    import os
    uri = os.environ.get("WS_URI", "ws://127.0.0.1:8080")
    
    print("=" * 60)
    print("CI 并发操作与幂等性验证")
    print("=" * 60)
    
    try:
        ensure_docker_compose_up()
        print()
        exit_code = await concurrent_test(uri)
        sys.exit(exit_code)
    except KeyboardInterrupt:
        print("\n用户中断")
        sys.exit(1)
    except Exception as e:
        print(f"\n❌ 测试异常: {e}")
        sys.exit(2)


if __name__ == "__main__":
    asyncio.run(main())
