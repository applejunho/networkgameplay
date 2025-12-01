#pragma once
#define WIN32_LEAN_AND_MEAN
#define _WINSOCKAPI_
#include <windows.h>

#define MAX_PLAYER 2

// ---- Packet types ----
enum PACKET_TYPE : BYTE
{
    PKT_TYPE_JOIN = 1,
    PKT_TYPE_MOVE = 2,
    PKT_TYPE_FIRE = 3,
    PKT_TYPE_STATE = 4,
    PKT_TYPE_TERRAIN_DELTA = 5,
    PKT_TYPE_TURN_END = 6, // 추가
};

// ---- Player replication flags ----
enum PlayerFlags : BYTE
{
    PLAYER_FLAG_VALID = 1 << 0,
    PLAYER_FLAG_FACING_LEFT = 1 << 1,
    PLAYER_FLAG_MOVING = 1 << 2,
    PLAYER_FLAG_FIRING_ANIM = 1 << 3,
    PLAYER_FLAG_MY_TURN = 1 << 4,
    PLAYER_FLAG_PROJECTILE_FIRED = 1 << 5,
};

#pragma pack(push, 1)

struct PlayerStateData
{
    float left;
    float top;
    float angle;
    float hp;
    float power;
    int   shoot_mode;
    int   tankType;
    BYTE  flags;
};

struct PKT_JOIN
{
    BYTE type;
    int  playerId;
};

struct PKT_MOVE
{
    BYTE            type;
    int             playerId;
    PlayerStateData state;
};

struct PKT_FIRE
{
    BYTE type;
    int  playerId;
    float startX;
    float startY;
    float angle;
    float power;
    int   shoot_mode;
};

struct PKT_STATE
{
    BYTE type;
    int  playerCount;
    PlayerStateData players[MAX_PLAYER];
};

struct PKT_TURN_END
{
    BYTE type;      // PKT_TYPE_TURN_END
    int  playerId;  // 턴을 끝내는 플레이어
};

struct PKT_TERRAIN_DELTA {
    BYTE type;        // PKT_TYPE_TERRAIN_DELTA
    int  x;           // 충돌/파괴 중심
    int  y;
    int  radius;      
    int  shoot_mode;  
};

#pragma pack(pop)