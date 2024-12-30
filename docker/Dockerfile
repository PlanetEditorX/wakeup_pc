FROM alpine:3.21.0

#安装必要依赖
RUN echo "https://dl-cdn.alpinelinux.org/alpine/edge/testing" >> /etc/apk/repositories && \
    apk update && \
    apk add --no-cache python3 curl openssh wakeonlan cronie sshpass

# 复制python脚本
COPY wakeup.py /wakeup.py
# 复制配置文件
COPY config.ini /config.ini
# 复制定时任务
COPY crontab /crontab
# 复制启动脚本
COPY start.sh /start.sh

# 使启动脚本可执行
RUN chmod +x /start.sh
# 使用 start.sh 作为 CMD 指令
CMD ["/start.sh"]