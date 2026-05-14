#!/usr/bin/env python3
"""
监控 GitHub Actions 工作流，等待完成并更新 TEST_CHECKLIST.md
"""
import subprocess
import time
import json
import sys
from pathlib import Path

def get_run_status(workflow_name, run_number):
    """获取工作流运行状态"""
    cmd = [
        "gh", "run", "list", 
        f"--workflow={workflow_name}",
        "--limit", "1",
        "--json", "number,status,conclusion"
    ]
    result = subprocess.run(cmd, capture_output=True, text=True, cwd="/home/tide/workplace/cpp-ai-chat-server")
    if result.returncode != 0:
        print(f"❌ 获取工作流状态失败: {result.stderr}")
        return None
    
    try:
        data = json.loads(result.stdout)
        if data:
            return data[0]
    except json.JSONDecodeError:
        pass
    return None

def wait_for_workflow(workflow_name, run_number, max_wait_seconds=1800, interval=30):
    """等待工作流完成"""
    start_time = time.time()
    print(f"[{time.strftime('%H:%M:%S')}] 开始监控 {workflow_name} (Run #{run_number})...")
    
    while time.time() - start_time < max_wait_seconds:
        status_info = get_run_status(workflow_name, run_number)
        if not status_info:
            print(f"[{time.strftime('%H:%M:%S')}] 无法获取工作流状态，继续等待...")
            time.sleep(interval)
            continue
        
        current_status = status_info.get("status")
        conclusion = status_info.get("conclusion")
        
        print(f"[{time.strftime('%H:%M:%S')}] 状态: {current_status}, 结果: {conclusion}")
        
        if current_status == "completed":
            return status_info
        
        time.sleep(interval)
    
    print(f"❌ 工作流监控超时（{max_wait_seconds}s）")
    return None

def get_run_logs(run_number):
    """获取工作流日志"""
    cmd = ["gh", "run", "view", str(run_number), "--log"]
    result = subprocess.run(cmd, capture_output=True, text=True, cwd="/home/tide/workplace/cpp-ai-chat-server")
    if result.returncode == 0:
        return result.stdout
    return None

def extract_test_results(logs):
    """从日志中提取测试结果"""
    results = {
        "private_history": "❓",
        "timeout": "❓",
        "concurrent": "❓"
    }
    
    if not logs:
        return results
    
    if "✅" in logs and "private_history" in logs.lower():
        results["private_history"] = "✅"
    elif "❌" in logs and "private_history" in logs.lower():
        results["private_history"] = "❌"
    
    if "✅" in logs and "timeout" in logs.lower():
        results["timeout"] = "✅"
    elif "❌" in logs and "timeout" in logs.lower():
        results["timeout"] = "❌"
    
    if "✅" in logs and ("concurrent" in logs.lower() or "并发" in logs):
        results["concurrent"] = "✅"
    elif "❌" in logs and ("concurrent" in logs.lower() or "并发" in logs):
        results["concurrent"] = "❌"
    
    return results

def main():
    workflow_name = "advanced-test.yml"
    run_number = 2
    
    # 等待工作流完成
    result = wait_for_workflow(workflow_name, run_number)
    
    if not result:
        print("❌ 工作流监控失败")
        sys.exit(1)
    
    conclusion = result.get("conclusion")
    if conclusion == "success":
        print(f"\n✅ 工作流成功完成")
    else:
        print(f"\n⚠️  工作流完成，结果: {conclusion}")
    
    # 获取日志并提取结果
    print("\n📋 获取工作流日志...")
    logs = get_run_logs(run_number)
    test_results = {
        "private_history": "❓",
        "timeout": "❓",
        "concurrent": "❓"
    }
    if logs:
        print(f"📊 日志摘要（前 500 字符）:\n{logs[:500]}\n")
        test_results = extract_test_results(logs)
        print(f"🧪 测试结果:\n  - 私聊历史: {test_results['private_history']}\n  - 超时处理: {test_results['timeout']}\n  - 并发/幂等: {test_results['concurrent']}")
    else:
        print("⚠️  无法获取日志详情")
    
    # 更新 TEST_CHECKLIST.md
    checklist_path = Path("/home/tide/workplace/cpp-ai-chat-server/docs/TEST_CHECKLIST.md")
    if checklist_path.exists():
        content = checklist_path.read_text()
        
        # 更新私聊历史状态
        if "✅" in test_results["private_history"]:
            content = content.replace(
                "- [ ] 私聊历史回放",
                "- [x] 私聊历史回放"
            )
        
        # 更新超时处理状态
        if "✅" in test_results["timeout"]:
            content = content.replace(
                "- [ ] 超时处理",
                "- [x] 超时处理"
            )
        
        # 更新并发幂等状态
        if "✅" in test_results["concurrent"]:
            content = content.replace(
                "- [ ] 并发/幂等",
                "- [x] 并发/幂等"
            )
        
        checklist_path.write_text(content)
        print(f"\n✅ 已更新 {checklist_path}")
    
    print("\n✅ 监控完成")

if __name__ == "__main__":
    main()
