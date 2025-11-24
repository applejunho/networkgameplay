// GameServer_GameLogic.cpp
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS

#include <winsock2.h>
#include <windows.h>
#include <stdio.h>
#include <stdint.h>
#include <math.h>

#pragma comment(lib, "ws2_32")

// -------------------- 설정 --------------------
#define SERVERPORT 9000
#define BUFSIZE 512
#define MAX_CLIENT 3
#define MAX_PROJECTILES 128
#define MAX_TERRAIN_EVENTS 64

#define TICK_MS 33            // 약 30Hz 업데이트
#define COLLISION_MS 16       // 충돌 스레드 주기
#define TERRAIN_MS 100        // 지형 스레드 주기

// -------------------- 패킷 타입 (간단) --------------------
// 텍스트 기반 간단 프로토콜도 사용 가능 ("STATE ..." 문자열 등)
// 아래는 내부 상수만 정의
enum {
    PKT_ASSIGN_ID = 100,
    PKT_INPUT = 1,      // 클라 -> 서버: InputPacket
    PKT_STATE = 200,    // 서버 -> 클라: 전체 상태(문자열 또는 바이너리)
    PKT_TERRAIN = 201
};

// -------------------- 데이터 구조 --------------------
typedef struct Player {
    BOOL connected;
    SOCKET sock;
    uint32_t clientId;

    // 게임 상태
    int x;
    int y;
    float angle;    // degree
    int hp;
    BOOL alive;

    // action flags (서버가 현재 가진 상태)
    BOOL firing;    // space (발사)
    BOOL skill1;    // F1
    BOOL skill2;    // F2
    BOOL skill3;    // F3

} Player;

typedef struct Projectile {
    BOOL active;
    uint32_t id;
    uint32_t ownerId;
    float x;
    float y;
    float vx;
    float vy;
    int damage;
    int life; // ticks remaining
} Projectile;

typedef struct TerrainEvent {
    BOOL valid;
    int x;
    int y;
    int radius;
} TerrainEvent;

// Input packet from client (binary)
#pragma pack(push,1)
typedef struct InputPacket {
    char type;   // 'I'
    char key;    // 'A','D','W','S',' ' (space), '1','2','3' for skills
    // optional: could include angle/power if client sends them
    // For security we will validate any position/angle if provided
} InputPacket;
#pragma pack(pop)

// -------------------- 전역 상태 --------------------
static Player players[MAX_CLIENT];
static CRITICAL_SECTION csPlayers;

static Projectile projectiles[MAX_PROJECTILES];
static CRITICAL_SECTION csProjectiles;

static TerrainEvent terrainEvents[MAX_TERRAIN_EVENTS];
static CRITICAL_SECTION csTerrainEvents;

// Id generators
static uint32_t nextClientId = 1;
static uint32_t nextProjectileId = 1;

// map / terrain parameters (단순화)
static const int MAP_WIDTH = 1600;
static const int MAP_HEIGHT = 800;
static const int GROUND_Y = 600; // 단순 지형: y >= GROUND_Y 는 땅 (지형 파괴는 radius 처리)

// -------------------- 유틸 --------------------
void err_quit(const char* msg) {
    LPVOID lpMsgBuf;
    FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL, WSAGetLastError(),
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (char*)&lpMsgBuf, 0, NULL);
    MessageBoxA(NULL, (const char*)lpMsgBuf, msg, MB_ICONERROR);
    LocalFree(lpMsgBuf);
    exit(1);
}
void err_display(const char* msg) {
    LPVOID lpMsgBuf;
    FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL, WSAGetLastError(),
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (char*)&lpMsgBuf, 0, NULL);
    printf("[%s] %s\n", msg, (char*)lpMsgBuf);
    LocalFree(lpMsgBuf);
}

// 안전 send (전체 전송 보장하려면 loop 필요)
int SafeSend(SOCKET s, const char* buf, int len) {
    int sent = 0;
    while (sent < len) {
        int r = send(s, buf + sent, len - sent, 0);
        if (r == SOCKET_ERROR) return SOCKET_ERROR;
        sent += r;
    }
    return sent;
}

// -------------------- 함수 선언 (요청한 명칭 포함) --------------------
BOOL validate(int x, int y, float angle);
void simulateProjectile(Projectile* p);
void ApplyDamage(uint32_t targetClientId, int damage);
DWORD WINAPI StartUpdateThread(LPVOID arg);
DWORD WINAPI StartCollisionThread(LPVOID arg);
DWORD WINAPI StartTerrainThread(LPVOID arg);

// -------------------- validate 구현 --------------------
// 플레이어가 보낸 (클라이언트-제공) 값 검증(좌표/각도 범위 등)
BOOL validate(int x, int y, float angle) {
    // 좌표 범위
    if (x < 0 || x > MAP_WIDTH) return FALSE;
    if (y < 0 || y > MAP_HEIGHT) return FALSE;
    // 각도 범위 (예: 0~180)
    if (angle < -360.0f || angle > 360.0f) return FALSE; // 더 엄격하게 바꿀 수 있음
    return TRUE;
}

// -------------------- simulateProjectile 구현 --------------------
// 한 투사체의 1틱(또는 호출주기) 시뮬레이션: 중력, 위치 업데이트, 충돌 판정(플레이어/지형)
// 충돌이 발생하면 projectile->active=false 하고 필요한 후속 작업(데미지, 지형 이벤트)을 생성
void simulateProjectile(Projectile* p) {
    if (!p->active) return;

    // 물리 통합: dt 가 1 틱이라고 가정 (충돌 스레드 주기에 맞춰 보정될 수 있음)
    const float dt = COLLISION_MS / 1000.0f; // 예: 16ms -> 0.016s
    const float gravity = 200.0f; // 픽셀/s^2 (임의 값 — 조정 필요)

    // 위치 업데이트 (단순 오일러)
    p->x += p->vx * dt;
    p->y += p->vy * dt;
    // 중력은 vy에 더함
    p->vy += gravity * dt;

    // 생명 감소
    p->life--;
    if (p->life <= 0) {
        p->active = FALSE;
        return;
    }

    // 화면 밖이면 소멸
    if (p->x < -100 || p->x > MAP_WIDTH + 100 || p->y > MAP_HEIGHT + 200) {
        p->active = FALSE;
        return;
    }

    // 플레이어 충돌 검사 (거리 기반 박스검사 또는 원 검사)
    EnterCriticalSection(&csPlayers);
    for (int i = 0; i < MAX_CLIENT; ++i) {
        if (!players[i].connected || !players[i].alive) continue;
        // 플레이어 중심과의 거리
        float dx = players[i].x - p->x;
        float dy = players[i].y - p->y;
        float dist2 = dx * dx + dy * dy;
        const float hitRadius = 24.0f; // 히트 박스 반경
        if (dist2 <= hitRadius * hitRadius) {
            // 히트!
            p->active = FALSE;
            // ApplyDamage(플레이어 id, 데미지)
            ApplyDamage(players[i].clientId, p->damage);
            // 지형 파괴 이벤트 등록 (폭발)
            EnterCriticalSection(&csTerrainEvents);
            for (int t = 0; t < MAX_TERRAIN_EVENTS; ++t) {
                if (!terrainEvents[t].valid) {
                    terrainEvents[t].valid = TRUE;
                    terrainEvents[t].x = (int)roundf(p->x);
                    terrainEvents[t].y = (int)roundf(p->y);
                    terrainEvents[t].radius = 32; // 폭발 반경
                    break;
                }
            }
            LeaveCriticalSection(&csTerrainEvents);
            break;
        }
    }
    LeaveCriticalSection(&csPlayers);

    if (!p->active) return;

    // 지형 충돌: 여기서는 간단히 y >= GROUND_Y 를 땅으로 판단
    if (p->y >= GROUND_Y) {
        p->active = FALSE;
        // 지형 이벤트 추가 (땅 폭발)
        EnterCriticalSection(&csTerrainEvents);
        for (int t = 0; t < MAX_TERRAIN_EVENTS; ++t) {
            if (!terrainEvents[t].valid) {
                terrainEvents[t].valid = TRUE;
                terrainEvents[t].x = (int)roundf(p->x);
                terrainEvents[t].y = (int)roundf(p->y);
                terrainEvents[t].radius = 48;
                break;
            }
        }
        LeaveCriticalSection(&csTerrainEvents);
    }
}

// -------------------- ApplyDamage 구현 --------------------
// 대상 클라이언트 id를 찾아서 hp 감소, 죽음 처리, 그리고 상태를 브로드캐스트
void ApplyDamage(uint32_t targetClientId, int damage) {
    EnterCriticalSection(&csPlayers);
    for (int i = 0; i < MAX_CLIENT; ++i) {
        if (!players[i].connected) continue;
        if (players[i].clientId == targetClientId) {
            players[i].hp -= damage;
            if (players[i].hp <= 0) {
                players[i].hp = 0;
                players[i].alive = FALSE;
            }
            // 상태 브로드캐스트 (간단 텍스트 메시지)
            char msg[128];
            sprintf(msg, "DAMAGE %u %d %d", targetClientId, damage, players[i].hp);
            // send to all
            for (int j = 0; j < MAX_CLIENT; ++j) {
                if (players[j].connected) SafeSend(players[j].sock, msg, (int)strlen(msg) + 1);
            }
            break;
        }
    }
    LeaveCriticalSection(&csPlayers);
}

// -------------------- StartUpdateThread --------------------
// 주기적으로 전체 게임 상태를 브로드캐스트
DWORD WINAPI StartUpdateThread(LPVOID arg) {
    (void)arg;
    char out[2048];

    while (1) {
        Sleep(TICK_MS);

        // Build a simple text-based state packet:
        // "STATE count\nid x y angle hp alive firing s1 s2 s3\n..."
        int pos = 0;
        pos += sprintf(out + pos, "STATE %d\n", MAX_CLIENT);
        EnterCriticalSection(&csPlayers);
        for (int i = 0; i < MAX_CLIENT; ++i) {
            if (players[i].connected) {
                pos += sprintf(out + pos, "%u %d %d %.1f %d %d %d %d %d\n",
                    players[i].clientId, players[i].x, players[i].y,
                    players[i].angle, players[i].hp, players[i].alive ? 1 : 0,
                    players[i].firing ? 1 : 0,
                    players[i].skill1 ? 1 : 0,
                    players[i].skill2 ? 1 : 0
                );
            }
            else {
                // empty slot
                pos += sprintf(out + pos, "0 0 0 0.0 0 0 0 0 0\n");
            }
        }
        LeaveCriticalSection(&csPlayers);

        // Projectiles
        EnterCriticalSection(&csProjectiles);
        pos += sprintf(out + pos, "BULLETS %d\n", MAX_PROJECTILES);
        for (int i = 0; i < MAX_PROJECTILES; ++i) {
            if (projectiles[i].active) {
                pos += sprintf(out + pos, "%u %.1f %.1f\n", projectiles[i].id, projectiles[i].x, projectiles[i].y);
            }
            else {
                pos += sprintf(out + pos, "0 0.0 0.0\n");
            }
        }
        LeaveCriticalSection(&csProjectiles);

        // Broadcast to all connected clients
        for (int i = 0; i < MAX_CLIENT; ++i) {
            if (players[i].connected) {
                SafeSend(players[i].sock, out, (int)strlen(out) + 1);
            }
        }
    }

    return 0;
}

// -------------------- StartCollisionThread --------------------
DWORD WINAPI StartCollisionThread(LPVOID arg) {
    (void)arg;
    while (1) {
        Sleep(COLLISION_MS);
        EnterCriticalSection(&csProjectiles);
        for (int i = 0; i < MAX_PROJECTILES; ++i) {
            if (projectiles[i].active) {
                simulateProjectile(&projectiles[i]);
            }
        }
        LeaveCriticalSection(&csProjectiles);
    }
    return 0;
}

// -------------------- StartTerrainThread --------------------
// terrainEvents 큐 처리: 현재는 이벤트를 클라이언트에 브로드캐스트하고 큐 비움
DWORD WINAPI StartTerrainThread(LPVOID arg) {
    (void)arg;
    char msg[128];
    while (1) {
        Sleep(TERRAIN_MS);
        EnterCriticalSection(&csTerrainEvents);
        for (int i = 0; i < MAX_TERRAIN_EVENTS; ++i) {
            if (!terrainEvents[i].valid) continue;
            // 간단히 브로드캐스트
            sprintf(msg, "TERRAIN_EXPLODE %d %d %d", terrainEvents[i].x, terrainEvents[i].y, terrainEvents[i].radius);
            // send to all players
            EnterCriticalSection(&csPlayers);
            for (int p = 0; p < MAX_CLIENT; ++p) {
                if (players[p].connected) SafeSend(players[p].sock, msg, (int)strlen(msg) + 1);
            }
            LeaveCriticalSection(&csPlayers);

            // 처리 완료
            terrainEvents[i].valid = FALSE;
        }
        LeaveCriticalSection(&csTerrainEvents);
    }
    return 0;
}

// -------------------- Projectile 생성 도우미 --------------------
void SpawnProjectile(uint32_t ownerId, float sx, float sy, float angleDeg) {
    // 속도는 angle과 파워 기반으로 정한다 (임시값)
    float power = 500.0f; // 픽셀/초
    float rad = (angleDeg) * 3.14159265f / 180.0f;
    float vx = cosf(rad) * power;
    float vy = -sinf(rad) * power; // 위로 쏘면 음수 vy

    EnterCriticalSection(&csProjectiles);
    for (int i = 0; i < MAX_PROJECTILES; ++i) {
        if (!projectiles[i].active) {
            projectiles[i].active = TRUE;
            projectiles[i].id = nextProjectileId++;
            projectiles[i].ownerId = ownerId;
            projectiles[i].x = sx;
            projectiles[i].y = sy;
            projectiles[i].vx = vx;
            projectiles[i].vy = vy;
            projectiles[i].damage = 25;
            projectiles[i].life = 300; // 대략 300 ticks limit
            break;
        }
    }
    LeaveCriticalSection(&csProjectiles);
}

// -------------------- 클라이언트 수신 스레드 --------------------
DWORD WINAPI ClientRecvThread(LPVOID arg) {
    SOCKET clientSock = (SOCKET)arg;

    // 식별자를 슬롯에 할당
    int slot = -1;
    EnterCriticalSection(&csPlayers);
    for (int i = 0; i < MAX_CLIENT; ++i) {
        if (!players[i].connected) {
            slot = i;
            players[i].connected = TRUE;
            players[i].sock = clientSock;
            players[i].clientId = nextClientId++;
            players[i].x = 100 + i * 80;
            players[i].y = GROUND_Y - 20;
            players[i].angle = 45.0f;
            players[i].hp = 100;
            players[i].alive = TRUE;
            players[i].firing = FALSE;
            players[i].skill1 = players[i].skill2 = players[i].skill3 = FALSE;
            break;
        }
    }
    LeaveCriticalSection(&csPlayers);

    if (slot == -1) {
        // 서버 풀 처리
        const char* full = "SERVER_FULL";
        send(clientSock, full, (int)strlen(full) + 1, 0);
        closesocket(clientSock);
        return 0;
    }

    // 클라이언트에게 할당된 ID 알림
    char idmsg[64];
    sprintf(idmsg, "ASSIGN_ID %u", players[slot].clientId);
    SafeSend(clientSock, idmsg, (int)strlen(idmsg) + 1);

    printf("Client connected slot=%d id=%u\n", slot, players[slot].clientId);

    // recv loop
    char buf[BUFSIZE];
    while (1) {
        int r = recv(clientSock, buf, sizeof(InputPacket), 0);
        if (r <= 0) break;

        // 최소한 sizeof(InputPacket)인지 확인
        if (r < (int)sizeof(InputPacket)) {
            // 무시하거나 계속 수신
            continue;
        }

        InputPacket* ip = (InputPacket*)buf;
        if (ip->type != 'I') continue;

        // 키 처리
        char key = ip->key;
        EnterCriticalSection(&csPlayers);

        // validate: 여기서는 클라이언트가 위치을 보내지 않으므로 입력 자체 유효성 검증
        // 허용되는 키: A,D,W,S,' ' (space), '1','2','3'
        BOOL ok = TRUE;
        if (!(key == 'A' || key == 'D' || key == 'W' || key == 'S' || key == ' ' || key == '1' || key == '2' || key == '3')) ok = FALSE;

        if (!ok) {
            // 비정상 입력 무시
            LeaveCriticalSection(&csPlayers);
            continue;
        }

        // 처리: a/d/w/s 조작, space 발사, 1/2/3 스킬
        if (key == 'A') {
            players[slot].x -= 4;
            if (players[slot].x < 0) players[slot].x = 0;
        }
        else if (key == 'D') {
            players[slot].x += 4;
            if (players[slot].x > MAP_WIDTH) players[slot].x = MAP_WIDTH;
        }
        else if (key == 'W') {
            players[slot].angle += 2.0f;
            if (players[slot].angle > 180.0f) players[slot].angle = 180.0f;
        }
        else if (key == 'S') {
            players[slot].angle -= 2.0f;
            if (players[slot].angle < -180.0f) players[slot].angle = -180.0f;
        }
        else if (key == ' ') {
            // 발사 (서버가 권한)
            // spawn projectile at player's muzzle position
            float sx = (float)players[slot].x;
            float sy = (float)players[slot].y - 10.0f;
            uint32_t owner = players[slot].clientId;
            SpawnProjectile(owner, sx, sy, players[slot].angle);
            players[slot].firing = TRUE;
        }
        else if (key == '1') {
            players[slot].skill1 = TRUE;
            // 스킬 효과(서버 적용 예: 자기 HP 회복)
            players[slot].hp += 20;
            if (players[slot].hp > 100) players[slot].hp = 100;
        }
        else if (key == '2') {
            players[slot].skill2 = TRUE;
            // 다른 임시 효과 가능
        }
        else if (key == '3') {
            players[slot].skill3 = TRUE;
            // ...
        }

        LeaveCriticalSection(&csPlayers);
    }

    // 연결 끊김 처리
    EnterCriticalSection(&csPlayers);
    printf("Client disconnected slot=%d id=%u\n", slot, players[slot].clientId);
    players[slot].connected = FALSE;
    players[slot].alive = FALSE;
    players[slot].clientId = 0;
    LeaveCriticalSection(&csPlayers);

    closesocket(clientSock);
    return 0;
}

// -------------------- 메인 --------------------
int main() {
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) return 1;

    // 초기화
    InitializeCriticalSection(&csPlayers);
    InitializeCriticalSection(&csProjectiles);
    InitializeCriticalSection(&csTerrainEvents);
    memset(players, 0, sizeof(players));
    memset(projectiles, 0, sizeof(projectiles));
    memset(terrainEvents, 0, sizeof(terrainEvents));

    SOCKET listenSock = socket(AF_INET, SOCK_STREAM, 0);
    if (listenSock == INVALID_SOCKET) err_quit("socket() error");
    printf("서버 소켓 생성됨\n");

    struct sockaddr_in serveraddr;
    memset(&serveraddr, 0, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serveraddr.sin_port = htons(SERVERPORT);
    if (bind(listenSock, (struct sockaddr*)&serveraddr, sizeof(serveraddr)) == SOCKET_ERROR) err_quit("bind() error");
    if (listen(listenSock, SOMAXCONN) == SOCKET_ERROR) err_quit("listen() error");
    printf("클라이언트 접속 대기 중...\n");

    // 시작 스레드: 업데이트, 충돌, 지형 작업
    HANDLE hUpdate = CreateThread(NULL, 0, StartUpdateThread, NULL, 0, NULL);
    HANDLE hCollision = CreateThread(NULL, 0, StartCollisionThread, NULL, 0, NULL);
    HANDLE hTerrain = CreateThread(NULL, 0, StartTerrainThread, NULL, 0, NULL);
    if (hUpdate) CloseHandle(hUpdate);
    if (hCollision) CloseHandle(hCollision);
    if (hTerrain) CloseHandle(hTerrain);

    // accept loop
    while (1) {
        struct sockaddr_in clientaddr;
        int addrlen = sizeof(clientaddr);
        SOCKET client_sock = accept(listenSock, (struct sockaddr*)&clientaddr, &addrlen);
        if (client_sock == INVALID_SOCKET) {
            err_display("accept()");
            continue;
        }

        // spawn client recv thread
        HANDLE h = CreateThread(NULL, 0, ClientRecvThread, (LPVOID)client_sock, 0, NULL);
        if (h) CloseHandle(h);
    }

    // 정리 - 사실 도달하지 않음
    closesocket(listenSock);
    WSACleanup();
    DeleteCriticalSection(&csPlayers);
    DeleteCriticalSection(&csProjectiles);
    DeleteCriticalSection(&csTerrainEvents);

    return 0;
}
