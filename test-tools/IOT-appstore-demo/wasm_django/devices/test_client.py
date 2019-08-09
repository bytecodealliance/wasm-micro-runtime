#!/usr/bin/python3
# 文件名：client.py

# 导入 socket、sys 模块
import socket
import sys

# 创建 socket 对象
s = socket.socket(socket.AF_INET, socket.SOCK_STREAM) 

# 获取本地主机名
host = '127.0.0.1'

# 设置端口号
port = 8889

# 连接服务，指定主机和端口
s.connect((host, port))

# s.send(bytes("install:127.0.0.1:33600:timer:/home/xujun/timer.wasm", encoding = 'utf8'))

s.send(bytes("query:all", encoding = 'utf8'))

# 接收小于 1024 字节的数据
msg = s.recv(1024)

s.close()

print (msg.decode('utf-8'))
print (msg.decode('utf-8').split(","))

dic = eval(msg.decode('utf-8').split("*")[0])
print(dic)

