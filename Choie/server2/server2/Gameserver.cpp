#include "GameServer.h"
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>

#pragma comment(lib, "ws2_32")

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

PlayerSlot players[MAX_PLAYER];
Projectile projectiles[MAX_PROJECTILES];
TerrainEvent terrainEvents[MAX_TERRAIN_EVENTS];

CRITICAL_SECTION csPlayers;
CRITICAL_SECTION csProjectiles;
CRITICAL_SECTION csTerrainEvents;

uint32_t nextClientId = 1;
uint32_t nextProjectileId = 1;

// ---- 유틸 ----
void err_quit(const char* msg) {
    LPVOID lpMsgBuf;
    FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL,
        WSAGetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (char*)&lpMsgBuf, 0, NULL);
    MessageBoxA(NULL, (const char*)lpMsgBuf, msg, MB_ICONERROR);
    LocalFree(lpMsgBuf);
    exit(1);
}

int SafeSend(SOCKET s, const char* buf, int len) {
    int sent = 0;
    while (sent < len) {
        int r = send(s, buf + sent, len - sent, 0);
        if (r == SOCKET_ERROR) return SOCKET_ERROR;
        sent += r;
    }
    return sent;
}

// ---- Framing helpers ----
static int SendFramed(SOCKET s, BYTE type, const void* payload, WORD size) {
    PKT_HEADER hdr;
    hdr.type = type;
    hdr.size = size;

    char outbuf[BUFSIZE];
    int total = sizeof(PKT_HEADER) + size;
    if (total > BUFSIZE) return SOCKET_ERROR;

    memcpy(outbuf, &hdr, sizeof(hdr));
    memcpy(outbuf + sizeof(hdr), payload, size);
    return SafeSend(s, outbuf, total);
}

static void BroadcastFramed(BYTE type, const void* payload, WORD size) {
    EnterCriticalSection(&csPlayers);
    for (int i = 0; i < MAX_PLAYER; ++i) {
        if (players[i].connected)
            SendFramed(players[i].sock, type, payload, size);
    }
    LeaveCriticalSection(&csPlayers);
}

// ---- 게임 로직 ----
void ApplyDamage(int targetId, int damage) {
    EnterCriticalSection(&csPlayers);
    for (int i = 0; i < MAX_PLAYER; i++) {
        if (players[i].connected && players[i].playerId == targetId) {
            players[i].state.hp -= damage;
            if (players[i].state.hp <= 0) {
                players[i].state.hp = 0;
                players[i].state.flags &= ~PLAYER_FLAG_VALID;
            }
            break;
        }
    }
    LeaveCriticalSection(&csPlayers);
}

void SpawnProjectile(int ownerId, float sx, float sy, float angleDeg) {
    EnterCriticalSection(&csProjectiles);
    for (int i = 0; i < MAX_PROJECTILES; i++) {
        if (!projectiles[i].active) {
            Projectile* p = &projectiles[i];
            p->active = TRUE;
            p->id = nextProjectileId++;
            p->ownerId = ownerId;
            p->x = sx;
            p->y = sy;
            float rad = angleDeg * (float)M_PI / 180.0f;
            p->vx = cosf(rad) * 400.0f;
            p->vy = -sinf(rad) * 400.0f;
            p->damage = 20;
            p->life = 200;
            break;
        }
    }
    LeaveCriticalSection(&csProjectiles);
}

void simulateProjectile(Projectile* p) {
    if (!p->active) return;
    const float dt = COLLISION_MS / 1000.0f;
    const float gravity = 200.0f;

    p->vy += gravity * dt;
    p->x += p->vx * dt;
    p->y += p->vy * dt;

    if (--p->life <= 0) { p->active = FALSE; return; }
    if (p->x < 0 || p->x > 1600 || p->y < 0 || p->y > 800) { p->active = FALSE; return; }

    if ((int)p->y >= 600) {
        p->active = FALSE;
        EnterCriticalSection(&csTerrainEvents);
        for (int i = 0; i < MAX_TERRAIN_EVENTS; i++) {
            if (!terrainEvents[i].valid) {
                terrainEvents[i].valid = TRUE;
                terrainEvents[i].x = (int)p->x;
                terrainEvents[i].y = 600;
                terrainEvents[i].radius = 30;
                terrainEvents[i].shoot_mode = 0;
                break;
            }
        }
        LeaveCriticalSection(&csTerrainEvents);
        return;
    }

    const float hitR = 20.0f;
    EnterCriticalSection(&csPlayers);
    for (int i = 0; i < MAX_PLAYER; i++) {
        PlayerSlot* ps = &players[i];
        if (!ps->connected) continue;
        float dx = p->x - (float)ps->state.left;
        float dy = p->y - (float)ps->state.top;
        if (dx * dx + dy * dy <= hitR * hitR) {
            int targetId = ps->playerId;
            LeaveCriticalSection(&csPlayers);
            p->active = FALSE;
            ApplyDamage(targetId, p->damage);
            return;
        }
    }
    LeaveCriticalSection(&csPlayers);
}

// ---- 스레드 루프 ----
DWORD WINAPI StartUpdateThread(LPVOID arg) {
    (void)arg;
    for (;;) {
        PKT_STATE pktState;
        pktState.type = PKT_STATE;
        pktState.playerCount = MAX_PLAYER;

        EnterCriticalSection(&csPlayers);
        for (int i = 0; i < MAX_PLAYER; i++) {
            pktState.players[i] = players[i].state;
        }
        LeaveCriticalSection(&csPlayers);

        BroadcastFramed(PKT_STATE, &pktState, sizeof(pktState));
        Sleep(TICK_MS);
    }
    return 0;
}

DWORD WINAPI StartCollisionThread(LPVOID arg) {
    (void)arg;
    for (;;) {
        EnterCriticalSection(&csProjectiles);
        for (int i = 0; i < MAX_PROJECTILES; i++) {
            if (projectiles[i].active)
                simulateProjectile(&projectiles[i]);
        }
        LeaveCriticalSection(&csProjectiles);
        Sleep(COLLISION_MS);
    }
    return 0;
}

DWORD WINAPI StartTerrainThread(LPVOID arg) {
    (void)arg;
    for (;;) {
        EnterCriticalSection(&csTerrainEvents);
        for (int i = 0; i < MAX_TERRAIN_EVENTS; i++) {
            if (terrainEvents[i].valid) {
                PKT_TERRAIN_DELTA pktDelta;
                pktDelta.type = PKT_TYPE_TERRAIN_DELTA;
                pktDelta.x = terrainEvents[i].x;
                pktDelta.y = terrainEvents[i].y;
                pktDelta.radius = terrainEvents[i].radius;
                pktDelta.shoot_mode = terrainEvents[i].shoot_mode;
                // terrainEvents[i] 처리 끝
            }
        }
        LeaveCriticalSection(&csTerrainEvents);
        Sleep(TERRAIN_MS);
    }
    return 0;
}

// ---- 클라이언트 수신 스레드 ----
static int FindPlayerSlotBySocket(SOCKET s) {
    int idx = -1;
    EnterCriticalSection(&csPlayers);
    for (int i = 0; i < MAX_PLAYER; ++i) {
        if (players[i].connected && players[i].sock == s) {
            idx = i;
            break;
        }
    }
    LeaveCriticalSection(&csPlayers);
    return idx;
}

DWORD WINAPI ClientRecvThread(LPVOID arg) {
    SOCKET s = (SOCKET)(uintptr_t)arg;
    char buf[BUFSIZE];

    for (;;) {
        int r = recv(s, buf, sizeof(buf), 0);
        if (r <= 0) {
            // 연결 끊김 처리
            EnterCriticalSection(&csPlayers);
            for (int i = 0; i < MAX_PLAYER; i++) {
                if (players[i].connected && players[i].sock == s) {
                    players[i].connected = FALSE;
                    players[i].sock = INVALID_SOCKET;
                    players[i].state.flags = 0;
                    break;
                }
            }
            LeaveCriticalSection(&csPlayers);
            closesocket(s);
            return 0;
        }

        // 패킷 파싱
        int offset = 0;
        while (offset + sizeof(PKT_HEADER) <= r) {
            PKT_HEADER* hdr = (PKT_HEADER*)(buf + offset);
            int frameEnd = offset + sizeof(PKT_HEADER) + hdr->size;
            if (frameEnd > r) break; // 불완전 프레임

            void* payload = buf + offset + sizeof(PKT_HEADER);
            int slot = FindPlayerSlotBySocket(s);

            if (slot >= 0) {

                switch (hdr->type) {
                case PKT_MOVE:
                    if (hdr->size == sizeof(PKT_MOVE)) {
                        PKT_MOVE* pktMove = (PKT_MOVE); payload;
                        EnterCriticalSection(&csPlayers);
                        for (int i = 0; i < MAX_PLAYER; i++) {
                            if (players[i].connected && players[i].playerId == pktMove->playerId) {
                                players[i].state = pktMove->state;
                                break;
                            }
                        }
                        LeaveCriticalSection(&csPlayers);
                    }
                    break;

                case PKT_FIRE:
                    if (hdr->size == sizeof(PKT_FIRE)) {
                        PKT_FIRE* pktFire = (PKT_FIRE); payload;
                        SpawnProjectile(pktFire->playerId,
                            pktFire->startX,
                            pktFire->startY,
                            pktFire->angle);
                    }
                    break;

                case PKT_JOIN:
                    // 필요시 처리
                    break;
                }
            }
            offset = frameEnd;
        }
    }
}
int main() {
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) return 1;

    InitializeCriticalSection(&csPlayers);
    InitializeCriticalSection(&csProjectiles);
    InitializeCriticalSection(&csTerrainEvents);
    memset(players, 0, sizeof(players));
    memset(projectiles, 0, sizeof(projectiles));
    memset(terrainEvents, 0, sizeof(terrainEvents));

    SOCKET listenSock = socket(AF_INET, SOCK_STREAM, 0);
    if (listenSock == INVALID_SOCKET) err_quit("socket() error");

    struct sockaddr_in serveraddr;
    memset(&serveraddr, 0, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serveraddr.sin_port = htons(SERVERPORT);
    if (bind(listenSock, (struct sockaddr*)&serveraddr, sizeof(serveraddr)) == SOCKET_ERROR)
        err_quit("bind() error");
    if (listen(listenSock, SOMAXCONN) == SOCKET_ERROR)
        err_quit("listen() error");

    // 게임 스레드 시작
    HANDLE hUpdate = CreateThread(NULL, 0, StartUpdateThread, NULL, 0, NULL);
    HANDLE hCollision = CreateThread(NULL, 0, StartCollisionThread, NULL, 0, NULL);
    HANDLE hTerrain = CreateThread(NULL, 0, StartTerrainThread, NULL, 0, NULL);
    if (hUpdate) CloseHandle(hUpdate);
    if (hCollision) CloseHandle(hCollision);
    if (hTerrain) CloseHandle(hTerrain);

    // 클라이언트 accept 루프
    while (1) {
        struct sockaddr_in clientaddr;
        int addrlen = sizeof(clientaddr);
        SOCKET client_sock = accept(listenSock, (struct sockaddr*)&clientaddr, &addrlen);
        if (client_sock == INVALID_SOCKET) continue;

        // 빈 슬롯 할당
        EnterCriticalSection(&csPlayers);
        int assigned = -1;
        for (int i = 0; i < MAX_PLAYER; i++) {
            if (!players[i].connected) {
                players[i].connected = TRUE;
                players[i].sock = client_sock;
                players[i].playerId = nextClientId++;
                players[i].state.flags = PLAYER_FLAG_VALID;
                assigned = i;
                break;
            }
        }
        LeaveCriticalSection(&csPlayers);

        // 수신 스레드 시작
        HANDLE h = CreateThread(NULL, 0, ClientRecvThread, (LPVOID)client_sock, 0, NULL);
        if (h) CloseHandle(h);
    }

    closesocket(listenSock);
    WSACleanup();
    DeleteCriticalSection(&csPlayers);
    DeleteCriticalSection(&csProjectiles);
    DeleteCriticalSection(&csTerrainEvents);

    return 0;
}