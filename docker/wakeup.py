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
# 指定编码为UTF-8，并读取配置文件
config.read(config_path, encoding='utf-8')

# 巴法云私钥
client_id = config.get('bafa', 'client_id').strip('"')
# 主题值
topic = config.get('bafa', 'topic').strip('"')
ssh_ip = config.get('openssh', 'ip').strip('"')
ssh_user = config.get('openssh', 'user').strip('"')
ssh_password = config.get('openssh', 'password').strip('"')
# 局域网连接openssh服务器，进行关机操作
cmd_shutdown = 'sshpass -p %(password)s ssh -A -g -o StrictHostKeyChecking=no %(user)s@%(ip)s "shutdown -s -t 10"'%{"password": ssh_password, "user": ssh_user, "ip": ssh_ip}

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

# 网络唤醒
def wake_on_lan(mac):
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
					wake_on_lan(config.get('interface', 'mac').strip('"'))
				except:
					print("打开电脑失败")
			else:
				try:
					print("正在关闭电脑")
					if check_url():
						print(f"执行命令:{cmd_shutdown}")
						os.system(cmd_shutdown)
					else:
						print("目标PC未在线或SSH服务器未开启")
				except:
					time.sleep(2)
		except:
			time.sleep(2)
	else:
		print("conn err")
		connTCP()