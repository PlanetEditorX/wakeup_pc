# -*- coding: utf-8
import socket
import threading
import time
import os
import configparser

# 获取脚本的绝对路径
script_path = os.path.abspath(__file__)

# 获取同级目录下的文件的绝对路径
config_path = os.path.join(os.path.dirname(script_path), 'config.ini')

config = configparser.ConfigParser()
config.read(config_path)

# 巴法云私钥
client_id = config.get('bafa', 'client_id').strip('"')
# 主题值
topic = config.get('bafa', 'topic').strip('"')
ssh_ip = config.get('openssh', 'ip').strip('"')
ssh_user = config.get('openssh', 'user').strip('"')
ssh_password = config.get('openssh', 'password').strip('"')
# 局域网网络唤醒
cmd1 = 'wakeonlan ' + config.get('interface', 'mac').strip('"')
# 发送关闭电脑off给巴法云，更新状态
cmd2 = '/usr/bin/curl -s "https://api.bemfa.com/api/device/v1/data/3/push/get/?uid=%(uid)s&topic=%(topic)s&msg=off" -w "\n"'%{"uid": client_id, "topic": topic}
# 局域网连接openssh服务器，进行关机操作
cmd3 = 'sshpass -p %(password)s ssh -A -g -o StrictHostKeyChecking=no %(user)s@%(ip)s "shutdown -s -t 10"'%{"password": ssh_password, "user": ssh_user, "ip": ssh_ip}

def connTCP():
    global tcp_client_socket
    # 创建socket
    tcp_client_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    # IP 和端口
    server_ip = 'bemfa.com'
    server_port = 8344
    try:
        # 连接服务器
        tcp_client_socket.connect((server_ip, server_port))
        #发送订阅指令
        substr = "cmd=1&uid=%(uid)s&topic=%(topic)s\r\n"%{"uid": client_id, "topic": topic}
        tcp_client_socket.send(substr.encode("utf-8"))
    except:
        time.sleep(2)
        connTCP()

#心跳
def Ping():
    # 发送心跳
    try:
        keeplive = 'ping\r\n'
        tcp_client_socket.send(keeplive.encode("utf-8"))
    except:
        time.sleep(2)
        connTCP()
    #开启定时，30秒发送一次心跳
    t = threading.Timer(30,Ping)
    t.start()


connTCP()
Ping()

# 检测目标PC是否在线
def check_url(url=ssh_ip, timeout=10):
	try:
		socket.setdefaulttimeout(3)
		socket.socket(socket.AF_INET, socket.SOCK_STREAM).connect((url, 22))
		return True
	except Exception as e:
		print(f"无法连接到 {url}:{22}，错误：{e}")
		return False

while True:
	# 接收服务器发送过来的数据
	recvData = tcp_client_socket.recv(1024)
	if len(recvData) != 0:
		try:
			res = recvData.decode('utf-8')
			print('recv:', res)
			sw = str(res.split('&')[3].split('=')[1]).strip()
			print('sw',sw)
			if str(sw) == str("on"):
				try:
					print("正在打开电脑")
					os.system(cmd1)
				except:
					print("打开电脑失败")
			else:
				try:
					print("正在关闭电脑")
					print(f"执行命令:{cmd2}")
					os.system(cmd2)
					if check_url():
						print(f"执行命令:{cmd3}")
						os.system(cmd3)
					else:
						print("目标PC未在线或SSH服务器未开启")
				except:
					time.sleep(2)
		except:
			time.sleep(2)
	else:
		print("conn err")
		connTCP()