#define WIN32_LEAN_AND_MEAN
#define _WINSOCKAPI_

// main.cpp
#include <windows.h>
#include <tchar.h>
#include "resource.h"
#include "Player.h"
#include "Game.h"
#include "Packet.h"
#include "ClientNet.h"

HWND g_hWnd = nullptr;

#pragma comment(lib, "Msimg32.lib")

HINSTANCE g_hInst;
LPCTSTR lpszClass = L"Window Class Name";
LPCTSTR lpszWindowName = L"Fortress";

LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpszCmdParam, int nCmdShow)
{
    if (!InitNetwork("127.0.0.1", 9000)) {
        MessageBox(NULL, L"서버 접속에 실패했습니다. 오프라인 모드로 진행합니다.", L"Fortress", MB_OK | MB_ICONWARNING);
    }
    HWND hWnd;
    MSG Message;
    WNDCLASSEX WndClass;

    g_hInst = hInstance;
    WndClass.cbSize = sizeof(WndClass);
    WndClass.style = CS_HREDRAW | CS_VREDRAW;
    WndClass.lpfnWndProc = (WNDPROC)WndProc;
    WndClass.cbClsExtra = 0;
    WndClass.cbWndExtra = 0;
    WndClass.hInstance = hInstance;
    WndClass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    WndClass.hCursor = LoadCursor(NULL, IDC_ARROW);
    WndClass.hbrBackground = (HBRUSH)GetStockObject(GRAY_BRUSH);
    WndClass.lpszMenuName = NULL;
    WndClass.lpszClassName = lpszClass;
    WndClass.hIconSm = LoadIcon(NULL, IDI_APPLICATION);

    RegisterClassEx(&WndClass);

    RECT windowRect = { 0, 0, 1920, 900 };
    AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, FALSE);

    int windowWidth = windowRect.right - windowRect.left;
    int windowHeight = windowRect.bottom - windowRect.top;

    hWnd = CreateWindow(lpszClass, lpszWindowName, WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, windowWidth, windowHeight,
        NULL, (HMENU)NULL, hInstance, NULL);

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    while (GetMessage(&Message, 0, 0, 0)) {
        TranslateMessage(&Message);
        DispatchMessage(&Message);
    }

    return Message.wParam;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    static PAINTSTRUCT ps;
    static HBITMAP hBackBufferBitmap;
    static HDC hDC;

    static int mouseX = 0;
    static int mouseY = 0;

    switch (uMsg) {
    case WM_CREATE:
        // 캐릭터/맵 선택 화면 비트맵 로딩
        InsertBitmap(g_hInst);
        CMSelect = LoadBitmap(g_hInst, MAKEINTRESOURCE(IDB_BITMAP114));
        UI = LoadBitmap(g_hInst, MAKEINTRESOURCE(IDB_BITMAP4));
        A.playerIndex = 0;
        B.playerIndex = 1;
        break;

    case WM_PAINT:
    {
        hDC = BeginPaint(hWnd, &ps);

        DrawFrame(hWnd, hDC);

        EndPaint(hWnd, &ps);
        return 0;
    }
    case WM_MOUSEMOVE:
        OnMouseMove(hWnd, lParam);
        return 0;

    case WM_LBUTTONDOWN:
        OnLButtonDown(hWnd, lParam);
        return 0;

    case WM_RBUTTONDOWN:
        OnRButtonDown(hWnd, lParam);
        return 0;

    case WM_TIMER:
        OnTimer(hWnd, wParam);
        return 0;

    case WM_KEYDOWN:
        OnKeyDown(hWnd, wParam);
        return 0;

    case WM_KEYUP:
        if (wParam == VK_SPACE)
        {
            if (player_1turn && CanControlPlayer(0))
                A.OnSpaceUp(0);
            else if (player_2turn && CanControlPlayer(1))
                B.OnSpaceUp(1);
        }
        return 0;

    case WM_CHAR:
        if (wParam == 'w')
        {
            if (camera_mode == true)
            {
                if (camera_y == 0)
                {
                    break;
                }
                camera_y -= 10;
            }
        }
        else if (wParam == 's')
        {
            if (camera_mode == true)
            {
                if (camera_y == 400)
                {
                    break;
                }
                camera_y += 10;
            }
        }
        else if (wParam == 'a')
        {
            if (camera_mode == true)
            {
                if (camera_x == 0)
                {
                    break;
                }
                camera_x -= 10;
            }
        }
        else if (wParam == 'd')
        {
            if (camera_mode == true)
            {
                if (camera_x == 1000)
                {
                    break;
                }
                camera_x += 10;
            }
        }
        else if (wParam == 'e')
        {
            if (camera_mode == true)
            {
                camera_mode = false;
            }
            else if (camera_mode == false)
            {
                camera_mode = true;
            }
        }
        break;

    case WM_ERASEBKGND:
        return 1;

    case WM_DESTROY:
        if (hBackBufferBitmap)
            DeleteObject(hBackBufferBitmap);
        CloseNetwork();
        PostQuitMessage(0);
        break;
    }
    return DefWindowProc(hWnd, uMsg, wParam, lParam);
}