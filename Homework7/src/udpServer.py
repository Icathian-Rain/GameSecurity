# udp server
import socket

# create a socket object
serversocket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

ip = "127.0.0.1"
port = 9999

# bind to the port
serversocket.bind((ip, port))

# receive data from client
while True:
    data, addr = serversocket.recvfrom(1024)
    try : 
        data = int(data.decode('ascii'))
        print(hex(data))
    except :
        data = data.decode('ascii')
        print(data)
