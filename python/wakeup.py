# -*- coding: utf-8
import socket
import threading
import time
import os
import configparser
import sys
from pathlib import Path

# 打开一个文件用于写入
f = open('log.txt', 'w', buffering=1)
# 将标准输出重定向到文件
sys.stdout = f

# 重读配置次数
read_config_time = 10
config = configparser.ConfigParser()
# 读取配置文件
def read_config():
	print('Read config.ini......')
	# 构建config.ini文件的路径
	inside_config_path = Path(__file__).parent / 'config.ini'
	external_config_path = Path(sys.executable).parent / 'config.ini'
	# 外部文件存在读取外部文件
	if external_config_path.exists():
		print(f'Read from {external_config_path}.....')
		config_path = external_config_path
	else:
		print(f'Read from {inside_config_path}.....')
		config_path = inside_config_path
	config.read(config_path)
	if (Path(config_path).exists() and config.get('bafa', 'client_id')):
		print("Read configuration")
	else:
		print("ERROR: Configuration not read......")
		print("Please waiting......")
		time.sleep(10)
		global read_config_time
		if read_config_time:
			read_config_time -= 1
			read_config()

read_config()

try:
	# 巴法云私钥
	client_id = config.get('bafa', 'client_id').strip('"')
	if (not client_id):
		raise ValueError
	# 主题值
	topic = config.get('bafa', 'topic').strip('"')
	ssh_ip = config.get('openssh', 'ip').strip('"')
	ssh_user = config.get('openssh', 'user').strip('"')
	ssh_password = config.get('openssh', 'password').strip('"')
	# 局域网连接openssh服务器，进行关机操作
	cmd_shutdown = 'sshpass -p %(password)s ssh -A -g -o StrictHostKeyChecking=no %(user)s@%(ip)s "shutdown -s -t 10"'%{"password": ssh_password, "user": ssh_user, "ip": ssh_ip}
	print(f"cmd_shutdown={cmd_shutdown}")
except:
	time.sleep(2)
	print("ERROR: No valid configuration was read!!!\nPlease check config.ini!\nProgram exit")
	sys.exit()  # 退出进程

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
		print("Heartbeat sent")
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

# 网络唤醒
def wake_on_lan(mac):
	try:
		print(f"正在向局域网发送MAC地址为{mac}的唤醒魔法包....")
		# 将MAC地址中的短横线去掉并转换为二进制格式
		mac = mac.lower().replace(":", "").replace("-", "")
		if len(mac) != 12:
			raise ValueError("格式错误的 MAC 地址")
		# 创建魔法包
		magic_packet = bytes.fromhex('FF' * 6 + mac * 16)
		# 发送魔法包到广播地址
		with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as sock:
			sock.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)  # 设置为广播
			sock.sendto(magic_packet, ('<broadcast>', 9))  # 发送到端口9的广播地址
			print(f"唤醒魔法包发送成功")
	except Exception as e:
		print(f"网络唤醒失败！{e}")

while True:
	# 接收服务器发送过来的数据
	recvData = tcp_client_socket.recv(1024)
	if len(recvData) != 0:
		try:
			res = recvData.decode('utf-8').replace("\r\n", "")
			print(f'recv: {res}')
			if res == 'cmd=1&res=1':
				print('Subscription topic successful.')
			elif res == 'cmd=0&res=1':
				print('Heartbeat sent successful.')
			else:
				sw = str(res.split('&')[3].split('=')[1]).strip()
				print('Received topic publishing data.')
				if sw == "on":
					try:
						print("正在打开电脑...")
						wake_on_lan(config.get('interface', 'mac').strip('"'))
					except:
						print("电脑开机指令发送失败!")
				elif sw == "off":
					try:
						print("正在关闭电脑")
						if check_url():
							print(f"执行命令:{cmd_shutdown}")
							os.system(cmd_shutdown)
						else:
							print("目标PC未在线或SSH服务器未开启")
					except:
						time.sleep(2)
				else:
					print("获取的指令不是 'on' 或 'off'")
		except:
			time.sleep(2)
	else:
		print("conn err")
		connTCP()