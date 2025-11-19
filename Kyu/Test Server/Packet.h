// Packet.h
#pragma once
#include <Windows.h>

enum PACKET_TYPE : BYTE
{
    PKT_FIRE = 1,
};

#pragma pack(push, 1)
struct FirePacket
{
    BYTE  type;       // PKT_FIRE
    int   playerId;
    float startX;
    float startY;
    float angle;
    float power;
    int   shoot_mode;
};
#pragma pack(pop)
