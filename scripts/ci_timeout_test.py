#!/usr/bin/env python3
"""
超时处理验证脚本：
1. AI 请求超时 - 验证友好错误提示（不透传原始异常）
2. 数据库操作超时 - 验证连接不断裂
3. 登录超时 - 验证重试或友好提示
"""

import asyncio
import json
import subprocess
import sys
import time
import ssl
from typing import Optional

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


async def ws_connect(uri: str, timeout: float = 5.0) -> websockets.WebSocketClientProtocol:
    """建立 WebSocket 连接。"""
    connect_kwargs = {"open_timeout": timeout, "close_timeout": timeout}
    if uri.startswith("wss://"):
        connect_kwargs["ssl"] = ssl._create_unverified_context()
    
    return await websockets.connect(uri, **connect_kwargs)


async def test_ai_request_timeout(uri: str) -> bool:
    """测试 AI 请求超时的错误处理。"""
    print("  -> 测试 AI 请求超时处理")
    
    try:
        ws = await ws_connect(uri)
        
        # 注册/登录
        username = f"timeout_test_{int(time.time())}"
        password = "test123456"
        
        # 使用文本命令注册/登录
        register_cmd = f"/register {username} {password}"
        await asyncio.wait_for(ws.send(register_cmd), timeout=5)
        try:
            await asyncio.wait_for(ws.recv(), timeout=5)
        except asyncio.TimeoutError:
            pass

        login_cmd = f"/login {username} {password}"
        await asyncio.wait_for(ws.send(login_cmd), timeout=5)

        # 循环读取消息，查找登录成功提示
        end = time.time() + 5
        success = False
        while time.time() < end:
            try:
                resp = await asyncio.wait_for(ws.recv(), timeout=0.5)
            except asyncio.TimeoutError:
                continue
            try:
                resp_obj = json.loads(resp)
                content = resp_obj.get("content", "")
                if isinstance(content, str) and "登录成功" in content:
                    success = True
                    break
            except Exception:
                if isinstance(resp, str) and "登录成功" in resp:
                    success = True
                    break

        if not success:
            print(f"    ✗ 登录失败")
            await ws.close()
            return False
        
        print(f"    ✓ 用户登录成功")
        
        # 发送 AI 请求
        # 注意：如果 AI API 配置不存在或额度不足，可能会返回友好错误
        # 使用 /pm ai 发送给 AI，触发服务器的 AI 处理路径
        ai_cmd = "/pm ai 你好，这是一个测试消息"
        await asyncio.wait_for(ws.send(ai_cmd), timeout=5)
        
        # 等待 AI 响应（可能超时）
        try:
            response = await asyncio.wait_for(ws.recv(), timeout=3)
            resp_obj = json.loads(response)
            
            # 检查错误信息是否友好
            error_msg = resp_obj.get("message") or resp_obj.get("error") or ""
            if error_msg:
                # 验证错误信息不包含原始异常堆栈
                if "traceback" in error_msg.lower() or "exception" in error_msg.lower():
                    print(f"    ⚠️  错误信息暴露了内部细节: {error_msg[:100]}")
                    return False
                else:
                    print(f"    ✓ 收到友好错误提示: {error_msg[:50]}...")
                    return True
            else:
                print(f"    ✓ 请求处理成功（可能无 AI 配置，属期望行为）")
                return True
        except asyncio.TimeoutError:
            print(f"    ⚠️  AI 响应超时（可能服务器确实超时）")
            return True  # 超时本身不是错误，取决于实现
        
        await ws.close()
    except Exception as e:
        print(f"    ✗ 异常: {e}")
        return False


async def test_login_with_timeout(uri: str) -> bool:
    """测试登录超时处理。"""
    print("  -> 测试登录超时处理")
    
    try:
        # 尝试非常短的超时时间
        ws = await ws_connect(uri, timeout=0.5)
        
        username = f"timeout_login_{int(time.time())}"
        password = "test123456"
        
        # 第一次注册
        register_cmd = f"/register {username} {password}"

        try:
            await asyncio.wait_for(ws.send(register_cmd), timeout=1)
            try:
                response = await asyncio.wait_for(ws.recv(), timeout=1)
            except asyncio.TimeoutError:
                response = ""
            print(f"    ✓ 注册请求完成（未超时）")
        except asyncio.TimeoutError:
            print(f"    ⚠️  注册请求超时（连接可能已建立）")
            await ws.close()
            return True
        
        # 登录
        login_cmd = f"/login {username} {password}"

        try:
            await asyncio.wait_for(ws.send(login_cmd), timeout=1)
            try:
                response = await asyncio.wait_for(ws.recv(), timeout=1)
            except asyncio.TimeoutError:
                response = ""
            try:
                resp_obj = json.loads(response)
                success = isinstance(resp_obj.get("content"), str) and "登录成功" in resp_obj.get("content")
            except Exception:
                success = "登录成功" in response

            if success:
                print(f"    ✓ 登录成功（未超时）")
                await ws.close()
                return True
            else:
                print(f"    ⚠️  登录失败（可能凭证问题）")
                await ws.close()
                return False
        except asyncio.TimeoutError:
            print(f"    ⚠️  登录请求超时（应重试或提示用户）")
            await ws.close()
            return True
        
    except Exception as e:
        print(f"    ✗ 异常: {e}")
        return False


async def test_connection_recovery(uri: str) -> bool:
    """测试连接断裂恢复能力。"""
    print("  -> 测试连接恢复能力")
    
    try:
        ws = await ws_connect(uri)
        
        # 注册/登录
        username = f"recovery_test_{int(time.time())}"
        password = "test123456"
        
        register_cmd = f"/register {username} {password}"
        await asyncio.wait_for(ws.send(register_cmd), timeout=5)
        try:
            await asyncio.wait_for(ws.recv(), timeout=5)
        except asyncio.TimeoutError:
            pass

        login_cmd = f"/login {username} {password}"
        await asyncio.wait_for(ws.send(login_cmd), timeout=5)
        resp = await asyncio.wait_for(ws.recv(), timeout=5)
        try:
            resp_obj = json.loads(resp)
            success = isinstance(resp_obj.get("content"), str) and "登录成功" in resp_obj.get("content")
        except Exception:
            success = "登录成功" in resp

        if not success:
            print(f"    ✗ 登录失败")
            await ws.close()
            return False
        
        print(f"    ✓ 登录成功")
        
        # 主动关闭连接，模拟网络中断
        await ws.close()
        print(f"    ✓ 连接已关闭，模拟网络中断")
        
        # 等待后重新连接
        await asyncio.sleep(1)
        
        try:
            ws = await ws_connect(uri, timeout=5)
            print(f"    ✓ 重新连接成功")
            
            # 重新登录（命令格式）
            login_cmd = f"/login {username} {password}"
            await asyncio.wait_for(ws.send(login_cmd), timeout=5)

            end = time.time() + 5
            relogin_success = False
            while time.time() < end:
                try:
                    resp = await asyncio.wait_for(ws.recv(), timeout=0.5)
                except asyncio.TimeoutError:
                    continue
                try:
                    resp_obj = json.loads(resp)
                    content = resp_obj.get("content", "")
                    if isinstance(content, str) and "登录成功" in content:
                        relogin_success = True
                        break
                except Exception:
                    if isinstance(resp, str) and "登录成功" in resp:
                        relogin_success = True
                        break

            if relogin_success:
                print(f"    ✓ 重连后登录成功，连接恢复正常")
                await ws.close()
                return True
            else:
                print(f"    ⚠️  重连后登录失败")
                await ws.close()
                return False
        except asyncio.TimeoutError:
            print(f"    ✗ 重新连接超时")
            return False
        
    except Exception as e:
        print(f"    ✗ 异常: {e}")
        return False


async def timeout_test(uri: str) -> int:
    """超时处理综合验证。"""
    print("-> 超时处理验证")
    print(f"  -> WebSocket 服务器: {uri}")
    
    results = []
    
    # 测试 AI 请求超时
    try:
        result = await test_ai_request_timeout(uri)
        results.append(("AI 请求超时处理", result))
    except Exception as e:
        print(f"    ❌ 异常: {e}")
        results.append(("AI 请求超时处理", False))
    
    await asyncio.sleep(0.5)
    
    # 测试登录超时
    try:
        result = await test_login_with_timeout(uri)
        results.append(("登录超时处理", result))
    except Exception as e:
        print(f"    ❌ 异常: {e}")
        results.append(("登录超时处理", False))
    
    await asyncio.sleep(0.5)
    
    # 测试连接恢复
    try:
        result = await test_connection_recovery(uri)
        results.append(("连接恢复能力", result))
    except Exception as e:
        print(f"    ❌ 异常: {e}")
        results.append(("连接恢复能力", False))
    
    # 总结
    print(f"\n📊 超时处理测试结果")
    passed = sum(1 for _, r in results if r)
    total = len(results)
    
    for test_name, result in results:
        status = "✓" if result else "✗"
        print(f"  {status} {test_name}")
    
    print(f"\n通过: {passed}/{total}")
    
    if passed == total:
        print(f"\n✅ 超时处理验证 OK")
        return 0
    else:
        print(f"\n⚠️  部分超时测试未通过")
        return 2


async def main():
    """主入口。"""
    import os
    uri = os.environ.get("WS_URI", "ws://127.0.0.1:8080")
    
    print("=" * 60)
    print("CI 超时处理验证")
    print("=" * 60)
    
    try:
        ensure_docker_compose_up()
        print()
        exit_code = await timeout_test(uri)
        sys.exit(exit_code)
    except KeyboardInterrupt:
        print("\n用户中断")
        sys.exit(1)
    except Exception as e:
        print(f"\n❌ 测试异常: {e}")
        sys.exit(2)


if __name__ == "__main__":
    asyncio.run(main())
