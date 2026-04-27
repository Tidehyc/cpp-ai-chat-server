#!/usr/bin/env python3
"""并发连接稳定性测试。"""

import argparse
import asyncio
import json
import ssl
import sys
from dataclasses import dataclass
from typing import List

import websockets


@dataclass
class ClientResult:
    client_id: int
    ok: bool
    error: str = ""


async def run_client(client_id: int, uri: str, timeout: float, mode: str) -> ClientResult:
    try:
        connect_kwargs = {
            "subprotocols": ["chat"],
            "open_timeout": timeout,
            "close_timeout": timeout,
        }
        if uri.startswith("wss://"):
            connect_kwargs["ssl"] = ssl._create_unverified_context()

        async with websockets.connect(uri, **connect_kwargs) as ws:
            welcome_raw = await asyncio.wait_for(ws.recv(), timeout=timeout)
            welcome = json.loads(welcome_raw)
            if welcome.get("type") != "system":
                return ClientResult(client_id, False, f"unexpected welcome type: {welcome.get('type')}")

            if mode == "rooms_json":
                await ws.send("/rooms_json")

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


async def run_wave(wave_id: int, clients: int, uri: str, timeout: float, mode: str) -> List[ClientResult]:
    tasks = [run_client(i, uri, timeout, mode) for i in range(clients)]
    results = await asyncio.gather(*tasks)

    ok_count = sum(1 for r in results if r.ok)
    print(f"[wave {wave_id}] ok={ok_count}/{clients}")

    failed = [r for r in results if not r.ok]
    for item in failed[:5]:
        print(f"  - client#{item.client_id} failed: {item.error}")

    return results


async def run_wave_batched(
    wave_id: int,
    clients: int,
    uri: str,
    timeout: float,
    mode: str,
    batch_size: int,
    batch_interval_ms: int,
) -> List[ClientResult]:
    results: List[ClientResult] = []
    next_client_id = 0
    batch_index = 0

    while next_client_id < clients:
        batch_index += 1
        current = min(batch_size, clients - next_client_id)
        tasks = [run_client(next_client_id + i, uri, timeout, mode) for i in range(current)]
        batch_results = await asyncio.gather(*tasks)
        results.extend(batch_results)
        next_client_id += current

        batch_ok = sum(1 for r in batch_results if r.ok)
        print(f"  [wave {wave_id} batch {batch_index}] ok={batch_ok}/{current}")

        if next_client_id < clients and batch_interval_ms > 0:
            await asyncio.sleep(batch_interval_ms / 1000.0)

    ok_count = sum(1 for r in results if r.ok)
    print(f"[wave {wave_id}] ok={ok_count}/{clients}")

    failed = [r for r in results if not r.ok]
    for item in failed[:5]:
        print(f"  - client#{item.client_id} failed: {item.error}")

    return results


async def main() -> int:
    parser = argparse.ArgumentParser(description="Concurrent websocket connection stability test")
    parser.add_argument("--uri", default="wss://127.0.0.1:18443/ws", help="WebSocket URI")
    parser.add_argument("--clients", type=int, default=30, help="Concurrent clients per wave")
    parser.add_argument("--waves", type=int, default=3, help="Number of waves")
    parser.add_argument("--timeout", type=float, default=5.0, help="Per-step timeout in seconds")
    parser.add_argument("--mode", choices=["rooms_json", "connect_only"], default="rooms_json", help="Test mode")
    parser.add_argument("--batch-size", type=int, default=0, help="Clients per batch (0 means all-at-once)")
    parser.add_argument("--batch-interval-ms", type=int, default=0, help="Interval between batches in milliseconds")
    args = parser.parse_args()

    total = 0
    failed = 0

    for wave_id in range(1, args.waves + 1):
        if args.batch_size and args.batch_size > 0:
            results = await run_wave_batched(
                wave_id,
                args.clients,
                args.uri,
                args.timeout,
                args.mode,
                args.batch_size,
                args.batch_interval_ms,
            )
        else:
            results = await run_wave(wave_id, args.clients, args.uri, args.timeout, args.mode)
        total += len(results)
        failed += sum(1 for r in results if not r.ok)

    print(f"[summary] total={total}, failed={failed}, success_rate={(total - failed) * 100.0 / max(total, 1):.2f}%")
    return 0 if failed == 0 else 1


if __name__ == "__main__":
    raise SystemExit(asyncio.run(main()))