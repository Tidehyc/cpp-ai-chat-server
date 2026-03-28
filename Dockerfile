# C++ AI Chat Server - Dockerfile
# 版本：v0.5.3

# ========== 构建阶段 ==========
FROM ubuntu:22.04 AS builder

# 设置工作目录
WORKDIR /build

# 安装构建依赖
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    libboost-all-dev \
    libsqlite3-dev \
    libssl-dev \
    libcurl4-openssl-dev \
    && rm -rf /var/lib/apt/lists/*

# 复制源代码
COPY . .

# 创建构建目录并编译
RUN mkdir -p build && \
    cd build && \
    cmake .. && \
    make -j$(nproc)

# ========== 运行阶段 ==========
FROM ubuntu:22.04

# 设置工作目录
WORKDIR /app

# 安装运行时依赖
RUN apt-get update && apt-get install -y \
    libboost-system1.74.0 \
    libsqlite3-0 \
    libssl3 \
    libcurl4 \
    && rm -rf /var/lib/apt/lists/*

# 从构建阶段复制编译好的文件
COPY --from=builder /build/build/chat_server /app/
COPY --from=builder /build/build/ws_client /app/

# 创建数据目录
RUN mkdir -p /app/data

# 设置环境变量
ENV AI_API_KEY=""
ENV AI_API_URL="https://api.scnet.cn/api/llm/v1/chat/completions"
ENV AI_MODEL="MiniMax-M2.5"
ENV SERVER_PORT=8080

# 暴露端口
EXPOSE 8080

# 健康检查
HEALTHCHECK --interval=30s --timeout=10s --start-period=5s --retries=3 \
    CMD pgrep -x chat_server || exit 1

# 启动服务器
CMD ["./chat_server"]
