#!/usr/bin/env python3
"""快速测试 AI 功能"""

import asyncio
import websockets
import json

async def test_ai():
    uri = "ws://localhost:8080"
    
    print("🔗 连接到服务器...")
    async with websockets.connect(uri, subprotocols=["chat"]) as ws:
        print("✅ 连接成功！\n")
        
        # 测试消息
        messages = [
            "你好",
            "@AI 你好，请介绍一下你自己",
            "@AI 今天天气怎么样？",
        ]
        
        for msg in messages:
            print(f"📤 发送：{msg}")
            data = {"type": "chat", "from": "test_user", "to": "ai" if "@AI" in msg else "", "content": msg}
            await ws.send(json.dumps(data))
            
            # 接收响应
            try:
                response = await asyncio.wait_for(ws.recv(), timeout=30.0)
                resp_data = json.loads(response)
                print(f"📥 接收：{resp_data.get('content', response)[:100]}...")
            except asyncio.TimeoutError:
                print("⚠️  响应超时")
            print("-" * 50)
        
        print("\n✅ 测试完成！")

if __name__ == "__main__":
    asyncio.run(test_ai())
