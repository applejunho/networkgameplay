#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <stdio.h>
#include "Packet.h"

#pragma comment(lib, "ws2_32.lib")

#define SERVERPORT 9000
#define MAX_CLIENT 2

SOCKET g_clientSock[MAX_CLIENT] = { INVALID_SOCKET, INVALID_SOCKET };

DWORD WINAPI ClientThread(LPVOID arg);
void BroadcastToAll(const char* buf, int len, SOCKET exceptSock = INVALID_SOCKET);

int main()
{
    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);

    SOCKET listen_sock = socket(AF_INET, SOCK_STREAM, 0);

    SOCKADDR_IN serveraddr;
    memset(&serveraddr, 0, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serveraddr.sin_port = htons(SERVERPORT);

    if (bind(listen_sock, (SOCKADDR*)&serveraddr, sizeof(serveraddr)) == SOCKET_ERROR) {
        printf("bind() error\n");
        return 0;
    }

    if (listen(listen_sock, SOMAXCONN) == SOCKET_ERROR) {
        printf("listen() error\n");
        return 0;
    }

    printf("=== Fortress Server Started (port %d) ===\n", SERVERPORT);

    while (true)
    {
        SOCKADDR_IN clientaddr;
        int addrlen = sizeof(clientaddr);

        SOCKET client_sock = accept(listen_sock, (SOCKADDR*)&clientaddr, &addrlen);
        if (client_sock == INVALID_SOCKET) {
            printf("accept() error\n");
            continue;
        }

        // 빈 슬롯 찾기
        int slot = -1;
        for (int i = 0; i < MAX_CLIENT; i++) {
            if (g_clientSock[i] == INVALID_SOCKET) {
                slot = i;
                break;
            }
        }

        if (slot == -1) {
            printf("더 이상 접속 불가 (MAX_CLIENT 초과)\n");
            closesocket(client_sock);
            continue;
        }

        g_clientSock[slot] = client_sock;
        printf("클라이언트 접속! slot=%d, sock=%d\n", slot, (int)client_sock);

        HANDLE hThread = CreateThread(
            NULL, 0, ClientThread, (LPVOID)client_sock, 0, NULL
        );
        CloseHandle(hThread);
    }

    closesocket(listen_sock);
    WSACleanup();
    return 0;
}

// 클라이언트 스레드
DWORD WINAPI ClientThread(LPVOID arg)
{
    SOCKET client_sock = (SOCKET)arg;
    char buf[512];

    while (true)
    {
        int recvlen = recv(client_sock, buf, sizeof(buf), 0);
        if (recvlen <= 0) {
            printf("클라이언트 종료 sock=%d\n", (int)client_sock);
            break;
        }

        BYTE type = (BYTE)buf[0];

        switch (type)
        {
        case PKT_FIRE:
        {
            // ★ 아주 중요: printf에서 p를 쓰기 전에 여기서 "선언"
            if (recvlen < (int)sizeof(PKT_FIRE)) {
                printf("PKT_FIRE size mismatch: %d (need %d)\n",
                    recvlen, (int)sizeof(PKT_FIRE));
                break;
            }

            PKT_FIRE* p = (PKT_FIRE*)buf;   // ← 이 줄이 있어야 함

            printf("\n[SERVER] PKT_FIRE recv\n");
            printf("  playerId : %d\n", p->playerId);
            printf("  startX   : %.1f\n", p->startX);
            printf("  startY   : %.1f\n", p->startY);
            printf("  angle    : %.1f\n", p->angle);
            printf("  power    : %.1f\n", p->power);
            printf("  mode     : %d\n", p->shoot_mode);

            // 나중에 다른 클라에게도 쏴주고 싶으면:
            // BroadcastToAll((char*)p, sizeof(PKT_FIRE), client_sock);
        }
        break;

        default:
            printf("[SERVER] Unknown packet type=%d, len=%d\n", type, recvlen);
            break;
        }
    }

    closesocket(client_sock);

    // g_clientSock 배열에서 제거
    for (int i = 0; i < MAX_CLIENT; i++) {
        if (g_clientSock[i] == client_sock) {
            g_clientSock[i] = INVALID_SOCKET;
            break;
        }
    }

    return 0;
}

