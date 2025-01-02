# 使用alpine作为基础镜像，并安装必要的编译工具和库
FROM alpine:3.18.6 AS builder
# 设置环境变量以确保 Python 输出实时打印，而不是被缓冲。
ENV PYTHONUNBUFFERED=1

# 安装必要依赖
RUN apk add python3 py3-pip binutils && \
    pip install pyinstaller

# 设置工作目录
WORKDIR /app

# 复制python脚本
COPY wakeup.py ./wakeup.py

# 复制配置文件
COPY config.ini ./config.ini

# 打包为二进制文件
RUN pyinstaller --onefile --add-data "config.ini:." wakeup.py

# 使用alpine作为最终的基础镜像
FROM alpine:3.18.6

# 从构建阶段复制静态编译的可执行文件到最终镜像
COPY --from=builder /app/dist /app/
COPY start.sh config.ini crontab /app/

# 设置工作目录
WORKDIR /app

# 安装必要依赖
RUN apk add  --no-cache openssh cronie sshpass && \
    chmod +x ./start.sh

# # 使用 start.sh 作为 CMD 指令
CMD ["./start.sh"]