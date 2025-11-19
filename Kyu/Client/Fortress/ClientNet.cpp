#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include "NetCommon.h"
#include "Packet.h"
#include "ClientNet.h"

#pragma comment(lib, "ws2_32.lib")

SOCKET g_sock = INVALID_SOCKET;

bool InitNetwork(const char* serverIp, int port)
{
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
        return false;

    g_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (g_sock == INVALID_SOCKET)
        return false;

    SOCKADDR_IN serveraddr;
    memset(&serveraddr, 0, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = inet_addr(serverIp);
    serveraddr.sin_port = htons(port);

    if (connect(g_sock, (SOCKADDR*)&serveraddr, sizeof(serveraddr)) == SOCKET_ERROR) {
        closesocket(g_sock);
        g_sock = INVALID_SOCKET;
        return false;
    }

    return true;
}

void SendPacket(const char* buf, int len)
{
    if (g_sock == INVALID_SOCKET) return;
    send(g_sock, buf, len, 0);
}
