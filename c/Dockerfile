# 使用alpine作为基础镜像，并安装必要的编译工具和库
FROM alpine:3.18.6 AS builder

# 安装musl-dev和libc-dev，它们包含了C标准库的头文件和静态库
RUN apk add --no-cache musl-dev libc-dev gcc

# 设置工作目录
WORKDIR /app

# 复制源代码和配置文件到工作目录
COPY wakeup.c config.ini /app/

# 静态编译C程序，确保使用静态链接
RUN gcc -static -o wakeup wakeup.c

# 使用alpine作为最终的基础镜像
FROM alpine:3.18.6

# 从构建阶段复制静态编译的可执行文件到最终镜像
COPY --from=builder /app/wakeup /app/
COPY start.sh clear_logs.sh config.ini crontab /app/
# 设置工作目录
WORKDIR /app
RUN apk add --no-cache sshpass openssh && \
    chmod +x start.sh

# 设置容器启动时执行的命令
CMD ["start.sh"]
