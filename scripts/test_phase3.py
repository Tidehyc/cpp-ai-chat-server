#!/usr/bin/env python3
"""阶段 3 测试脚本"""

import asyncio
import websockets
import json

async def test_phase3():
    uri = "ws://localhost:8080"
    
    print("=" * 60)
    print("阶段 3 功能测试")
    print("=" * 60)
    
    async with websockets.connect(uri, subprotocols=["chat"]) as ws:
        print("\n✅ 连接成功\n")
        
        # 测试 1: 欢迎消息
        print("--- 测试 1: 欢迎消息 ---")
        msg = await ws.recv()
        print(f"收到：{json.loads(msg).get('content', '')}\n")
        
        # 测试 2: 注册
        print("--- 测试 2: 注册 ---")
        await ws.send("/register testuser")
        msg = await ws.recv()
        print(f"发送：/register testuser")
        print(f"收到：{json.loads(msg).get('content', '')}\n")
        
        # 测试 3: 登录
        print("--- 测试 3: 登录 ---")
        await ws.send("/login testuser")
        msg = await ws.recv()
        print(f"发送：/login testuser")
        print(f"收到：{json.loads(msg).get('content', '')}\n")
        
        # 测试 4: 帮助
        print("--- 测试 4: 帮助 ---")
        await ws.send("/help")
        msg = await ws.recv()
        print(f"发送：/help")
        content = json.loads(msg).get('content', '')
        print(f"收到：\n{content}\n")
        
        # 测试 5: 查看房间
        print("--- 测试 5: 查看房间 ---")
        await ws.send("/rooms")
        msg = await ws.recv()
        print(f"发送：/rooms")
        content = json.loads(msg).get('content', '')
        print(f"收到：\n{content}\n")
        
        # 测试 6: 加入代码房间
        print("--- 测试 6: 加入代码房间 ---")
        await ws.send("/join code")
        msg = await ws.recv()
        print(f"发送：/join code")
        print(f"收到：{json.loads(msg).get('content', '')}\n")
        
        # 测试 7: 在代码房间提问
        print("--- 测试 7: 在代码房间提问 ---")
        await ws.send("写一个 Python 的 hello world")
        print(f"发送：写一个 Python 的 hello world")
        
        # 接收"AI 思考中"
        msg = await ws.recv()
        data = json.loads(msg)
        print(f"收到 [{data.get('type')}]: {data.get('content', '')}")
        
        # 接收 AI 回复
        msg = await ws.recv()
        data = json.loads(msg)
        content = data.get('content', '')[:200]
        print(f"收到 [{data.get('type')}]: {content}...\n")
        
        # 测试 8: 查看预设
        print("--- 测试 8: 查看预设 ---")
        await ws.send("/preset list")
        msg = await ws.recv()
        print(f"发送：/preset list")
        content = json.loads(msg).get('content', '')
        print(f"收到：\n{content}\n")
        
        # 测试 9: 切换预设
        print("--- 测试 9: 切换预设 ---")
        await ws.send("/preset use teacher")
        msg = await ws.recv()
        print(f"发送：/preset use teacher")
        print(f"收到：{json.loads(msg).get('content', '')}\n")
        
        # 测试 10: 私聊 AI
        print("--- 测试 10: 私聊 AI ---")
        await ws.send("/ai 解释一下什么是量子纠缠")
        print(f"发送：/ai 解释一下什么是量子纠缠")
        
        # 接收"AI 思考中"
        msg = await ws.recv()
        data = json.loads(msg)
        print(f"收到 [{data.get('type')}]: {data.get('content', '')}")
        
        # 接收 AI 回复
        msg = await ws.recv()
        data = json.loads(msg)
        content = data.get('content', '')[:200]
        print(f"收到 [{data.get('type')}]: {content}...\n")
        
        print("=" * 60)
        print("✅ 测试完成！")
        print("=" * 60)

if __name__ == "__main__":
    try:
        asyncio.run(test_phase3())
    except Exception as e:
        print(f"❌ 测试失败：{e}")
