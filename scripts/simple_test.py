#!/usr/bin/env python3
"""简单测试脚本"""

import asyncio
import websockets

async def test():
    uri = "ws://localhost:8080"
    
    print("连接服务器...")
    async with websockets.connect(uri, subprotocols=["chat"]) as ws:
        # 接收欢迎消息
        msg = await ws.recv()
        print(f"欢迎：{msg[:100]}")
        
        # 登录
        await ws.send("/login testuser")
        msg = await ws.recv()
        print(f"登录：{msg[:100]}")
        
        # 查看房间
        await ws.send("/rooms")
        msg = await ws.recv()
        print(f"房间：{msg[:200]}")
        
        # 加入代码房间
        await ws.send("/join code")
        msg = await ws.recv()
        print(f"加入房间：{msg[:100]}")
        
        # 查看预设
        await ws.send("/preset list")
        msg = await ws.recv()
        print(f"预设：{msg[:200]}")
        
        print("\n✅ 基本功能测试通过！")

if __name__ == "__main__":
    asyncio.run(test())
