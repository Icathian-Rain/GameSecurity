#include "pch.h"
#include "udpClient.h"

UdpClient::UdpClient()
{
}

UdpClient::~UdpClient()
{
}

void UdpClient::initSocket()
{
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
    s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    addr.sin_family = AF_INET;
    addr.sin_port = htons(9999);
    InetPtonA(AF_INET, "127.0.0.1", &addr.sin_addr.s_addr);
}

void UdpClient::sendMessage(const char* message)
{
    sendto(s, message, strlen(message), 0, (sockaddr*)&addr, sizeof(addr));
}

