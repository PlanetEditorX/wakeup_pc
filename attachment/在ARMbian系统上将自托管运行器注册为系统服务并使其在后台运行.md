# 要在ARMbian系统上将自托管运行器注册为系统服务并使其在后台运行

1.**创建服务文件**： 创建一个新的systemd服务文件来管理你的自托管运行器。使用文本编辑器创建一个新文件，例如`/etc/systemd/system/actions-runner.service`，并添加以下内容：

```ini
[Unit]
Description=GitHub Actions Runner
After=network.target

[Service]
User=actions-runner
Group=actions-runner
WorkingDirectory=/home/actions-runner
ExecStart=/home/actions-runner/run.sh
Restart=always

[Install]
WantedBy=multi-user.target
```

2.**启用并启动服务**： 使用以下命令启用并启动你的服务：

```shellscript
sudo systemctl daemon-reload
sudo systemctl enable actions-runner.service
sudo systemctl start actions-runner.service
```

3.**检查服务状态**：

```shellscript
sudo systemctl status actions-runner.service
```

4.**配置rc.local**： 如果你想要在`/etc/rc.local`中添加自启动脚本，你需要确保`rc.local`服务被启用。编辑`/lib/systemd/system/rc-local.service`文件，在文件末尾添加以下内容：

```ini
[Install]
WantedBy=multi-user.target
Alias=rc-local.service
```

然后启用并启动`rc-local`服务：

```shellscript
sudo systemctl enable rc-local
sudo systemctl start rc-local.service
```

接着，在`/etc/rc.local`中添加你的自托管运行器启动命令，确保这些命令在`exit 0`之前：

```shellscript
#!/bin/bash -e
echo "Starting GitHub Actions Runner..."
/home/actions-runner/run.sh
```

赋予`/etc/rc.local`执行权限：

```shellscript
sudo chmod +x /etc/rc.local
```

