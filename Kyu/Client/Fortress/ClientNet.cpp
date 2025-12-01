#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <cstdio>
#include <cstring>

#include "Packet.h"
#include "NetCommon.h"
#include "Player.h"
#include "Game.h"
#include "ClientNet.h"

#pragma comment(lib, "ws2_32.lib")

SOCKET g_sock = INVALID_SOCKET;
static HANDLE g_recvThread = NULL;
static bool   g_isConnected = false;
int    g_myPlayerId = -1;

extern Fire A;
extern Fire B;
extern bool player1_left;
extern bool player2_left;
extern bool p1isMoving;
extern bool p2isMoving;
extern bool player_1turn;
extern bool player_2turn;
extern bool isFired;
extern int  player1TankNumber;
extern int  player2TankNumber;


static Fire* GetPlayerById(int playerId)
{
    if (playerId == 0) return &A;
    if (playerId == 1) return &B;
    return nullptr;
}

static void ApplyStatePacket(const struct PKT_STATE& pkt)
{
    bool firingAnim = false;
    for (int i = 0; i < MAX_PLAYER; ++i)
    {
        const PlayerStateData& state = pkt.players[i];
        if ((state.flags & PLAYER_FLAG_VALID) == 0)
            continue;

        Fire* player = GetPlayerById(i);
        if (!player)
            continue;

        player->left = state.left;
        player->top = state.top;
        player->angle = state.angle;
        player->HP = state.hp;
        player->power = state.power;
        player->shoot_mode = state.shoot_mode;

        const bool facingLeft = (state.flags & PLAYER_FLAG_FACING_LEFT) != 0;
        const bool moving = (state.flags & PLAYER_FLAG_MOVING) != 0;
        const bool animFiring = (state.flags & PLAYER_FLAG_FIRING_ANIM) != 0;
        const bool isTurn = (state.flags & PLAYER_FLAG_MY_TURN) != 0;
        const bool projectileActive = (state.flags & PLAYER_FLAG_PROJECTILE_FIRED) != 0;

        if (i == 0)
        {
            player1_left = facingLeft;
            p1isMoving = moving;
            player_1turn = isTurn;
            player1TankNumber = state.tankType;
        }
        else if (i == 1)
        {
            player2_left = facingLeft;
            p2isMoving = moving;
            player_2turn = isTurn;
            player2TankNumber = state.tankType;
        }

        player->isFire = projectileActive;
        if (projectileActive)
            player->set_ball();

        firingAnim = firingAnim || animFiring;
    }
    isFired = firingAnim;
}

static void ApplyFirePacket(const struct PKT_FIRE& pkt)
{
    Fire* player = GetPlayerById(pkt.playerId);
    if (!player)
        return;

    player->left = pkt.startX;
    player->top = pkt.startY;
    player->angle = pkt.angle;
    player->power = pkt.power;
    player->shoot_mode = pkt.shoot_mode;
    player->set_ball();
    player->isFire = true;
}

DWORD WINAPI RecvThread(LPVOID)
{
    char buf[1024];

    while (true)
    {
        int recvlen = recv(g_sock, buf, sizeof(buf), 0);
        if (recvlen <= 0)
            break;

        if (recvlen < 1)
            continue;

        BYTE type = static_cast<BYTE>(buf[0]);

        switch (type)
        {
        case PKT_JOIN:
            if (recvlen >= (int)sizeof(struct PKT_JOIN))
            {
                struct PKT_JOIN pkt {};
                memcpy(&pkt, buf, sizeof(struct PKT_JOIN));
                g_myPlayerId = pkt.playerId;
                g_isConnected = true;
                printf("서버로부터 내 ID = %d\n", g_myPlayerId);
            }
            break;

        case PKT_STATE:
            if (recvlen >= (int)sizeof(struct PKT_STATE))
            {
                struct PKT_STATE pkt {};
                memcpy(&pkt, buf, sizeof(struct PKT_STATE));
                ApplyStatePacket(pkt);
            }
            break;

        case PKT_FIRE:
            if (recvlen >= (int)sizeof(struct PKT_FIRE))
            {
                struct PKT_FIRE pkt {};
                memcpy(&pkt, buf, sizeof(struct PKT_FIRE));
                if (pkt.playerId != g_myPlayerId)
                    ApplyFirePacket(pkt);
            }
            break;
        case PKT_TYPE_TERRAIN_DELTA:
            if (recvlen >= (int)sizeof(PKT_TERRAIN_DELTA))
            {
                PKT_TERRAIN_DELTA pkt{};
                memcpy(&pkt, buf, sizeof(PKT_TERRAIN_DELTA));

                // ★ playerId 비교 X — 지형은 모든 클라 동기화
                ApplyTerrainDelta(pkt);
            }
            break;

        default:
            break;
        }
    }

    g_isConnected = false;
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
    // use InetPtonA instead of deprecated inet_addr
    if (InetPtonA(AF_INET, serverIp, &serveraddr.sin_addr) != 1) {
        closesocket(g_sock);
        g_sock = INVALID_SOCKET;
        WSACleanup();
        return false;
    }
    serveraddr.sin_port = htons(port);

    if (connect(g_sock, (SOCKADDR*)&serveraddr, sizeof(serveraddr)) == SOCKET_ERROR)
    {
        closesocket(g_sock);
        g_sock = INVALID_SOCKET;
        WSACleanup();
        return false;
    }

    g_recvThread = CreateThread(NULL, 0, RecvThread, NULL, 0, NULL);
    if (g_recvThread)
        CloseHandle(g_recvThread);

    return true;
}

void SendPacket(const char* buf, int len)
{
    if (g_sock == INVALID_SOCKET)
        return;
    send(g_sock, buf, len, 0);
}

bool IsNetworkConnected()
{
    return g_isConnected;
}

int GetMyPlayerId()
{
    return g_myPlayerId;
}

bool CanControlPlayer(int playerIndex)
{
    // 오프라인(서버 연결X)일 때는 그냥 자유롭게 조작
    if (!g_isConnected)
        return true;

    // 내 ID가 아닌 탱크는 조작 불가
    if (g_myPlayerId != playerIndex)
        return false;

    // 내 ID는 맞는데, 현재 턴인지 확인
    if (playerIndex == 0)
        return player_1turn;
    else if (playerIndex == 1)
        return player_2turn;

    return false;
}

void SendFirePacket(int playerIndex, float startX, float startY, float angle, float power, int shootMode)
{
    if (!g_isConnected)
        return;

    struct PKT_FIRE pkt {};
    pkt.type = PKT_FIRE;
    pkt.playerId = playerIndex;
    pkt.startX = startX;
    pkt.startY = startY;
    pkt.angle = angle;
    pkt.power = power;
    pkt.shoot_mode = shootMode;
    SendPacket(reinterpret_cast<char*>(&pkt), sizeof(struct PKT_FIRE));
}

void SendPlayerState(int playerIndex)
{
    if (!g_isConnected)
        return;

    Fire* player = GetPlayerById(playerIndex);
    if (!player)
        return;

    PlayerStateData state{};
    state.left = static_cast<float>(player->left);
    state.top = static_cast<float>(player->top);
    state.angle = static_cast<float>(player->angle);
    state.hp = static_cast<float>(player->HP);
    state.power = static_cast<float>(player->power);
    state.shoot_mode = player->shoot_mode;
    state.tankType = (playerIndex == 0) ? player1TankNumber : player2TankNumber;

    state.flags = PLAYER_FLAG_VALID;

    const bool facingLeft = (playerIndex == 0) ? player1_left : player2_left;
    const bool moving = (playerIndex == 0) ? p1isMoving : p2isMoving;
    if (facingLeft)
        state.flags |= PLAYER_FLAG_FACING_LEFT;
    if (moving)
        state.flags |= PLAYER_FLAG_MOVING;
    if (isFired)
        state.flags |= PLAYER_FLAG_FIRING_ANIM;
    if (player->isFire)
        state.flags |= PLAYER_FLAG_PROJECTILE_FIRED;

    struct PKT_MOVE pkt {};
    pkt.type = PKT_MOVE;
    pkt.playerId = playerIndex;
    pkt.state = state;

    SendPacket(reinterpret_cast<char*>(&pkt), sizeof(struct PKT_MOVE));
}

void CloseNetwork()
{
    if (g_sock != INVALID_SOCKET)
    {
        closesocket(g_sock);
        g_sock = INVALID_SOCKET;
    }
    WSACleanup();
    g_isConnected = false;
    g_myPlayerId = -1;
}

// 지형 파괴 정보 서버 전송 함수
void SendTerrainDelta(int x, int y, int radius, int shootMode)
{
    if (!IsNetworkConnected()) return;
    PKT_TERRAIN_DELTA pkt{};
    pkt.type = PKT_TYPE_TERRAIN_DELTA;
    pkt.x = x; pkt.y = y;
    pkt.radius = radius;
    pkt.shoot_mode = shootMode;
    SendPacket((char*)&pkt, sizeof(pkt));
}

void SendTurnEndPacket()
{
    if (!g_isConnected)
        return;

    PKT_TURN_END pkt{};
    pkt.type = PKT_TYPE_TURN_END;
    pkt.playerId = g_myPlayerId;

    SendPacket(reinterpret_cast<char*>(&pkt), sizeof(pkt));
}