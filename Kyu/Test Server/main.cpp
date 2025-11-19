// main.cpp  (Test Server)

#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <winsock2.h>
#include <windows.h>
#include <stdio.h>
#include "Packet.h"

#pragma comment(lib, "ws2_32.lib")

#define SERVERPORT 9000
#define MAX_CLIENT 2

SOCKET g_clientSock[MAX_CLIENT] = { INVALID_SOCKET, INVALID_SOCKET };

// 미리 선언
DWORD WINAPI ClientThread(LPVOID arg);
void BroadcastToAll(const char* buf, int len, SOCKET exceptSock = INVALID_SOCKET);

int main()
{
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
    {
        printf("WSAStartup() fail\n");
        return 0;
    }

    SOCKET listen_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_sock == INVALID_SOCKET)
    {
        printf("socket() fail\n");
        WSACleanup();
        return 0;
    }

    SOCKADDR_IN serveraddr;
    memset(&serveraddr, 0, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serveraddr.sin_port = htons(SERVERPORT);

    if (bind(listen_sock, (SOCKADDR*)&serveraddr, sizeof(serveraddr)) == SOCKET_ERROR)
    {
        printf("bind() error\n");
        closesocket(listen_sock);
        WSACleanup();
        return 0;
    }

    if (listen(listen_sock, SOMAXCONN) == SOCKET_ERROR)
    {
        printf("listen() error\n");
        closesocket(listen_sock);
        WSACleanup();
        return 0;
    }

    printf("=== Fortress Server Started (port %d) ===\n", SERVERPORT);

    while (true)
    {
        SOCKADDR_IN clientaddr;
        int addrlen = sizeof(clientaddr);

        SOCKET client_sock = accept(listen_sock, (SOCKADDR*)&clientaddr, &addrlen);
        if (client_sock == INVALID_SOCKET)
        {
            printf("accept() error\n");
            continue;
        }

        // 빈 슬롯 찾기
        int slot = -1;
        for (int i = 0; i < MAX_CLIENT; i++)
        {
            if (g_clientSock[i] == INVALID_SOCKET)
            {
                slot = i;
                break;
            }
        }

        if (slot == -1)
        {
            printf("더 이상 접속 불가 (MAX_CLIENT 초과)\n");
            closesocket(client_sock);
            continue;
        }

        g_clientSock[slot] = client_sock;
        printf("클라이언트 접속! slot=%d, sock=%d\n", slot, (int)client_sock);

        HANDLE hThread = CreateThread(
            NULL, 0, ClientThread, (LPVOID)client_sock, 0, NULL);
        if (hThread)
            CloseHandle(hThread);
    }

    closesocket(listen_sock);
    WSACleanup();
    return 0;
}

// ----------------------------------------------------
// 클라이언트 스레드
// ----------------------------------------------------
DWORD WINAPI ClientThread(LPVOID arg)
{
    SOCKET client_sock = (SOCKET)arg;

    while (true)
    {
        FirePacket pkt{};
        int recvlen = recv(client_sock, (char*)&pkt, sizeof(pkt), 0);

        if (recvlen <= 0)
        {
            printf("클라이언트 종료 sock=%d\n", (int)client_sock);
            break;
        }

        // 패킷 타입 확인
        if (pkt.type == PKT_FIRE && recvlen == sizeof(FirePacket))
        {
            printf("\n[SERVER] PKT_FIRE recv\n");
            printf("  playerId : %d\n", pkt.playerId);
            printf("  startX   : %.1f\n", pkt.startX);
            printf("  startY   : %.1f\n", pkt.startY);
            printf("  angle    : %.1f\n", pkt.angle);
            printf("  power    : %.1f\n", pkt.power);
            printf("  mode     : %d\n", pkt.shoot_mode);

            // 나중에 다른 클라이언트에게도 전달하고 싶으면:
            // BroadcastToAll((char*)&pkt, sizeof(pkt), client_sock);
        }
        else
        {
            printf("[SERVER] Unknown packet. type=%d len=%d\n",
                pkt.type, recvlen);
        }
    }

    // 소켓 정리
    closesocket(client_sock);

    // g_clientSock 배열에서 제거
    for (int i = 0; i < MAX_CLIENT; i++)
    {
        if (g_clientSock[i] == client_sock)
        {
            g_clientSock[i] = INVALID_SOCKET;
            break;
        }
    }

    return 0;
}

// ----------------------------------------------------
// 전체 브로드캐스트 (지금은 안 써도 됨)
// ----------------------------------------------------
void BroadcastToAll(const char* buf, int len, SOCKET exceptSock)
{
    for (int i = 0; i < MAX_CLIENT; i++)
    {
        if (g_clientSock[i] != INVALID_SOCKET &&
            g_clientSock[i] != exceptSock)
        {
            send(g_clientSock[i], buf, len, 0);
        }
    }
}
