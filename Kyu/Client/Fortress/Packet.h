#pragma once
#define WIN32_LEAN_AND_MEAN
#define _WINSOCKAPI_ 
#include <windows.h>

const BYTE PKT_FIRE_TYPE = 1;   // 패킷 타입 값

#pragma pack(push, 1)
struct PKT_FIRE
{
    BYTE  type;
    int   playerId;
    float startX;
    float startY;
    float angle;
    float power;
    int   shoot_mode;
};
#pragma pack(pop)
