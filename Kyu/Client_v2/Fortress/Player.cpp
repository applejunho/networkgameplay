#include "Player.h"
#include "ClientNet.h"

// ========================== Player ===========================

Player::Player()
    : left(0)
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
    , power_now(0)
    , playerIndex(-1)
    , spaceLock(false) // <-- 추가: 기본 false로 초기화
{
}

void Player::set_pos(int p1_left, int p1_top)
{
    left = p1_left;
    top = p1_top;
    right = left + 20;
}

//void Player::Space_Down()
//{
//    if (GetAsyncKeyState(VK_SPACE))
//    {
//        isSpaceUp = TRUE;
//    }
//}

void Player::Move(bool isFire, int tank_mode)
{
    if (GetAsyncKeyState(VK_LEFT))
    {
        if (tank_mode == 1)
        {
            if (0 < left)
                left -= 3;
        }
        else if (tank_mode == 3)
        {
            if (0 < left)
                left -= 1.5;
        }
        else if (tank_mode == 2)
        {
            if (0 < left)
                left -= 4;
        }
    }

    if (GetAsyncKeyState(VK_RIGHT))
    {
        if (tank_mode == 1)
        {
            if (1920 > left + 20)
                left += 3;
        }
        if (tank_mode == 3)
        {
            if (1920 > left + 20)
                left += 1.5;
        }
        if (tank_mode == 2)
        {
            if (1920 > left + 20)
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
}

void Player::Update(bool isFire, HWND hWnd)
{
    if (isFire)
        return;

    if (spaceLock)
    {
        const SHORT spaceState = GetAsyncKeyState(VK_SPACE);
        if ((spaceState & 0x8000) == 0)
        {
            spaceLock = false;
        }
        else
        {
            return;
        }
    }


    // 스페이스(발사 버튼) 처리
    if (GetAsyncKeyState(VK_SPACE))
    {
        if (!isSpacePress)
        {
            isSpaceDown = true;
            isSpacePress = true;
        }

        if (isSpaceDown)
        {
            Charge();
        }
    }
    else
    {
        if (isSpacePress && isSpaceDown)
        {
            isSpaceDown = false;
            isSpacePress = false;
            SpaceUp(hWnd);
        }
    }

    // 좌우 이동 키에 따른 속도 게이지
    if (GetAsyncKeyState(VK_LEFT) || GetAsyncKeyState(VK_RIGHT))
    {
        if (!isleftPress)
        {
            isleftDown = true;
            isleftPress = true;
        }

        if (isleftDown)
        {
            if (Speed < Speed_Max)
                Speed += Speed_Max * 0.02;
        }
    }
    else
    {
        if (isleftPress && isleftDown)
        {
            isleftDown = false;
            isleftPress = false;
        }
    }
}

void Player::Render_PowerGauge(HDC hdc, int camera_x, int camera_y)
{
    const int gaugeWidth = 265;
    const int gaugeHeight = 13;

    int filledWidth = static_cast<int>((power_now / power_Max) * gaugeWidth);

    // 게이지 배경
    Rectangle(hdc,
        camera_x + 224, camera_y + 368,
        camera_x + 224 + gaugeWidth, camera_y + 368 + gaugeHeight);

    // 충전된 부분
    HBRUSH brush = CreateSolidBrush(RGB(255, 0, 0));
    HBRUSH oldBrush = (HBRUSH)SelectObject(hdc, brush);

    Rectangle(hdc,
        camera_x + 224, camera_y + 368,
        camera_x + 224 + filledWidth, camera_y + 368 + gaugeHeight);

    SelectObject(hdc, oldBrush);
    DeleteObject(brush);
}

void Player::Render_SpeedGauge(HDC hdc, int camera_x, int camera_y)
{
    const int gaugeWidth = 265;
    const int gaugeHeight = 13;

    int filledWidth = static_cast<int>((Speed / Speed_Max) * gaugeWidth);

    // 게이지 배경
    Rectangle(hdc,
        camera_x + 224, camera_y + 383,
        camera_x + 224 + gaugeWidth, camera_y + 383 + gaugeHeight);

    // 충전된 부분
    HBRUSH brush = CreateSolidBrush(RGB(0, 0, 255));
    HBRUSH oldBrush = (HBRUSH)SelectObject(hdc, brush);

    Rectangle(hdc,
        camera_x + 224, camera_y + 383,
        camera_x + 224 + filledWidth, camera_y + 383 + gaugeHeight);

    SelectObject(hdc, oldBrush);
    DeleteObject(brush);
}

void Player::Render_HP(HDC hdc, int left, int top)
{
    const int gaugeWidth = 20;
    const int gaugeHeight = 5;

    int filledWidth = static_cast<int>((HP / HP_MAX) * gaugeWidth);

    // 게이지 배경
    Rectangle(hdc,
        left, top + 22,
        left + gaugeWidth, top + 22 + gaugeHeight);

    // 충전된 부분
    HBRUSH brush = CreateSolidBrush(RGB(0, 255, 0));
    HBRUSH oldBrush = (HBRUSH)SelectObject(hdc, brush);

    Rectangle(hdc,
        left, top + 22,
        left + filledWidth, top + 22 + gaugeHeight);

    SelectObject(hdc, oldBrush);
    DeleteObject(brush);
}

// =========================== Line ===========================

Line::Line()
    : begin_x(0)
    , begin_y(0)
    , end_x(0)
    , end_y(0)
    , angle(90.0)
    , radian(0.0)
{
}

void Line::set_radian()
{
    // 현재는 사용 안 함 (각도는 toRadians로 변환)
}

void Line::Render_Line(HDC hdc, int camera_x, int camera_y)
{
    begin_x = 70;
    begin_y = 350;

    double radianAngle = toRadians(angle + 90); // 각도를 라디안으로 변환

    MoveToEx(hdc,
        camera_x + static_cast<int>(begin_x),
        camera_y + static_cast<int>(begin_y),
        nullptr);

    LineTo(hdc,
        camera_x + static_cast<int>(begin_x + (30 * sin(radianAngle))),
        camera_y + static_cast<int>(begin_y + (30 * cos(radianAngle))));
}

void Line::Angle(bool isFire)
{
    if (isFire)
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

// =========================== Fire ===========================

Fire::Fire()
    : v_0(0)
    , v_x(0)      // 파워 값 0 ~ 30 정도
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
    , shoot3(true)
    , spaceLock(false)
{
}


void Fire::ResetGauge()
{
    const bool wasFiring = isFire;

    Speed = 0;
    power = 0;
    power_now = 0;

    Time = 1;
    isFire = false;
    isSpaceUp = false;
    spaceLock = true;

    isSpaceDown = false;
    isSpacePress = false;

    shoot_mode = 0;

    set_ball();
    if (wasFiring && playerIndex >= 0 && CanControlPlayer(playerIndex))
    {
        SendPlayerState(playerIndex, true);
    }
}

void Fire::set_ball()
{
    if (!isFire)
    {
        double radianAngle = toRadians(angle + 90);
        x = (left + 8.5) + (20 * sin(radianAngle));
        y = (top + 8.5) + (20 * cos(radianAngle));
    }
}

void Fire::Render_Fire(HDC hdc)
{
    if (isSpaceUp && !isFire)
    {
        isFire = true;
        isSpaceUp = false;   // ★ 트리거는 한 번 쓰고 바로 끈다
        spaceLock = true;
    }
}

void Fire::Action(double* ball_x, double* ball_y, int tank_mode)
{
    if (!isFire)
    {
        v_x = power + 5;
    }

    if (isFire)
    {
        v_0 = power + 5;
        double radianAngle = toRadians(angle);

        // 탱크 타입에 따른 중력 값
        if (tank_mode == 1)
            gravity = 10;
        else if (tank_mode == 2)
            gravity = 7;
        else if (tank_mode == 3)
            gravity = 15;

        // 포물선 운동
        x += v_0 * Time * cos(radianAngle) / 10;
        y -= (v_x * Time * sin(radianAngle) - 0.5 * gravity * Time * Time) / 10;

        Time += 0.05;

        *ball_x = x;
        *ball_y = y;

        // 화면 밖으로 나가면 "그 플레이어의 탄만" 리셋
        if (y > 800 || x > 1600 || x < 0)
        {
            ResetGauge();
            set_ball();
            if (playerIndex >= 0 && CanControlPlayer(playerIndex))
                SendPlayerState(playerIndex, true);
            return;
        }
    }
}

void Fire::Hit(double left, double top,
    double* targetHP,
    int tank_mode)
{
    if (left - 15 < x + 5 && top - 15 < y + 5 &&
        left + 35 > x && top + 35 > y)
    {
        // 특수탄(슈퍼탄) 처리
        if (shoot1 && shoot_mode == 1)
        {
            *targetHP -= 50;
            shoot1 = false;
            shoot_mode = 0;
        }

        // 탱크 타입에 따른 기본 데미지
        if (tank_mode == 1)
            *targetHP -= 15;
        else if (tank_mode == 2)
            *targetHP -= 10;
        else if (tank_mode == 3)
            *targetHP -= 20;

        // 턴 전환 삭제, 탄만 리셋
        ResetGauge();
        set_ball();
        return;
    }
}

void Fire::shoot_1(bool* player_1turn, bool* player_2turn,
    double* ball_x, double* ball_y)
{
    if (isFire)
    {
        v_0 = power + 5;
        double radianAngle = toRadians(angle);

        x += v_0 * Time * cos(radianAngle) / 10;
        y -= (v_0 * Time * sin(radianAngle) - 0.5 * gravity * Time * Time) / 10;

        Time += 0.05;

        *ball_x = x;
        *ball_y = y;

        if (y > 840 || x > 1920 || x < 0)
        {
            Speed = 0;
            power = 0;
            Time = 1;
            isFire = false;
            isSpaceUp = false;

            set_ball();
            return;
        }
    }
}

void Fire::shoot_2(bool* player_1turn, bool* player_2turn,
    double* ball_x, double* ball_y)
{
    if (shoot_mode == 2)
    {
        set_pos(static_cast<int>(*ball_x), static_cast<int>(*ball_y));
        shoot_mode = 0;
    }
}

void Fire::OnSpaceUp(int playerId)
{
    if (isFire)
        return;

    if (spaceLock)
    {
        // 이전 발사 이후 첫 번째 키 업은 잠금 해제만 수행
        spaceLock = false;
        return;
    }

    playerIndex = playerId;
    isSpaceUp = true;
    set_ball();
    SendFirePacket(playerId, static_cast<float>(left), static_cast<float>(top),
        static_cast<float>(angle), static_cast<float>(power), shoot_mode);
    SendPlayerState(playerId);
}

void Fire::shootmode()
{
    if (GetAsyncKeyState(VK_F1))
    {
        if (!shoot1)
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