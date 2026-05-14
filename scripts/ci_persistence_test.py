#!/usr/bin/env python3
"""CI: 验证消息写入到 Docker 卷中并在服务重启后持久化。

流程：
  1. 读取卷中 SQLite `messages` 表计数（PRE_COUNT）。
  2. 通过 WebSocket 注册/登录一个临时用户并发送一个探测消息。
  3. 读取写入后的计数（POST_WRITE_COUNT），重启服务并再次读取（POST_RESTART_COUNT）。
  4. 若计数一致且比 PRE_COUNT 增加则返回成功。

注意：脚本依赖 `docker` 与 `docker compose` 在运行环境中可用。
"""

import asyncio
import os
import random
import string
import subprocess
import sys
import time

import websockets


def run_shell(cmd: str) -> str:
    res = subprocess.run(cmd, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
    if res.returncode != 0:
        raise RuntimeError(f"cmd failed: {cmd}\nstdout:{res.stdout}\nstderr:{res.stderr}")
    return res.stdout.strip()


def compose_cmd() -> str:
    if subprocess.run("docker-compose version", shell=True, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL).returncode == 0:
        return "docker-compose"
    return "docker compose"


def find_chat_server_container() -> str:
    cmd = "docker ps -a --format '{{.Names}}' | grep -m1 'chat-server' || true"
    return run_shell(cmd)


def sqlite_count_from_volume() -> int:
    # use alpine + sqlite to read the DB file inside the named volume
    cmd = (
        "docker run --rm -v cpp_ai_chat_data:/data alpine sh -c 'apk add --no-cache sqlite >/dev/null 2>&1; "
        "sqlite3 /data/chat.db "
        "\"select count(*) from messages;\"'"
    )
    out = run_shell(cmd)
    try:
        return int(out.splitlines()[-1].strip())
    except Exception:
        return 0


async def ws_register_login_and_send(uri: str, message: str) -> None:
    user = "persist_" + "".join(random.choice(string.ascii_lowercase + string.digits) for _ in range(8))
    pwd = "pass123"
    async with websockets.connect(uri, subprotocols=["chat"], ping_interval=5, ping_timeout=5) as ws:
        await ws.recv()  # welcome
        await ws.send(f"/register {user} {pwd}")
        await ws.recv()
        await ws.send(f"/login {user} {pwd}")
        await ws.recv()
        # send a plain message (server side should record it)
        await ws.send(message)
        # try to read a system response or ack (non-blocking wait)
        try:
            await asyncio.wait_for(ws.recv(), timeout=3)
        except asyncio.TimeoutError:
            pass


def ensure_docker_compose_up():
    # Reuse an already existing container to avoid the local docker-compose v1 recreate bug.
    existing = find_chat_server_container()
    if existing:
        started = subprocess.run(
            f"docker ps --format '{{{{.Names}}}}' | grep -x '{existing}' || true",
            shell=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.DEVNULL,
            text=True,
        ).stdout.strip()
        if started:
            return
        run_shell(f"docker start {existing}")
        return
    run_shell(f"{compose_cmd()} up -d")


def restart_chat_server():
    container_name = find_chat_server_container() or "cpp-ai-chat-server"
    run_shell(f"docker restart {container_name}")
    time.sleep(2)


def main():
    uri = os.environ.get("PERSIST_WS_URI", "ws://127.0.0.1:8080")
    probe_msg = "persistence_probe_message"

    print("-> ensuring containers up (docker compose up -d)")
    ensure_docker_compose_up()

    print("-> reading PRE_COUNT from volume")
    pre = sqlite_count_from_volume()
    print(f"PRE_COUNT={pre}")

    print("-> registering/logging in and sending probe message")
    asyncio.run(ws_register_login_and_send(uri, probe_msg))

    post_write = sqlite_count_from_volume()
    print(f"POST_WRITE_COUNT={post_write}")

    print("-> restart chat-server to validate persistence")
    restart_chat_server()

    post_restart = sqlite_count_from_volume()
    print(f"POST_RESTART_COUNT={post_restart}")

    if post_write >= pre + 1 and post_restart == post_write:
        print("✅ persistence test OK")
        sys.exit(0)
    else:
        print("❌ persistence test FAILED")
        sys.exit(2)


if __name__ == "__main__":
    main()
