#!/bin/sh

# 定时任务
/usr/bin/crontab /crontab

# 启动 cron 服务
/usr/sbin/crond

# 等待 cron 启动完成
sleep 1

# 使Python脚本可执行
chmod +x /wakeup.py

# 运行 Python 脚本
python3 /wakeup.py

# 清理缓存
rm -rf /var/cache/apk/* && apk cache clean