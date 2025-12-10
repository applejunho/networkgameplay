#pragma once
#include "NetCommon.h"
#include "Packet.h"

extern SOCKET g_sock;

bool InitNetwork(const char* serverIp, int port);
void CloseNetwork();

bool IsNetworkConnected();
int  GetMyPlayerId();
bool CanControlPlayer(int playerIndex);

void SendPacket(const char* buf, int len);
void SendPlayerState(int playerIndex, bool force = false);
void SendFirePacket(int playerIndex, float startX, float startY, float angle, float power, int shootMode);
void SendTerrainDelta(int x, int y, int radius, int shootMode);