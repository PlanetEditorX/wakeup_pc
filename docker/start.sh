#!/bin/sh

# 启动 cron 服务
/usr/sbin/crond

# 等待 cron 启动完成
sleep 1

# 运行 Python 脚本
python3 /wakeup.py
