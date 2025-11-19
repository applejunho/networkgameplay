#pragma once
#include "NetCommon.h"

// 외부에서 접근할 소켓
extern SOCKET g_sock;

// 함수 선언
bool InitNetwork(const char* serverIp, int port);
void SendPacket(const char* buf, int len);