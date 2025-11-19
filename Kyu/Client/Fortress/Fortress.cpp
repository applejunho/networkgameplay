// main.cpp
#include <windows.h>
#include <tchar.h>
#include "resource.h"
#include "Player.h"
#include "Game.h"

#pragma comment(lib, "Msimg32.lib")

HINSTANCE g_hInst;
LPCTSTR lpszClass = L"Window Class Name";
LPCTSTR lpszWindowName = L"Fortress";

LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpszCmdParam, int nCmdShow)
{
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

    hWnd = CreateWindow(lpszClass, lpszWindowName, WS_OVERLAPPEDWINDOW, 0, 0, 610, 440, NULL, (HMENU)NULL, hInstance, NULL);

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

        // 특정 키 입력 받을 시 발사 + 패킷 전송
    case WM_KEYUP:
        if (wParam == VK_SPACE)
        {
            if (player_1turn) {
                A.OnSpaceUp();   // 플레이어1 발사 + 서버로 PKT_FIRE
            }
            else if (player_2turn) {
                B.OnSpaceUp();   // 플레이어2 발사 + 서버로 PKT_FIRE
            }
        }
        break;

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
        DeleteObject(hBitmap);
        PostQuitMessage(0);
        break;
    }
    return DefWindowProc(hWnd, uMsg, wParam, lParam);
}
