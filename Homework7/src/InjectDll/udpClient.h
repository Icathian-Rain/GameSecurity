#pragma once

#include <WinSock2.h>
#include <WS2tcpip.h>

class UdpClient
{
public:
    UdpClient();
    ~UdpClient();
    void initSocket();
    void sendMessage(const char* message);
private:
    SOCKET s;
    sockaddr_in addr;
};

