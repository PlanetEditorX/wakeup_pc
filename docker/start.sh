#!/bin/sh

# 清理缓存
rm -rf /var/cache/apk/* && apk cache clean

# 创建日志文件
touch log.txt

# 定时任务
/usr/bin/crontab ./crontab

# 启动 cron 服务
/usr/sbin/crond

# 等待 cron 启动完成
sleep 1

# 使wakeup可执行
chmod +x ./wakeup

# 运行
./wakeup
