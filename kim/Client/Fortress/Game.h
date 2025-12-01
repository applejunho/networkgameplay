// Game.h
#pragma once
#define WIN32_LEAN_AND_MEAN
#ifndef _WINSOCKAPI_
#define _WINSOCKAPI_
#endif
#include <windows.h>
#include "resource.h"
#include "Player.h"
#include "Packet.h"

extern int xFPos[12];
extern int yFPos[12];

// ===== 상수 =====
#define TARGET_COLOR RGB(255, 0, 255)
#define MAP1_X 1600
#define MAP1_Y 800
#define MAP2_X 1800
#define MAP2_Y 450

// ===== 전역 객체 =====
extern Fire A;
extern Fire B;

// 탄환 위치
extern double x;
extern double y;

// 바람, 카메라, 턴 등 상태
extern bool wind_right;
extern bool wind_left;

extern bool player1_left;
extern bool player2_left;

extern bool player_1turn;
extern bool player_2turn;

extern bool p1isMoving;
extern bool p2isMoving;

extern bool isHited;
extern bool isFired;

extern int  camera_x;
extern int  camera_y;

extern int  bulletAni;
extern int  tank1MovingAni;
extern int  tank23MovingAni;
extern int  tank1FireAni;
extern int  tank2FireAni;
extern int  tank3FireAni;
extern int  flareAni;
extern int  map13WindAni;
extern int  map2WindAni;

extern int  player1TankNumber;
extern int  player2TankNumber;

extern bool AitemFix, AitemTeleport, AitemPowerup;
extern bool BitemFix, BitemTeleport, BitemPowerup;

extern bool camera_mode;

extern bool isCharacter1Selected;
extern bool isCharacter2Selected;
extern bool isMapSelected;
extern bool isStarted;
extern bool isSelectedCharacter;
extern bool isSelectedMap;

extern int  select_map;

extern bool player1_select;
extern bool player2_select;

extern bool player1_rect1, player1_rect2, player1_rect3;
extern bool player2_rect1, player2_rect2, player2_rect3;
extern bool map_rect1, map_rect2, map_rect3;

extern int  xPos[10];
extern int  yPos[10];
extern int  xSet[10];

extern int  xFPos[12];
extern int  yFPos[12];

extern bool isShellCollision;

extern int  randNum;

// ===== 비트맵 핸들 =====
extern HBITMAP hBitmap;      // 배경 맵
extern HBITMAP UI;
extern HBITMAP CMSelect;

extern HBITMAP flare[12];

extern HBITMAP bullet1[4];   // 1P 탄
extern HBITMAP sBullet1[4];  // 1P 특수탄
extern HBITMAP bullet2[4];   // 2P 탄
extern HBITMAP sBullet2[4];  // 2P 특수탄

extern HBITMAP tank1_idle_left;
extern HBITMAP tank1_idle_right;
extern HBITMAP tank1_moving_left[5];
extern HBITMAP tank1_moving_right[5];
extern HBITMAP tank1_hited_left;
extern HBITMAP tank1_hited_right;
extern HBITMAP tank1_fire_left[6];
extern HBITMAP tank1_fire_right[6];

extern HBITMAP tank2_idle_left;
extern HBITMAP tank2_idle_right;
extern HBITMAP tank2_moving_left[4];
extern HBITMAP tank2_moving_right[4];
extern HBITMAP tank2_hited_left;
extern HBITMAP tank2_hited_right;
extern HBITMAP tank2_fire_left[4];
extern HBITMAP tank2_fire_right[4];

extern HBITMAP tank3_idle_left;
extern HBITMAP tank3_idle_right;
extern HBITMAP tank3_moving_left[4];
extern HBITMAP tank3_moving_right[4];
extern HBITMAP tank3_hited_left;
extern HBITMAP tank3_hited_right;
extern HBITMAP tank3_fire_left[5];
extern HBITMAP tank3_fire_right[5];

extern HBITMAP item[6];

extern HBITMAP map1_wind[4];
extern HBITMAP map2_wind[8];
extern HBITMAP map3_wind[4];

// ===== 그리기용 HDC (WndProc에서 만들고, 그리기 함수가 씀) =====
extern HDC hBackBuffer;
extern HDC MemDC;

// ===== 게임 함수들 =====
void InsertBitmap(HINSTANCE hInst);

void Draw_tank();
void Draw_skill();
void Draw_shoot();
void Draw_wind();
void Draw_Flare();
void player_UI();
void select_UI();

void physics(HWND hWnd);
void physics_Action(HWND hWnd);
void wind(HWND hWnd);
void make_random();
void player_dead(HWND hWnd);
void mouse_moving(int mouseX, int mouseY);
void reset(HWND hWnd);

void camera_turn();
void Fire_turn();

// 픽셀 단위 충돌 / 파괴
void SetPixelColor(HBITMAP hBitmap, int x, int y, COLORREF color);
COLORREF GetPixelColor(HBITMAP hBitmap, int x, int y);
bool IsShapeCollidingWithBitmap(HBITMAP hBitmap, int shapeX, int shapeY, int shapeWidth, int shapeHeight);
bool IsShapeCollidingWithBitmap_2(HBITMAP hBitmap, int shapeX, int shapeY, int shapeWidth, int shapeHeight);
bool IsBitmapsIntersecting(HBITMAP hBitmap1, int x1, int y1, HBITMAP hBitmap2, int x2, int y2, int width, int height);

// 원도우 그리기 함수
void DrawFrame(HWND hWnd, HDC hDC);
void OnMouseMove(HWND hWnd, LPARAM lParam);
void OnLButtonDown(HWND hWnd, LPARAM lParam);
void OnRButtonDown(HWND hWnd, LPARAM lParam);
void OnTimer(HWND hWnd, WPARAM wParam);
void OnKeyDown(HWND hWnd, WPARAM wParam);

void ApplyTerrainDelta(const PKT_TERRAIN_DELTA& pkt);

extern HWND g_hWnd;
extern HBITMAP g_hTerrainBitmap; // 필요하면 지형 전역 비트맵도 extern 선언