#!/bin/sh

# 日志路径
LOG_FILE="/app/log.txt"
# 日志期望最大大小100kb
SIZE=102400

# 检查日志文件大小
if [ "$(stat -c %s "$LOG_FILE")" -gt "$SIZE" ]; then
    # 清空日志文件
    echo "This is clear by Script at $(date)" > "$LOG_FILE"
fi