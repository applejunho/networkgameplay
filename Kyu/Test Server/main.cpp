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
static int g_currentTurnPlayerId = 0;

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

        printf("클라이언트 접속! slot=%d sock=%d playerId=%d\n", slot, (int)client_sock, join.playerId);

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

                // ★ 클라가 MY_TURN 플래그를 장난쳐도 서버는 무시
                g_playerStates[pkt.playerId].flags &= ~PLAYER_FLAG_MY_TURN;
            }
            BroadcastState();
        }
        else if (type == PKT_TYPE_FIRE && recvlen >= (int)sizeof(PKT_FIRE))
        {
            PKT_FIRE pkt{};
            memcpy(&pkt, buf, sizeof(PKT_FIRE));
            BroadcastPacket((char*)&pkt, sizeof(pkt), INVALID_SOCKET);
            printf("발사 currentTurn=%d\n", g_currentTurnPlayerId);
        }
        else if (type == PKT_TYPE_TERRAIN_DELTA && recvlen >= sizeof(PKT_TERRAIN_DELTA))
        {
            PKT_TERRAIN_DELTA pkt{};
            memcpy(&pkt, buf, sizeof(pkt));
            BroadcastPacket((char*)&pkt, sizeof(pkt), INVALID_SOCKET);
            printf("지형파괴 currentTurn=%d\n", g_currentTurnPlayerId);
        }
        else if (type == PKT_TYPE_TURN_END && recvlen >= (int)sizeof(PKT_TURN_END))
        {
            PKT_TURN_END pkt{};
            memcpy(&pkt, buf, sizeof(PKT_TURN_END));

            // ★ 지금 턴을 가진 플레이어만 턴을 끝낼 수 있게 안전장치
            if (pkt.playerId == g_currentTurnPlayerId)
            {
                // 2인용이면 0 <-> 1 토글 (MAX_PLAYER 3이면 조금 더 신경 써야 함)
				if (MAX_PLAYER == 2)
                    g_currentTurnPlayerId = 1 - g_currentTurnPlayerId;
                else
                g_currentTurnPlayerId = (g_currentTurnPlayerId + 1) % MAX_PLAYER;

                printf("턴 변경! currentTurn=%d\n", g_currentTurnPlayerId);

                // 새 턴 정보 브로드캐스트
                BroadcastState();
            }
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
        {
            pkt.players[i] = g_playerStates[i];

            // ★ 클라가 보내온 MY_TURN 비트는 무시하고, 서버가 직접 설정
            pkt.players[i].flags &= ~PLAYER_FLAG_MY_TURN;  // 턴 비트 제거
            if (i == g_currentTurnPlayerId)
                pkt.players[i].flags |= PLAYER_FLAG_MY_TURN; // 현재 턴인 애만 ON
        }
    }

    BroadcastPacket((char*)&pkt, sizeof(pkt));
}
