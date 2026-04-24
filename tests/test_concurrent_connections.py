#!/usr/bin/env python3
"""并发连接稳定性测试

场景：
1. 同时创建多个 WebSocket 客户端连接。
2. 每个连接接收欢迎消息后发送 /rooms_json。
3. 验证服务端返回 room_list_update，确保并发连接下请求可稳定响应。
"""

import argparse
import asyncio
import json
from dataclasses import dataclass
from typing import List

import websockets


@dataclass
class ClientResult:
    client_id: int
    ok: bool
    error: str = ""


async def run_client(client_id: int, uri: str, timeout: float) -> ClientResult:
    try:
        async with websockets.connect(uri, subprotocols=["chat"], open_timeout=timeout, close_timeout=timeout) as ws:
            welcome_raw = await asyncio.wait_for(ws.recv(), timeout=timeout)
            welcome = json.loads(welcome_raw)
            if welcome.get("type") != "system":
                return ClientResult(client_id, False, f"unexpected welcome type: {welcome.get('type')}")

            await ws.send("/rooms_json")

            # /rooms_json 应该返回单条 room_list_update
            response_raw = await asyncio.wait_for(ws.recv(), timeout=timeout)
            response = json.loads(response_raw)
            if response.get("type") != "room_list_update":
                return ClientResult(client_id, False, f"unexpected response type: {response.get('type')}")

            rooms = response.get("rooms", [])
            if not isinstance(rooms, list) or len(rooms) == 0:
                return ClientResult(client_id, False, "rooms is empty")

            return ClientResult(client_id, True)
    except Exception as exc:  # pylint: disable=broad-except
        return ClientResult(client_id, False, str(exc))


async def run_wave(wave_id: int, clients: int, uri: str, timeout: float) -> List[ClientResult]:
    tasks = [run_client(i, uri, timeout) for i in range(clients)]
    results = await asyncio.gather(*tasks)

    ok_count = sum(1 for r in results if r.ok)
    print(f"[wave {wave_id}] ok={ok_count}/{clients}")

    failed = [r for r in results if not r.ok]
    for item in failed[:5]:
        print(f"  - client#{item.client_id} failed: {item.error}")

    return results


async def main() -> int:
    parser = argparse.ArgumentParser(description="Concurrent websocket connection stability test")
    parser.add_argument("--uri", default="ws://127.0.0.1:8080", help="WebSocket URI")
    parser.add_argument("--clients", type=int, default=30, help="Concurrent clients per wave")
    parser.add_argument("--waves", type=int, default=3, help="Number of waves")
    parser.add_argument("--timeout", type=float, default=5.0, help="Per-step timeout in seconds")
    args = parser.parse_args()

    total = 0
    failed = 0

    for wave in range(1, args.waves + 1):
        results = await run_wave(wave, args.clients, args.uri, args.timeout)
        total += len(results)
        failed += sum(1 for r in results if not r.ok)

    print(f"[summary] total={total}, failed={failed}, success_rate={(total - failed) * 100.0 / max(total, 1):.2f}%")
    return 0 if failed == 0 else 1


if __name__ == "__main__":
    raise SystemExit(asyncio.run(main()))
