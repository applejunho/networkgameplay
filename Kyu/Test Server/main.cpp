#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <winsock2.h>
#include <windows.h>
#include <stdio.h>
#include <mutex>
#include <cstring>

#include "Packet.h"

#pragma comment(lib, "ws2_32.lib")

#define SERVERPORT 9000
#define MAX_CLIENT MAX_PLAYER

struct ClientSlot
{
    SOCKET sock = INVALID_SOCKET;
    int    id = -1;
};

static ClientSlot g_clients[MAX_CLIENT];
static PlayerStateData g_playerStates[MAX_PLAYER] = {};
static std::mutex g_stateLock;

DWORD WINAPI ClientThread(LPVOID arg);
void BroadcastPacket(const char* buf, int len, SOCKET exceptSock = INVALID_SOCKET);
void BroadcastState();

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

    SOCKADDR_IN serveraddr = {};
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

        int slot = -1;
        for (int i = 0; i < MAX_CLIENT; ++i)
        {
            if (g_clients[i].sock == INVALID_SOCKET)
            {
                slot = i;
                break;
            }
        }

        if (slot == -1)
        {
            printf("서버 풀 (MAX_CLIENT=%d) 초과. 접속 거부\n", MAX_CLIENT);
            closesocket(client_sock);
            continue;
        }

        g_clients[slot].sock = client_sock;
        g_clients[slot].id = slot;

        PKT_JOIN join{};
        join.type = PKT_TYPE_JOIN;
        join.playerId = slot;
        send(client_sock, (char*)&join, sizeof(join), 0);

        printf("클라이언트 접속! slot=%d sock=%d\n", slot, (int)client_sock);

        HANDLE hThread = CreateThread(NULL, 0, ClientThread, (LPVOID)(intptr_t)slot, 0, NULL);
        if (hThread)
            CloseHandle(hThread);

        BroadcastState();
    }

    closesocket(listen_sock);
    WSACleanup();
    return 0;
}

DWORD WINAPI ClientThread(LPVOID arg)
{
    int slot = (int)(intptr_t)arg;
    SOCKET client_sock = g_clients[slot].sock;

    char buf[1024];

    while (true)
    {
        int recvlen = recv(client_sock, buf, sizeof(buf), 0);
        if (recvlen <= 0)
        {
            printf("클라이언트 종료 slot=%d\n", slot);
            break;
        }

        BYTE type = (BYTE)buf[0];

        if (type == PKT_TYPE_MOVE && recvlen >= (int)sizeof(PKT_MOVE))
        {
            PKT_MOVE pkt{};
            memcpy(&pkt, buf, sizeof(PKT_MOVE));
            if (pkt.playerId >= 0 && pkt.playerId < MAX_PLAYER)
            {
                std::lock_guard<std::mutex> lock(g_stateLock);
                g_playerStates[pkt.playerId] = pkt.state;
                g_playerStates[pkt.playerId].flags |= PLAYER_FLAG_VALID;
            }
            BroadcastState();
        }
        else if (type == PKT_TYPE_FIRE && recvlen >= (int)sizeof(PKT_FIRE))
        {
            PKT_FIRE pkt{};
            memcpy(&pkt, buf, sizeof(PKT_FIRE));
            BroadcastPacket((char*)&pkt, sizeof(pkt), INVALID_SOCKET);
        }
        else
        {
            printf("Unknown packet type=%d len=%d\n", type, recvlen);
        }
    }

    closesocket(client_sock);
    g_clients[slot].sock = INVALID_SOCKET;
    g_clients[slot].id = -1;

    {
        std::lock_guard<std::mutex> lock(g_stateLock);
        memset(&g_playerStates[slot], 0, sizeof(PlayerStateData));
    }

    BroadcastState();
    return 0;
}

void BroadcastPacket(const char* buf, int len, SOCKET exceptSock)
{
    for (int i = 0; i < MAX_CLIENT; ++i)
    {
        SOCKET s = g_clients[i].sock;
        if (s != INVALID_SOCKET && s != exceptSock)
        {
            send(s, buf, len, 0);
        }
    }
}

void BroadcastState()
{
    PKT_STATE pkt{};
    pkt.type = PKT_TYPE_STATE;
    pkt.playerCount = MAX_PLAYER;

    {
        std::lock_guard<std::mutex> lock(g_stateLock);
        for (int i = 0; i < MAX_PLAYER; ++i)
            pkt.players[i] = g_playerStates[i];
    }

    BroadcastPacket((char*)&pkt, sizeof(pkt));
}