FROM alpine:latest

#安装必要依赖
RUN echo "https://dl-cdn.alpinelinux.org/alpine/edge/testing" >> /etc/apk/repositories
RUN apk update
RUN apk add --no-cache python3 curl openssh wakeonlan cronie sshpass

# 将脚本复制到容器中
COPY wakeup.py /wakeup.py
# 使脚本可执行
RUN chmod +x /wakeup.py

# 复制配置文件
COPY config.ini /config.ini

# cron定时任务
ADD crontab /crontab
RUN /usr/bin/crontab /crontab

# 将 start.sh 脚本复制到容器中
COPY start.sh /start.sh
# 使脚本可执行
RUN chmod +x /start.sh

# 清理缓存
RUN apk cache clean

# 使用 start.sh 作为 CMD 指令
CMD ["/start.sh"]