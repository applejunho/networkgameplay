#pragma once
#pragma pack(push, 1)

enum PACKET_TYPE {
    PKT_FIRE = 1,
    // 나중에 TERRAIN_UPDATE, WIND_UPDATE 등 추가
};

struct PKT_FIRE {
    BYTE type;          // PKT_FIRE
    int  playerId;      // 1 or 2
    float startX;
    float startY;
    float angle;
    float power;
    int   shoot_mode;   // 0,1,2,3 ...
};

#pragma pack(pop)
