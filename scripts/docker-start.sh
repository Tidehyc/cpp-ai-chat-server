#!/bin/bash
# C++ AI Chat Server - Docker 启动脚本

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# 检查 .env 文件
if [ ! -f .env ]; then
    echo -e "${YELLOW}警告：.env 文件不存在，使用默认配置${NC}"
    echo "请复制 .env.example 为 .env 并配置 AI_API_KEY"
    echo ""
fi

# 检查 Docker
if ! command -v docker &> /dev/null; then
    echo -e "${RED}错误：Docker 未安装${NC}"
    exit 1
fi

if ! command -v docker-compose &> /dev/null; then
    echo -e "${RED}错误：docker-compose 未安装${NC}"
    exit 1
fi

# 显示菜单
echo "========================================"
echo "  C++ AI Chat Server - Docker 管理"
echo "========================================"
echo ""
echo "请选择操作："
echo "  1. 启动服务器"
echo "  2. 停止服务器"
echo "  3. 重启服务器"
echo "  4. 查看日志"
echo "  5. 进入容器"
echo "  6. 清理资源"
echo "  7. 重新构建"
echo ""
read -p "请输入选项 (1-7): " choice

case $choice in
    1)
        echo -e "${GREEN}启动服务器...${NC}"
        docker-compose up -d
        echo -e "${GREEN}服务器已启动！${NC}"
        echo "访问地址：http://localhost:8080"
        ;;
    2)
        echo -e "${YELLOW}停止服务器...${NC}"
        docker-compose down
        echo -e "${GREEN}服务器已停止${NC}"
        ;;
    3)
        echo -e "${YELLOW}重启服务器...${NC}"
        docker-compose restart
        echo -e "${GREEN}服务器已重启${NC}"
        ;;
    4)
        echo -e "${GREEN}查看日志...${NC}"
        docker-compose logs -f chat-server
        ;;
    5)
        echo -e "${GREEN}进入容器...${NC}"
        docker-compose exec chat-server /bin/bash
        ;;
    6)
        echo -e "${RED}清理资源（包括数据卷）...${NC}"
        read -p "确定要清理所有资源吗？(y/N): " confirm
        if [ "$confirm" = "y" ]; then
            docker-compose down -v
            echo -e "${GREEN}资源已清理${NC}"
        else
            echo "已取消"
        fi
        ;;
    7)
        echo -e "${YELLOW}重新构建...${NC}"
        docker-compose build --no-cache
        echo -e "${GREEN}构建完成${NC}"
        ;;
    *)
        echo "无效选项"
        exit 1
        ;;
esac

echo ""
echo "========================================"
