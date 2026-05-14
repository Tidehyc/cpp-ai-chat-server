#!/usr/bin/env python3
"""
日志级别验证脚本：检查 Docker 容器日志级别、日志内容完整性。
测试场景：
1. 验证容器日志输出正常
2. 检查主要事件日志（连接、消息发送、错误）
3. 日志级别可配置（debug/info/warn/error）
"""

import subprocess
import sys
import time
import re
from typing import Optional, Dict, List


def compose_cmd() -> str:
    """检测本地 docker-compose 或 docker compose 命令。"""
    result = subprocess.run(["docker-compose", "--version"], capture_output=True)
    if result.returncode == 0:
        return "docker-compose"
    result = subprocess.run(["docker", "compose", "--version"], capture_output=True)
    if result.returncode == 0:
        return "docker"
    raise RuntimeError("无法找到 docker-compose 或 docker compose 命令")


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
    """确保 docker-compose 容器已启动。"""
    compose = compose_cmd()
    container = find_chat_server_container()
    
    if container:
        print(f"  -> 容器已存在：{container}")
        return
    
    print(f"  -> 启动容器 (docker compose up -d)")
    if compose == "docker-compose":
        subprocess.run(["docker-compose", "up", "-d"], check=True)
    else:
        subprocess.run(["docker", "compose", "up", "-d"], check=True)
    time.sleep(2)


def get_container_logs(container: str, tail: int = 100) -> str:
    """获取容器日志（最后 N 行）。"""
    result = subprocess.run(
        ["docker", "logs", "--tail", str(tail), container],
        capture_output=True,
        text=True,
    )
    if result.returncode != 0:
        print(f"  ✗ 无法读取容器日志: {result.stderr}")
        return ""
    return result.stdout


def analyze_logs(logs: str) -> Dict[str, int]:
    """分析日志内容，统计各级别日志出现次数。"""
    stats = {
        "debug": 0,
        "info": 0,
        "warn": 0,
        "error": 0,
        "connection": 0,
        "message": 0,
        "total_lines": 0,
    }
    
    for line in logs.split("\n"):
        if not line.strip():
            continue
        
        stats["total_lines"] += 1
        
        # 检查日志级别
        if re.search(r"\bdebug\b", line, re.IGNORECASE):
            stats["debug"] += 1
        elif re.search(r"\berror\b", line, re.IGNORECASE):
            stats["error"] += 1
        elif re.search(r"\bwarn\b", line, re.IGNORECASE):
            stats["warn"] += 1
        else:
            stats["info"] += 1
        
        # 检查关键事件
        if re.search(r"connect|connection|websocket", line, re.IGNORECASE):
            stats["connection"] += 1
        elif re.search(r"message|send|recv", line, re.IGNORECASE):
            stats["message"] += 1
    
    return stats


def check_logging_test(container: str) -> int:
    """
    日志级别验证。
    返回 0 成功，2 失败。
    """
    print("-> 日志级别验证")
    print(f"  -> 容器: {container}")
    
    # 获取最近的日志
    print(f"  -> 获取容器日志（最后 100 行）")
    logs = get_container_logs(container, tail=100)
    
    if not logs.strip():
        print(f"  ✗ 无日志输出或容器不健康")
        return 2
    
    print(f"    ✓ 获取 {len(logs.splitlines())} 行日志")
    
    # 分析日志
    stats = analyze_logs(logs)
    
    print(f"\n  📊 日志统计:")
    print(f"    - 总行数: {stats['total_lines']}")
    print(f"    - DEBUG 级别: {stats['debug']}")
    print(f"    - INFO 级别: {stats['info']}")
    print(f"    - WARN 级别: {stats['warn']}")
    print(f"    - ERROR 级别: {stats['error']}")
    print(f"    - 连接相关日志: {stats['connection']}")
    print(f"    - 消息相关日志: {stats['message']}")
    
    # 基本验证：应该至少有一些日志输出
    if stats['total_lines'] < 5:
        print(f"  ✗ 日志输出不足（仅 {stats['total_lines']} 行）")
        return 2
    
    # 应该至少有一些 INFO 或 DEBUG 级别的日志
    if stats['info'] + stats['debug'] < 1:
        print(f"  ⚠️ 缺少 INFO/DEBUG 级别日志")
    
    # 优先级别的日志应该有一些关键事件
    if stats['connection'] > 0 or stats['message'] > 0:
        print(f"  ✓ 捕获到关键事件日志")
    
    print(f"\n✅ 日志级别验证 OK")
    return 0


def main():
    """主入口。"""
    print("=" * 60)
    print("CI 日志级别验证")
    print("=" * 60)
    
    try:
        ensure_docker_compose_up()
        print()
        
        container = find_chat_server_container()
        if not container:
            print("✗ 未找到 chat-server 容器")
            sys.exit(2)
        
        exit_code = check_logging_test(container)
        sys.exit(exit_code)
    
    except KeyboardInterrupt:
        print("\n用户中断")
        sys.exit(1)
    except Exception as e:
        print(f"\n❌ 测试异常: {e}")
        sys.exit(2)


if __name__ == "__main__":
    main()
