#include "Player.h"

#ifndef PLAYER_H
#define PLAYER_H

Player::Player()
    :left(0)
    , top(0)
    , right(0)
    , speed(10)
    , start_x(100)
    , start_y(800)
    , power(0)
    , power_Max(30)
    , Speed(0)
    , Speed_Max(30)
    , isSpaceDown(false)
    , isSpacePress(false)
    , isSpaceUp(false)
    , isleftDown(false)
    , isleftPress(false)
    , isleftUp(false)
    , HP(100)
    , HP_MAX(100)
    , power_now(0){}

void Player::set_pos(int p1_left, int p1_top)
{
    left = p1_left;
    top = p1_top;
    right = left + 20;
}

void Player::Space_Down()
{
    if (GetAsyncKeyState(VK_SPACE))
    {
        isSpaceUp = TRUE;
    }
}

void Player::Move(bool isFire, int tank_mode)
{
    if (isFire == true)
        return;
    if (GetAsyncKeyState(VK_LEFT))
    {
        if (tank_mode == 1)
        {
            if (0 < left && Speed < 30)
                left -= 3;
        }
        else if (tank_mode == 3)
        {
            if (0 < left && Speed < 30)
                left -= 1.5;
        }
        else if (tank_mode == 2)
        {
            if (0 < left && Speed < 30)
                left -= 4;
        }
    }
    if (GetAsyncKeyState(VK_RIGHT))
    {
        if (tank_mode == 1)
        {
            if (1920 > left + 20 && Speed < 30)
                left += 3;
        }
        if (tank_mode == 3)
        {
            if (1920 > left + 20 && Speed < 30)
                left += 1.5;
        }
        if (tank_mode == 2)
        {
            if (1920 > left + 20 && Speed < 30)
                left += 4;
        }
    }
}

void Player::Render(HDC hdc)
{
    Ellipse(hdc, left, top, left + 20.0, top + 20.0);
}

void Player::Charge()
{
    if (power < power_Max)
        power += power_Max * 0.01;
    if (power_now < power_Max)
        power_now += power_Max * 0.01;
}

void Player::SpacePress()
{
    Charge();
}

void Player::SpaceUp(HWND hWnd)
{
    isSpaceUp = true;
    power_now = power;
    //SetTimer(hWnd, 6, 60, NULL); // 탱크 1
    //SetTimer(hWnd, 7, 60, NULL); // 탱크 2
    //SetTimer(hWnd, 8, 60, NULL); // 탱크 3
}

void Player::Update(bool isFire, HWND hWnd)
{
    if (isFire == true)
        return;
    if (GetAsyncKeyState(VK_SPACE))
    {
        if (isSpacePress == false) {
            isSpaceDown = true;
            isSpacePress = true;
        }
        if (isSpaceDown == true)
        {
            Charge();
        }
    }
    else
    {
        if (isSpacePress == true && isSpaceDown == true)
        {
            isSpaceDown = false;
            isSpacePress = false;
            SpaceUp(hWnd);
        }
    }
    // ---
    if (GetAsyncKeyState(VK_LEFT) || GetAsyncKeyState(VK_RIGHT))
    {
        if (isleftPress == false) {
            isleftDown = true;
            isleftPress = true;
        }
        if (isleftDown == true)
        {
            if (Speed < Speed_Max)
                Speed += Speed_Max * 0.02;
        }
    }
    else
    {
        if (isleftPress == true && isleftDown == true)
        {
            isleftDown = false;
            isleftPress = false;
        }
    }
}

void Player::Render_PowerGauge(HDC hdc, int camera_x, int camera_y)
{
    // 파워 게이지를 화면에 표시
    int gaugeWidth = 265; // 게이지의 전체 너비
    int gaugeHeight = 13; // 게이지의 높이
    int filledWidth = static_cast<int>((power_now / power_Max) * gaugeWidth); // 충전된 게이지의 너비

    // 게이지 배경
    Rectangle(hdc, camera_x + 224, camera_y + 368, camera_x + 224 + gaugeWidth, camera_y + 368 + gaugeHeight);
    // 충전된 부분
    HBRUSH brush = CreateSolidBrush(RGB(255, 0, 0)); // 홍색 브러시 생성
    HBRUSH oldBrush = (HBRUSH)SelectObject(hdc, brush);
    Rectangle(hdc, camera_x + 224, camera_y + 368, camera_x + 224 + filledWidth, camera_y + 368 + gaugeHeight);
    SelectObject(hdc, oldBrush);
    DeleteObject(brush);
}

void Player::Render_SpeedGauge(HDC hdc, int camera_x, int camera_y)
{
    // 파워 게이지를 화면에 표시
    int gaugeWidth = 265; // 게이지의 전체 너비
    int gaugeHeight = 13; // 게이지의 높이
    int filledWidth = static_cast<int>((Speed / Speed_Max) * gaugeWidth); // 충전된 게이지의 너비

    // 게이지 배경
    Rectangle(hdc, camera_x + 224, camera_y + 383, camera_x + 224 + gaugeWidth, camera_y + 383 + gaugeHeight);
    // 충전된 부분
    HBRUSH brush = CreateSolidBrush(RGB(0, 0, 255)); // 파란색 브러시 생성
    HBRUSH oldBrush = (HBRUSH)SelectObject(hdc, brush);
    Rectangle(hdc, camera_x + 224, camera_y + 383, camera_x + 224 + filledWidth, camera_y + 383 + gaugeHeight);
    SelectObject(hdc, oldBrush);
    DeleteObject(brush);
}

void Player::Render_HP(HDC hdc, int left, int top)
{
    // 파워 게이지를 화면에 표시
    int gaugeWidth = 20; // 게이지의 전체 너비
    int gaugeHeight = 5; // 게이지의 높이
    int filledWidth = static_cast<int>((HP / HP_MAX) * gaugeWidth); // 충전된 게이지의 너비

    // 게이지 배경
    Rectangle(hdc, left, top + 22, left + gaugeWidth, top + 22 + gaugeHeight);
    // 충전된 부분
    HBRUSH brush = CreateSolidBrush(RGB(0, 255, 0)); // 초록색 브러시 생성
    HBRUSH oldBrush = (HBRUSH)SelectObject(hdc, brush);
    Rectangle(hdc, left, top + 22, left + filledWidth, top + 22 + gaugeHeight);
    SelectObject(hdc, oldBrush);
    DeleteObject(brush);
}

#endif PLAYER_H

// ---

#ifndef LINE_H
#define LINE_H

Line::Line()
    :begin_x(0)
    , begin_y(0)
    , end_x(0)
    , end_y(0)
    , angle(90.0)
    , radian(0.0) {}

void Line::set_radian()
{

}

void Line::Render_Line(HDC hdc, int camera_x, int camera_y)
{
    begin_x = 70;
    begin_y = 350;

    double radianAngle = toRadians(angle + 90); // 선 그리기에서의 각도를 라디안으로 변환

    // 포물선 운동의 방향을 나타내는 선을 그립니다.
    MoveToEx(hdc, camera_x+(int)begin_x, camera_y+(int)begin_y, nullptr);
    LineTo(hdc, camera_x + begin_x + (30 * sin(radianAngle)), camera_y + begin_y + (30 * cos(radianAngle))); // Y좌표를 반대로 설정합니다.카메라에 맞춰 수정
}

void Line::Angle(bool isFire)
{
    if (isFire == true)
        return;
    if (GetAsyncKeyState(VK_UP))
    {
        if (angle > 180)
            return;
        angle += 1;
    }
    if (GetAsyncKeyState(VK_DOWN))
    {
        if (angle < 0)
            return;
        angle -= 1;
    }
}

#endif LINE_H

// ---

#ifndef FIRE_H
#define FIRE_H

Fire::Fire()
    :v_0(0)
    , v_x(0)// 파워 값 0 ~ 30 정도가 적당해 보임
    , diameter(5.f)
    , gravity(10)
    , Time(0.1f)
    , z(0.f)
    , x(0)
    , y(0)
    , isFire(false)
    , shoot_mode(0)
    , shoot1(true) 
    , shoot2(true) 
    , shoot3(true) {}

void Fire::set_ball()
{
    if (isFire == false)
    {
        double radianAngle = toRadians(angle + 90);
        x = (left + 8.5) + (20 * sin(radianAngle));
        y = (top + 8.5) + (20 * cos(radianAngle));
    }
}

void Fire::Render_Fire(HDC hdc)
{
    if (isSpaceUp == true)
        isFire = true;
}

void Fire::Action(bool* player_1turn, bool* player_2turn, double* ball_x, double* ball_y, int tank_mode)
{
    if (isFire == false)
    {
        v_x = power + 5;
    }
    if (isFire == true)
    {
        v_0 = power + 5;
        double radianAngle = toRadians(angle);
        // 포물선 운동을 위한 x와 y 좌표 계산
        if (tank_mode == 1)
        {
            gravity = 10;
        }
        else if (tank_mode == 2)
        {
            gravity = 7;
        }
        else if (tank_mode == 3)
        {
            gravity = 15;
        }

        x += v_0 * Time * cos(radianAngle) / 10; // 기존 위치에 더합니다.
        y -= (v_x * Time * sin(radianAngle) - 0.5 * gravity * Time * Time) / 10; // 기존 위치에서 빼줍니다.
        Time += 0.05; // 시간 증가
        *ball_x = x;
        *ball_y = y;
        if (y > 800 || x > 1600 || x < 0) //    화면 밖 나가면 탄 사라짐
        {
            if (*player_1turn == true)
            {
                *player_2turn = true;
                *player_1turn = false;
            }
            else if (*player_2turn == true)
            {
                *player_2turn = false;
                *player_1turn = true;
            }
            Speed = 0;
            power = 0;
            Time = 1;
            power_now = 0;
            isFire = false; isSpaceUp = false;
            set_ball();
            return;
        }
    }
}

void Fire::Hit(bool* player_1turn, bool* player_2turn, double left, double top, double* Player1_HP, double* Player2_HP, int tank_mode)
{
    if (left - 15 < x + 5 && top - 15 < y + 5 &&
        left + 35 > x && top + 35 > y)
    {
        if (*player_1turn == true)
        {
            *player_2turn = true;
            *player_1turn = false;
            if (shoot1 == true && shoot_mode == 1)
            {
                *Player2_HP = *Player2_HP - 50;
                shoot1 = false;
                shoot_mode = 0;
            }
            if (tank_mode == 1)
            {
                *Player2_HP = *Player2_HP - 15;
            }
            else if (tank_mode == 2)
            {
                *Player2_HP = *Player2_HP - 10;
            }
            else if (tank_mode == 3)
            {
                *Player2_HP = *Player2_HP - 20;
            }
            
        }
        else if (*player_2turn == true)
        {
            *player_2turn = false;
            *player_1turn = true;
            if (shoot1 == true && shoot_mode == 1)
            {
                *Player1_HP = *Player1_HP - 50;
                shoot1 = false;
                shoot_mode = 0;
            }
            if (tank_mode == 1)
            {
                *Player1_HP = *Player1_HP - 15;
            }
            else if (tank_mode == 2)
            {
                *Player1_HP = *Player1_HP - 10;
            }
            else if (tank_mode == 3)
            {
                *Player1_HP = *Player1_HP - 20;
            }
        }
        Speed = 0;
        power = 0;
        Time = 1;
        power_now = 0;
        isFire = false; isSpaceUp = false;
        set_ball();
        return;

    }
}

void Fire::shoot_1(bool* player_1turn, bool* player_2turn, double* ball_x, double* ball_y)
{
    if (isFire == true)
    {
        v_0 = power + 5;
        double radianAngle = toRadians(angle);
        // 포물선 운동을 위한 x와 y 좌표 계산
        x += v_0 * Time * cos(radianAngle) / 10; // 기존 위치에 더합니다.
        y -= (v_0 * Time * sin(radianAngle) - 0.5 * gravity * Time * Time) / 10; // 기존 위치에서 빼줍니다.
        Time += 0.05; // 시간 증가
        *ball_x = x;
        *ball_y = y;
        /*double vx = speed * cos(radian);
        double vy = speed * sin(radian);
        const double windSpeed = 5.0;
        double t = 0.0;
        double dt = 0.1;

        for (int i = 0; i < 1000; ++i) {
            double x = (vx * t) + (0.5 * windSpeed * t * t);
            double y = (vy * t) - (0.5 * gravity * t * t);

            t += dt;
        }
        */
        if (y > 840 || x > 1920 || x < 0) //    화면 밖 나가면 탄 사라짐
        {
            if (*player_1turn == true)
            {
                *player_2turn = true;
                *player_1turn = false;
            }
            else if (*player_2turn == true)
            {
                *player_2turn = false;
                *player_1turn = true;
            }
            Speed = 0;
            power = 0;
            Time = 1;
            isFire = false; isSpaceUp = false;
            set_ball();
            return;
        }
    }
}

void Fire::shoot_2(bool* player_1turn, bool* player_2turn, double* ball_x, double* ball_y)
{
    if (shoot_mode == 2)
    {
        set_pos(*ball_x, *ball_y);
        shoot_mode = 0;
    }
}

void Fire::shootmode()
{
    if (GetAsyncKeyState(VK_F1))
    {
        if (shoot1 == false)
        {
            shoot_mode = 0;
            return;
        }
        if (shoot_mode == 1)
        {
            shoot_mode = 0;
            return;
        }
        shoot_mode = 1;
    }
    else if (GetAsyncKeyState(VK_F2))
    {
        if (shoot_mode == 2)
        {
            shoot_mode = 0;
            return;
        }
        shoot_mode = 2;
    }
    else if (GetAsyncKeyState(VK_F3))
    {
        if (shoot_mode == 3)
        {
            shoot_mode = 0;
            return;
        }
        shoot_mode = 3;
    }

    if (shoot_mode == 3)
    {
        if (GetAsyncKeyState(VK_RETURN))
        {
            if (HP < 100)
            {
                HP += 30;
                shoot3 = false;
            }
        }
    }
}

#endif FIRE_H