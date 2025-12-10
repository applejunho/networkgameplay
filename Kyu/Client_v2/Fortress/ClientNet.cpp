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

struct LastFirePacket
{
    bool  hasData = false;
    struct PKT_FIRE pkt{};
    DWORD tick = 0;
    UINT32 inputVersion = 0;
};

static LastFirePacket g_lastFire[MAX_PLAYER];
static UINT32         g_fireInputVersion[MAX_PLAYER] = {};
static bool           g_hasFireInput[MAX_PLAYER] = {};
static float          g_lastAngle[MAX_PLAYER] = {};
static float          g_lastPower[MAX_PLAYER] = {};
static int            g_lastShootMode[MAX_PLAYER] = {};
static bool           g_lastProjectileActive = false;

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

        // 실시간 모드: 턴 항상 활성
        if (i == 0)
            player_1turn = true;
        else if (i == 1)
            player_2turn = true;

        if ((state.flags & PLAYER_FLAG_VALID) == 0)
            continue;

        if (!g_hasFireInput[i]
            || g_lastAngle[i] != state.angle
            || g_lastPower[i] != state.power
            || g_lastShootMode[i] != state.shoot_mode)
        {
            g_fireInputVersion[i]++;
            g_hasFireInput[i] = true;
            g_lastAngle[i] = state.angle;
            g_lastPower[i] = state.power;
            g_lastShootMode[i] = state.shoot_mode;
        }


        Fire* player = GetPlayerById(i);
        if (!player)
            continue;

        const auto lerp = [](double current, double target)
            {
                return current + (target - current) * 0.35;
            };

        if (CanControlPlayer(i))
        {
            player->left = state.left;
            player->top = state.top;
            player->angle = state.angle;
        }
        else
        {
            player->left = lerp(player->left, state.left);
            player->top = lerp(player->top, state.top);
            player->angle = lerp(player->angle, state.angle);
        }

        player->HP = state.hp;
        player->power = state.power;
        player->shoot_mode = state.shoot_mode;

        const bool facingLeft = (state.flags & PLAYER_FLAG_FACING_LEFT) != 0;
        const bool moving = (state.flags & PLAYER_FLAG_MOVING) != 0;
        const bool animFiring = (state.flags & PLAYER_FLAG_FIRING_ANIM) != 0;

        if (i == 0)
        {
            player1_left = facingLeft;
            p1isMoving = moving;
            player1TankNumber = state.tankType;
        }
        else if (i == 1)
        {
            player2_left = facingLeft;
            p2isMoving = moving;
            player2TankNumber = state.tankType;
        }

        firingAnim = firingAnim || animFiring;
    }

    isFired = firingAnim;


    // 프로젝트일 좌표는 서버 기준으로 초기화한다.
    // 양쪽 모두 발사 중이 아닐 때에만 덮어써, 진행 중인 로컬 탄도 계산을 건드리지 않는다.
    const bool acceptServerProjectile = !A.isFire && !B.isFire;

    if (acceptServerProjectile && pkt.projectileActive)
    {
        x = pkt.projX;
        y = pkt.projY;
    }

    // 서버에서 포탄이 사라졌다고 알려오면 좌표를 즉시 초기화해 잔상이 남지 않도록 한다.
    if (!pkt.projectileActive && g_lastProjectileActive)
    {
        x = -100.0;
        y = -100.0;
        g_lastProjectileActive = false;
    }

    g_lastProjectileActive = acceptServerProjectile
        ? pkt.projectileActive
        : (g_lastProjectileActive && pkt.projectileActive);
}



static void ApplyFirePacket(const struct PKT_FIRE& pkt)
{
    Fire* player = GetPlayerById(pkt.playerId);
    if (!player)
        return;

    if (player->isFire)
        return;

    LastFirePacket& cache = g_lastFire[pkt.playerId];
    if (cache.hasData)
    {
        const bool samePayload = memcmp(&cache.pkt, &pkt, sizeof(PKT_FIRE)) == 0;
        const bool sameInput = cache.inputVersion == g_fireInputVersion[pkt.playerId];
        if (samePayload && sameInput)
            return; // 입력 변화 없이 반복 수신된 동일 패킷은 무시
    }

    // 이미 발사 중인 탄이 있을 때 중복으로 새 발사를 만들지 않는다.
    if (player->isFire)
        return;


    player->left = pkt.startX;
    player->top = pkt.startY;
    player->angle = pkt.angle;
    player->power = pkt.power;
    player->shoot_mode = pkt.shoot_mode;
    player->set_ball();
    player->isFire = true;

    cache.hasData = true;
    cache.pkt = pkt;
    cache.tick = GetTickCount();
    cache.inputVersion = g_fireInputVersion[pkt.playerId];
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


    // 지연 없는 전송으로 이동/발사 보간 끊김을 줄인다.

    BOOL noDelay = TRUE;
    setsockopt(g_sock, IPPROTO_TCP, TCP_NODELAY, (const char*)&noDelay, sizeof(noDelay));

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
    // 오프라인(서버 연결X)일 때는 자유롭게 조작
    if (!g_isConnected)
        return true;

    // 온라인에서는 자신의 플레이어만 조작 가능
    return g_myPlayerId == playerIndex;
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

void SendPlayerState(int playerIndex, bool force)
{
    if (!g_isConnected)
        return;

    static DWORD lastSendTick[MAX_PLAYER] = { 0, 0 };
    DWORD now = GetTickCount();

    if (!force && now - lastSendTick[playerIndex] < 30)
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

    lastSendTick[playerIndex] = now;
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
