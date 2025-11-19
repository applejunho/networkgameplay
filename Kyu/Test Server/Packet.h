#pragma once
#include <stdint.h>   // BYTE 정의 대신 사용

enum PACKET_TYPE : uint8_t
{
    PKT_FIRE = 1,
};

#pragma pack(push, 1)
struct PKT_FIRE
{
    uint8_t type;
    int playerId;
    float startX;
    float startY;
    float angle;
    float power;
    int shoot_mode;
};
#pragma pack(pop)
