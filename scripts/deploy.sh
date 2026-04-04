#!/bin/bash
# 一键部署脚本
# 用法: chmod +x deploy.sh && ./deploy.sh

set -e

echo "========================================="
echo "  C++ AI Chat Server - 一键部署脚本"
echo "========================================="
echo ""

# 检查 Docker
if ! command -v docker &> /dev/null; then
    echo "❌ Docker 未安装，正在安装..."
    curl -fsSL https://get.docker.com | sh
    sudo systemctl enable docker
    sudo systemctl start docker
    echo "✅ Docker 安装完成"
fi

# 检查 Docker Compose
if ! command -v docker-compose &> /dev/null; then
    echo "❌ Docker Compose 未安装，正在安装..."
    sudo curl -L "https://github.com/docker/compose/releases/latest/download/docker-compose-$(uname -s)-$(uname -m)" -o /usr/local/bin/docker-compose
    sudo chmod +x /usr/local/bin/docker-compose
    echo "✅ Docker Compose 安装完成"
fi

echo ""
echo "📦 配置环境变量..."
if [ ! -f .env ]; then
    cp .env.example .env
    echo "⚠️  请编辑 .env 文件，填入你的 AI_API_KEY"
    echo "   nano .env"
    read -p "按回车继续..."
fi

echo ""
echo "🔨 构建并启动服务..."
docker-compose up -d --build

echo ""
echo "✅ 部署完成！"
echo ""
echo "📊 服务状态："
docker-compose ps
echo ""
echo "📝 查看日志："
echo "   docker-compose logs -f"
echo ""
echo "🌐 访问地址："
echo "   前端: http://localhost:5173"
echo "   后端: ws://localhost:8080"
echo ""
echo "🔒 生产环境部署请参考文档："
echo "   docs/DEPLOYMENT.md"
