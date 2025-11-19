#pragma once
#define WIN32_LEAN_AND_MEAN
#define _WINSOCKAPI_
#include <windows.h>

#define MAX_PLAYER 3

// ---- Packet types ----
enum PACKET_TYPE : BYTE
{
    PKT_TYPE_JOIN = 1,
    PKT_TYPE_MOVE = 2,
    PKT_TYPE_FIRE = 3,
    PKT_TYPE_STATE = 4,
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

#pragma pack(pop)