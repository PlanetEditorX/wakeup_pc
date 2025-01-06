#!/bin/sh

echo "===================================" >> ./log.txt
echo "Start Wakeup PC Running with C" >> ./log.txt
echo "===================================" >> ./log.txt
# 清理缓存
echo "Clear cache......" >> ./log.txt
rm -rf /var/cache/apk/*

# 创建日志文件
if [ ! -f "log.txt" ]; then
    echo "The log.txt does not exist and is being created......" >> ./log.txt
    touch log.txt
else
    echo "Find local log.txt" >> ./log.txt
fi

# 定时任务
echo "Set crontab......" >> log.txt
/usr/bin/crontab ./crontab

# 启动 cron 服务
echo "Start crond service......" >> log.txt
/usr/sbin/crond

# 等待 cron 启动完成
sleep 1

# 使wakeup可执行
echo "Wakeup adds executable permissions......" >> log.txt
chmod +x wakeup clear_logs.sh

# 运行
echo "Running wakeup......" >> log.txt
./wakeup