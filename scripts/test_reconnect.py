#!/usr/bin/env python3
"""简单断线重连测试客户端：持续尝试连接并在断开后重连，打印事件日志。"""

import asyncio
import argparse
import time
import ssl

import websockets


async def run_client(uri: str):
    attempt = 0
    while True:
        attempt += 1
        try:
            print(f"[{time.strftime('%X')}] Attempt {attempt}: connecting to {uri}")
            connect_kwargs = {"subprotocols": ["chat"], "open_timeout": 5, "close_timeout": 5}
            if uri.startswith("wss://"):
                connect_kwargs["ssl"] = ssl._create_unverified_context()

            async with websockets.connect(uri, **connect_kwargs) as ws:
                print(f"[{time.strftime('%X')}] connected")
                while True:
                    try:
                        msg = await asyncio.wait_for(ws.recv(), timeout=5)
                        print(f"[{time.strftime('%X')}] recv: {msg}")
                    except asyncio.TimeoutError:
                        # 发送心跳保持连接
                        try:
                            await ws.send("/ping")
                        except Exception as e:
                            print(f"[{time.strftime('%X')}] send failed: {e}")
                            break
        except Exception as exc:
            print(f"[{time.strftime('%X')}] disconnected: {exc}; retrying in 2s")
            await asyncio.sleep(2)


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--uri", default="ws://127.0.0.1:8080", help="WebSocket URI")
    args = parser.parse_args()

    try:
        asyncio.run(run_client(args.uri))
    except KeyboardInterrupt:
        print("exiting")
