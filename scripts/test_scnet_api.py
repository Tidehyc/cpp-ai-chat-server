#!/usr/bin/env python3
"""测试超算互联网 API，查看支持的模型"""

import requests
import json

API_KEY = "sk-MTY4LTIxOTI1NzkwODE0LTE3NzQ1OTY0NTU1NDU="
BASE_URL = "https://api.scnet.cn/api/llm/v1"

print("=" * 50)
print("测试超算互联网 API")
print("=" * 50)

# 测试 1: 获取模型列表
print("\n1. 获取模型列表...")
try:
    headers = {"Authorization": f"Bearer {API_KEY}"}
    response = requests.get(f"{BASE_URL}/models", headers=headers, timeout=10)
    print(f"状态码：{response.status_code}")
    print(f"响应：{json.dumps(response.json(), indent=2, ensure_ascii=False)}")
except Exception as e:
    print(f"错误：{e}")

# 测试 2: 尝试不同模型
print("\n2. 测试不同模型...")
models_to_try = [
    "deepseek-chat",
    "deepseek-r1",
    "deepseek-v3", 
    "qwen-2.5-72b-instruct",
    "qwen3",
    "minimax-m2.5",
    "MiniMax-M2.5",
]

for model in models_to_try:
    print(f"\n  测试模型：{model}")
    try:
        headers = {
            "Authorization": f"Bearer {API_KEY}",
            "Content-Type": "application/json"
        }
        data = {
            "model": model,
            "messages": [{"role": "user", "content": "Hi"}],
            "max_tokens": 10
        }
        response = requests.post(f"{BASE_URL}/chat/completions", headers=headers, json=data, timeout=15)
        print(f"    状态码：{response.status_code}")
        if response.status_code == 200:
            result = response.json()
            if "choices" in result:
                print(f"    ✅ 成功！回复：{result['choices'][0]['message']['content'][:50]}")
                break
        else:
            error = response.json()
            print(f"    错误：{error.get('error', {}).get('message', 'Unknown')}")
    except Exception as e:
        print(f"    错误：{e}")

print("\n" + "=" * 50)
