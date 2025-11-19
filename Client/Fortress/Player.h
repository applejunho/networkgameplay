#pragma once
#pragma once
#include <Windows.h>
#include <math.h>

#define PI 3.141519

class Player
{
public:
	double left;
	double top;
	double right;
	double speed;
	double angle;
	int start_x;
	int start_y;
	double HP;
	double HP_MAX;
	double power;
	int power_Max;
	double Speed;
	int Speed_Max;
	bool isSpaceDown;
	bool isSpacePress;
	bool isSpaceUp;
	bool isleftDown;
	bool isleftPress;
	bool isleftUp;
	double power_now;
public:
	Player();
	void Move(bool isFire, int tank_mode);
	void Render(HDC hdc);
	void set_pos(int p1_left, int p1_top);
	void Charge();
	void Space_Down();
	void SpacePress();
	void SpaceUp(HWND hWnd);
	void Render_PowerGauge(HDC hdc,int camera_x,int camera_y);
	void Update(bool isFire, HWND hWnd);
	void Render_SpeedGauge(HDC hdc,  int camera_x, int camera_y);
	void Render_HP(HDC hdc, int left, int top);
};

// ---

class Line : public Player
{
public:
	double begin_x;
	double begin_y;
	double end_x;
	double end_y;
	double angle;
	double radian;
public:
	Line();
	void set_radian();
	void Render_Line(HDC hdc, int camera_x, int camera_y);
	void Angle(bool isFire);
	double toRadians(double degrees)
	{
		return degrees * (PI / 180.0);
	}
};

// ---

class Fire : public Line
{
public:
	int shoot_mode;
	double diameter;
	double v_0;
	double v_x;
	double gravity;
	double Time;
	double z;
	double x;
	double y;
	bool isFire;
	bool shoot1;
	bool shoot2;
	bool shoot3;
	double toRadians(double degrees) { return degrees * 3.14159265358979323846 / 180.0; }
public:
	Fire();
	void set_ball();
	void Render_Fire(HDC hdc);
	void Action(bool* player_1turn, bool* player_2turn, double* ball_x, double* ball_y, int tank_mode);
	void Hit(bool* player_1turn, bool* player_2turn, double left, double top, double* Player1_HP, double* Player2_HP, int tank_mode);
	void shootmode();
	void shoot_1(bool* player_1turn, bool* player_2turn, double* ball_x, double* ball_y);
	void shoot_2(bool* player_1turn, bool* player_2turn, double* ball_x, double* ball_y);
};
