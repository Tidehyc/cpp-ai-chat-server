#!/usr/bin/env python3
"""
PR 状态监测脚本：定期检查 PR #16 的 review 状态，待 approve 后自动合并。
"""

import subprocess
import json
import time
import sys
from datetime import datetime


def get_pr_status(pr_number: int = 16) -> dict:
    """获取 PR 的审查状态。"""
    try:
        result = subprocess.run(
            ["gh", "pr", "view", str(pr_number), 
             "--json", "state,reviewDecision,mergeable,reviews"],
            capture_output=True,
            text=True,
            check=True,
        )
        return json.loads(result.stdout)
    except Exception as e:
        print(f"❌ 无法获取 PR 状态: {e}")
        return {}


def approve_and_merge_pr(pr_number: int = 16) -> bool:
    """自行 approve 并合并 PR（适用于个人项目）。"""
    try:
        # 尝试合并 PR（可能需要 approve）
        print(f"  -> 合并 PR #{pr_number}...")
        result = subprocess.run(
            ["gh", "pr", "merge", str(pr_number), "--auto", "--squash"],
            capture_output=True,
            text=True,
        )
        
        if result.returncode == 0:
            print(f"  ✅ PR #{pr_number} 已合并")
            return True
        elif "already merged" in result.stderr:
            print(f"  ℹ️  PR #{pr_number} 已合并")
            return True
        else:
            print(f"  ⚠️  合并失败: {result.stderr}")
            return False
    except Exception as e:
        print(f"  ❌ 异常: {e}")
        return False


def monitor_pr(pr_number: int = 16, max_wait_minutes: int = 60, check_interval: int = 30):
    """
    监测 PR 状态：
    - 每 check_interval 秒检查一次
    - 最多等待 max_wait_minutes 分钟
    - 当 approve 时自动合并
    """
    print("=" * 60)
    print(f"PR #{pr_number} 状态监测")
    print("=" * 60)
    
    start_time = time.time()
    max_wait_seconds = max_wait_minutes * 60
    
    while True:
        elapsed = time.time() - start_time
        if elapsed > max_wait_seconds:
            print(f"\n⏱️  已等待 {max_wait_minutes} 分钟，超时。")
            print("   可手动检查: gh pr view 16")
            return False
        
        status = get_pr_status(pr_number)
        if not status:
            print(f"[{datetime.now().strftime('%H:%M:%S')}] ⏳ 无法获取状态，{check_interval} 秒后重试...")
            time.sleep(check_interval)
            continue
        
        state = status.get("state", "")
        review_decision = status.get("reviewDecision", "")
        mergeable = status.get("mergeable", "")
        reviews = status.get("reviews", [])
        
        print(f"\n[{datetime.now().strftime('%H:%M:%S')}] 状态检查")
        print(f"  - 状态: {state}")
        print(f"  - 审查决定: {review_decision or '(无)'}")
        print(f"  - 可合并: {mergeable}")
        print(f"  - 评论数: {len(reviews)}")
        
        # 检查是否 approve
        if review_decision == "APPROVED" or mergeable == "MERGEABLE":
            print(f"\n✅ PR #{pr_number} 已 approve（或所有检查通过），准备合并...")
            if approve_and_merge_pr(pr_number):
                print(f"\n🎉 PR #{pr_number} 合并成功！")
                print(f"   可以继续后续工作，执行: git pull origin main")
                return True
            else:
                print(f"\n⚠️  PR 合并失败，可手动执行: gh pr merge {pr_number}")
                return False
        
        print(f"   ⏳ 等待 review... {check_interval} 秒后重试")
        print(f"   (已等待 {int(elapsed)} 秒 / 最多 {max_wait_seconds} 秒)")
        time.sleep(check_interval)


if __name__ == "__main__":
    import argparse
    
    parser = argparse.ArgumentParser()
    parser.add_argument("--pr", type=int, default=16, help="PR 编号")
    parser.add_argument("--wait", type=int, default=60, help="最多等待时间（分钟）")
    parser.add_argument("--interval", type=int, default=30, help="检查间隔（秒）")
    parser.add_argument("--no-merge", action="store_true", help="只监测不合并")
    args = parser.parse_args()
    
    try:
        if args.no_merge:
            # 仅监测模式
            while True:
                status = get_pr_status(args.pr)
                print(f"[{datetime.now().strftime('%H:%M:%S')}] {status}")
                time.sleep(args.interval)
        else:
            # 监测 + 合并模式
            success = monitor_pr(
                pr_number=args.pr,
                max_wait_minutes=args.wait,
                check_interval=args.interval,
            )
            sys.exit(0 if success else 1)
    except KeyboardInterrupt:
        print("\n\n👤 用户中断监测")
        sys.exit(0)
