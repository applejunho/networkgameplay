#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <winsock2.h>
#include <windows.h>
#include <cstdio>   // printf
#include <cstring>  // memcpy

#include "Packet.h"
#include "NetCommon.h"
#include "Player.h"
#include "ClientNet.h"


#pragma comment(lib, "ws2_32.lib")

SOCKET g_sock = INVALID_SOCKET;
extern Player g_players[MAX_PLAYER];
extern int myPlayerId;
extern int g_playerCount;

DWORD WINAPI RecvThread(LPVOID arg)
{
    char buf[512];

    while (true)
    {
        int recvlen = recv(g_sock, buf, sizeof(buf), 0);
        if (recvlen <= 0)
            break;

        if (recvlen < 1)
            continue;

        BYTE type = (BYTE)buf[0];

        switch (type)
        {
        case PKT_JOIN:
        {
            // 패킷 크기 확인
            if (recvlen < (int)sizeof(PKT_JOIN))
                break;

            PKT_JOIN pkt{};
            memcpy(&pkt, buf, sizeof(PKT_JOIN));

            myPlayerId = pkt.playerId;
            g_isConnected = true;

            printf("서버로부터 내 ID = %d\n", myPlayerId);
        }
        break;

        case PKT_STATE:
        {
            if (recvlen < (int)sizeof(PKT_STATE))
                break;

            PKT_STATE pkt{};
            memcpy(&pkt, buf, sizeof(PKT_STATE));

            g_playerCount = pkt.playerCount;

            for (int i = 0; i < pkt.playerCount && i < MAX_PLAYER; i++)
            {
                g_players[i].left = (int)pkt.x[i];
                g_players[i].top = (int)pkt.y[i];
                g_players[i].angle = pkt.angle[i];
                g_players[i].HP = (int)pkt.hp[i];
            }
        }
        break;

        default:
            // 필요하면 디버그 출력
            // printf("Unknown packet type: %d, len=%d\n", type, recvlen);
            break;
        }
    }

    return 0;
}


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

    // ⭐⭐⭐ 여기 추가해야 함! RecvThread 실행 ⭐⭐⭐
    HANDLE hThread = CreateThread(NULL, 0, RecvThread, NULL, 0, NULL);
    CloseHandle(hThread);

    return true;
}


void SendPacket(const char* buf, int len)
{
    if (g_sock == INVALID_SOCKET) return;
    send(g_sock, buf, len, 0);
}

Player g_players[MAX_PLAYER];    // 모든 플레이어 (내 것 + 다른 플레이어)
int    myPlayerId = -1;          // 서버가 정해주는 내 ID
int    g_playerCount = MAX_PLAYER;

double g_ballX[MAX_PLAYER];
double g_ballY[MAX_PLAYER];

bool g_isConnected = false;

void CloseNetwork()
{
    if (g_sock != INVALID_SOCKET)
    {
        closesocket(g_sock);
        g_sock = INVALID_SOCKET;
    }
    WSACleanup();
}
