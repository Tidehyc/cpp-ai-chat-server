#!/usr/bin/env python3
"""
Concurrent connection stress test for chat server.
Tests multiple simultaneous WebSocket connections and command dispatch.
"""

import asyncio
import websockets
import json
import sys

async def run_client(client_id: int) -> bool:
    """Run a single client connection."""
    try:
        async with websockets.connect('ws://localhost:8080') as websocket:
            # Receive welcome message
            msg = await asyncio.wait_for(websocket.recv(), timeout=5)
            
            # Send rooms command
            await websocket.send('/rooms_json')
            
            # Receive response (should be JSON object with rooms)
            response = await asyncio.wait_for(websocket.recv(), timeout=5)
            
            # Verify response is JSON and contains rooms array
            try:
                data = json.loads(response)
                if not isinstance(data, dict) or "rooms" not in data:
                    return False
            except json.JSONDecodeError:
                return False
            
            return True
    except Exception as e:
        print(f"Client {client_id} error: {e}", file=sys.stderr)
        return False

async def wave(wave_num: int, client_count: int = 30) -> tuple:
    """Run a wave of concurrent connections."""
    tasks = [run_client(i) for i in range(client_count)]
    results = await asyncio.gather(*tasks)
    
    ok_count = sum(1 for r in results if r)
    print(f"[wave {wave_num}] ok={ok_count}/{client_count}")
    return ok_count, client_count

async def main():
    """Run 3 waves of concurrent connections."""
    total_ok = 0
    total_count = 0
    
    for wave_num in range(1, 4):
        ok, count = await wave(wave_num)
        total_ok += ok
        total_count += count
        await asyncio.sleep(0.5)  # Brief pause between waves
    
    success_rate = 100.0 * total_ok / total_count if total_count > 0 else 0.0
    print(f"[summary] total={total_count}, fail={total_count - total_ok}, success_rate={success_rate:.2f}%")
    
    # Exit with error if not 100% success
    return 0 if total_ok == total_count else 1

if __name__ == '__main__':
    exit_code = asyncio.run(main())
    sys.exit(exit_code)
