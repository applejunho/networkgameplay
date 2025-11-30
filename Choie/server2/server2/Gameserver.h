#pragma once
#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <windows.h>
#include <stdint.h>
#include "Packet.h"

#define SERVERPORT 9000
#define BUFSIZE 512
#define MAX_PLAYER 3
#define MAX_PROJECTILES 128
#define MAX_TERRAIN_EVENTS 64
#define TICK_MS 33
#define COLLISION_MS 16
#define TERRAIN_MS 100

struct PlayerSlot {
    BOOL connected;
    SOCKET sock;
    int playerId;
    PlayerStateData state;
};

struct Projectile {
    BOOL active;
    uint32_t id;
    uint32_t ownerId;
    float x, y, vx, vy;
    int damage;
    int life;
};

struct TerrainEvent {
    BOOL valid;
    int x, y, radius, shoot_mode;
};

extern PlayerSlot players[MAX_PLAYER];
extern Projectile projectiles[MAX_PROJECTILES];
extern TerrainEvent terrainEvents[MAX_TERRAIN_EVENTS];

extern CRITICAL_SECTION csPlayers;
extern CRITICAL_SECTION csProjectiles;
extern CRITICAL_SECTION csTerrainEvents;

extern uint32_t nextClientId;
extern uint32_t nextProjectileId;

void err_quit(const char* msg);
int SafeSend(SOCKET s, const char* buf, int len);

void simulateProjectile(Projectile* p);
void ApplyDamage(int targetId, int damage);
void SpawnProjectile(int ownerId, float sx, float sy, float angleDeg);

DWORD WINAPI StartUpdateThread(LPVOID arg);
DWORD WINAPI StartCollisionThread(LPVOID arg);
DWORD WINAPI StartTerrainThread(LPVOID arg);
DWORD WINAPI ClientRecvThread(LPVOID arg);
