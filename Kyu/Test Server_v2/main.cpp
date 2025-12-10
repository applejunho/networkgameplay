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
#define SERVER_TICK 33   // ✅ 30 FPS 동기화

struct ClientSlot
{
    SOCKET sock = INVALID_SOCKET;
    int    id = -1;
};

static ClientSlot g_clients[MAX_CLIENT];
static PlayerStateData g_playerStates[MAX_PLAYER] = {};
static std::mutex g_stateLock;

DWORD WINAPI ClientThread(LPVOID arg);
DWORD WINAPI ServerTickThread(LPVOID arg);

void BroadcastPacket(const char* buf, int len, SOCKET exceptSock = INVALID_SOCKET);
void BroadcastState();

int main()
{
    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);

    SOCKET listen_sock = socket(AF_INET, SOCK_STREAM, 0);

    SOCKADDR_IN serveraddr{};
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serveraddr.sin_port = htons(SERVERPORT);

    bind(listen_sock, (SOCKADDR*)&serveraddr, sizeof(serveraddr));
    listen(listen_sock, SOMAXCONN);

    printf("=== Fortress Tick Server Started (%d FPS) ===\n", 1000 / SERVER_TICK);

    // ✅ Tick 스레드 시작
    CreateThread(NULL, 0, ServerTickThread, NULL, 0, NULL);

    while (true)
    {
        SOCKADDR_IN clientaddr;
        int addrlen = sizeof(clientaddr);
        SOCKET client_sock = accept(listen_sock, (SOCKADDR*)&clientaddr, &addrlen);

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
            closesocket(client_sock);
            continue;
        }

        g_clients[slot].sock = client_sock;
        g_clients[slot].id = slot;

        PKT_JOIN join{};
        join.type = PKT_TYPE_JOIN;
        join.playerId = slot;
        send(client_sock, (char*)&join, sizeof(join), 0);

        printf("클라이언트 접속 slot=%d\n", slot);

        HANDLE hThread = CreateThread(NULL, 0, ClientThread, (LPVOID)(intptr_t)slot, 0, NULL);
        CloseHandle(hThread);
    }

    return 0;
}

// ✅ 30FPS 고정 동기화 Thread
DWORD WINAPI ServerTickThread(LPVOID)
{
    while (true)
    {
        BroadcastState();
        Sleep(SERVER_TICK);
    }
}

// ✅ 클라이언트 입력 처리 Thread
DWORD WINAPI ClientThread(LPVOID arg)
{
    int slot = (int)(intptr_t)arg;
    SOCKET client_sock = g_clients[slot].sock;

    char buf[1024];

    while (true)
    {
        int recvlen = recv(client_sock, buf, sizeof(buf), 0);
        if (recvlen <= 0) break;

        BYTE type = buf[0];

        if (type == PKT_TYPE_MOVE && recvlen >= sizeof(PKT_MOVE))
        {
            PKT_MOVE pkt{};
            memcpy(&pkt, buf, sizeof(pkt));

            std::lock_guard<std::mutex> lock(g_stateLock);
            g_playerStates[slot] = pkt.state;
            g_playerStates[slot].flags |= PLAYER_FLAG_VALID;
        }
        else if (type == PKT_TYPE_FIRE && recvlen >= sizeof(PKT_FIRE))
        {
            PKT_FIRE pkt{};
            memcpy(&pkt, buf, sizeof(pkt));
            pkt.playerId = slot;

            BroadcastPacket((char*)&pkt, sizeof(pkt));
        }
        else if (type == PKT_TYPE_TERRAIN_DELTA)
        {
            BroadcastPacket(buf, recvlen);
        }
    }

    closesocket(client_sock);
    g_clients[slot].sock = INVALID_SOCKET;

    std::lock_guard<std::mutex> lock(g_stateLock);
    memset(&g_playerStates[slot], 0, sizeof(PlayerStateData));

    return 0;
}

// ✅ 패킷 브로드캐스트
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

// ✅ 상태 동기화 전용 패킷
void BroadcastState()
{
    PKT_STATE pkt{};
    pkt.type = PKT_TYPE_STATE;
    pkt.playerCount = MAX_PLAYER;

    {
        std::lock_guard<std::mutex> lock(g_stateLock);
        for (int i = 0; i < MAX_PLAYER; ++i)
        {
            pkt.players[i] = g_playerStates[i];
            pkt.players[i].flags &= ~PLAYER_FLAG_MY_TURN;
        }
    }

    BroadcastPacket((char*)&pkt, sizeof(pkt));
}
