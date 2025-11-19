#pragma once
#define WIN32_LEAN_AND_MEAN
#define _WINSOCKAPI_
#include <windows.h>

#define MAX_PLAYER 3

// ---- 패킷 번호 ----
enum PACKET_TYPE : BYTE
{
    PKT_JOIN = 1,
    PKT_MOVE = 2,
    PKT_FIRE = 3,
    PKT_STATE = 4
};

#pragma pack(push, 1)

// ---- JOIN ----
struct PKT_JOIN
{
    BYTE type;
    int  playerId;
};

// ---- MOVE ----
struct PKT_MOVE
{
    BYTE type;
    int  playerId;
    float x, y;
    float angle;
};

// ---- FIRE ----
struct PKT_FIRE
{
    BYTE type;
    int  playerId;
    float startX;
    float startY;
    float angle;
    float power;
    int shoot_mode;        // ← Player.cpp에서 쓰는 필드 추가
};

// ---- 전체 상태 ----
struct PKT_STATE
{
    BYTE type;
    int  playerCount;
    float x[MAX_PLAYER];
    float y[MAX_PLAYER];
    float angle[MAX_PLAYER];
    float hp[MAX_PLAYER];
};

#pragma pack(pop)
