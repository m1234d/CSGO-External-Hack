#include "ProcMem.h";
#include "SendKeys.h";
#define WIN32_LEAN_AND_MEAN
#define _WIN32_WINNT 0x0500
#define KEY9 0x20 
#define KEY9SC 0x39 
#define DBOUT( s )            \
{                             \
	std::ostringstream os_;    \
	os_ << s;                   \
	OutputDebugString(os_.str().c_str());  \
}
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <conio.h>
#include <string.h>
#include <windows.h>
#include <thread> 
#include <ctime>
using namespace std;
ProcMem m;
DWORD ClientD;
DWORD EngineD;
HDC const dc = GetDC(0);
bool wallToggle = true;
clock_t start;
clock_t stop;
//Offsets
//update these
const DWORD playerB = 0x00AAAAB4; //dwLocalPlayer signature
const DWORD entityB = 0x04A8574C;  //dwEntityList signature
const DWORD crosshairOff = 0xB2A4; //NetVars m_iCrosshairId offset
const DWORD vecOrigin = 0x134; //NetVars m_vecOrigin
const DWORD viewAng = 0x4D10; //SetViewAngles
const DWORD engPoint = 0x57F84C; //EnginePointer
const DWORD boneMatrix = 0x2698; //NetVars m_dwBoneMatrix offset
const DWORD vecView = 0x104; //NetVars m_vecViewOffset
const DWORD flagOffset = 0x100;//NetVars m_fFlags
//not these
const DWORD teamOff = 0xF0;
const DWORD healthOff = 0xFC;
const DWORD entLoop = 0x10;
//

//vector classes
class Vector3 {
public:
	float x, y, z;
	Vector3() {
		x = 0;
		y = 0;
		z = 0;
	}
	~Vector3() {

	}
	Vector3(float xxx, float yyy, float zzz) {
		x = xxx;
		y = yyy;
		z = zzz;
	}
};
class Vector2 {
public:
	float x, y;
	Vector2() {
		x = 0;
		y = 0;
	}
	~Vector2() {

	}
	Vector2(float xxx, float yyy) {
		x = xxx;
		y = yyy;
	}
};

//helper methods
Vector3 VectorSubtract(Vector3 a, Vector3 b) {
	float row1 = a.x - b.x;
	float row2 = a.y - b.y;
	float row3 = a.z - b.z;
	Vector3 v;
	v.x = row1;
	v.y = row2;
	v.z = row3;
	return v;
}
Vector3 VectorAdd(Vector3 a, Vector3 b) {
	float row1 = a.x + b.x;
	float row2 = a.y + b.y;
	float row3 = a.z + b.z;
	Vector3 v;
	v.x = row1;
	v.y = row2;
	v.z = row3;
	return v;
}
float DotProduct(Vector3 a, Vector3 b) {
	float row1 = a.x * b.x;
	float row2 = a.y * b.y;
	float row3 = a.z * b.z;
	return row1 + row2 + row3;
}
double Distance(double dX0, double dY0, double dX1, double dY1)
{
	return sqrt((dX1 - dX0)*(dX1 - dX0) + (dY1 - dY0)*(dY1 - dY0));
}
double Distance(double dX0, double dY0, double dZ0, double dX1, double dY1, double dZ1)
{
	return sqrt((dX1 - dX0)*(dX1 - dX0) + (dY1 - dY0)*(dY1 - dY0) + (dZ1 - dZ0)*(dZ1 - dZ0));
}

//get coordinates of bone id in the bone matrix
Vector3 GetBonePosition(int bone, DWORD BoneBase)
{
	Vector3 vecBones;

	vecBones.x = m.Read<float>(BoneBase + 0x30 * bone + 0xC);
	vecBones.y = m.Read<float>(BoneBase + 0x30 * bone + 0x1C);
	vecBones.z = m.Read<float>(BoneBase + 0x30 * bone + 0x2C);
	return vecBones;
}

//convert positions of players to angle
Vector3 CalcAngle(Vector3 playerP, Vector3 enemyP, Vector3 playerA) {
	//use two triangles, one for x-angle and one for y-angle
	//x-y coord
	Vector3 playerPos = playerP;
	Vector3 enemyPos = enemyP;
	Vector3 playerAngles = playerA;
	float tempAngle;
	float targetAngle;
	float playerAngle = playerAngles.y;
	float rightAngle = 90;
	//first, decide whether the x-axis will be 0 or 180
	float offset = 180;
	float playerQuadrant;
	float enemyQuadrant;
	float distance = Distance(playerPos.x, playerPos.y, enemyPos.x, enemyPos.y);
	float xDistance = enemyPos.x - playerPos.x;


	tempAngle = xDistance*sin(rightAngle*3.14159265 / 180) / distance;
	tempAngle = asin(tempAngle);
	tempAngle = tempAngle * 180 / 3.14159265;
	if (playerAngles.y >= 0) {
		if (playerAngles.y >= 90) {
			playerQuadrant = 4;
			playerAngle = 450 - abs(playerAngles.y);
		}
		else {
			playerQuadrant = 1;
			playerAngle = 90 - abs(playerAngles.y);
		}
	}
	else {
		if (playerAngles.y <= -90) {
			playerQuadrant = 3;
			playerAngle = abs(playerAngles.y) + 90;
		}
		else {
			playerQuadrant = 2;
			playerAngle = abs(playerAngles.y) + 90;
		}
	}

	if (enemyPos.x - playerPos.x >= 0) {
		if (enemyPos.y - playerPos.y >= 0) {
			enemyQuadrant = 1;
		}
		else {
			enemyQuadrant = 2;
			tempAngle = 180 - abs(tempAngle);
		}
	}
	else {
		if (enemyPos.y - playerPos.y >= 0) {
			enemyQuadrant = 4;
			tempAngle = 360 - abs(tempAngle);
		}
		else {
			enemyQuadrant = 3;
			tempAngle = abs(tempAngle) + 180;
		}
	}
	targetAngle = 0;
	if (playerQuadrant == 1 && enemyQuadrant == 1) {

		if (tempAngle > playerAngle) {
			targetAngle = tempAngle - playerAngle - 0;
			targetAngle = -targetAngle;
		}
		else {
			targetAngle = playerAngle - tempAngle - 0;
		}
	}
	else if (playerQuadrant == 2 && enemyQuadrant == 2) {
		if (tempAngle > playerAngle) {
			targetAngle = tempAngle - playerAngle - 0;
			targetAngle = -targetAngle;
		}
		else {
			targetAngle = playerAngle - tempAngle - 0;
		}
	}
	else if (playerQuadrant == 3 && enemyQuadrant == 3) {
		if (tempAngle > playerAngle) {
			targetAngle = tempAngle - playerAngle - 0;
			targetAngle = -targetAngle;
		}
		else {
			targetAngle = playerAngle - tempAngle - 0;
		}
	}
	else if (playerQuadrant == 4 && enemyQuadrant == 4) {
		if (tempAngle > playerAngle) {
			targetAngle = tempAngle - playerAngle - 0;
			targetAngle = -targetAngle;
		}
		else {
			targetAngle = playerAngle - tempAngle - 0;
		}
	}
	else if (playerQuadrant == 1 && enemyQuadrant == 2) {
		if (tempAngle > playerAngle) {
			targetAngle = tempAngle - playerAngle - 0;
		}
		else {
			targetAngle = playerAngle - tempAngle - 0;
		}
		targetAngle = -targetAngle;
	}
	else if (playerQuadrant == 2 && enemyQuadrant == 3) {
		if (tempAngle > playerAngle) {
			targetAngle = tempAngle - playerAngle - 0;
		}
		else {
			targetAngle = playerAngle - tempAngle - 0;
		}
		targetAngle = -targetAngle;
	}
	else if (playerQuadrant == 3 && enemyQuadrant == 4) {
		if (tempAngle > playerAngle) {
			targetAngle = tempAngle - playerAngle - 0;		}
		else {
			targetAngle = playerAngle - tempAngle - 0;
		}
		targetAngle = -targetAngle;
	}
	else if (playerQuadrant == 4 && enemyQuadrant == 1) {
		if (tempAngle > playerAngle) {
			targetAngle = tempAngle - playerAngle - 0;
		}
		else {
			targetAngle = playerAngle - tempAngle - 360;
			targetAngle = abs(targetAngle);
		}
		targetAngle = -targetAngle;
	}
	else if (playerQuadrant == 2 && enemyQuadrant == 1) {
		if (tempAngle > playerAngle) {
			targetAngle = tempAngle - playerAngle - 0;
		}
		else {
			targetAngle = playerAngle - tempAngle - 0;
		}
	}
	else if (playerQuadrant == 3 && enemyQuadrant == 2) {
		if (tempAngle > playerAngle) {
			targetAngle = tempAngle - playerAngle - 0;
		}
		else {
			targetAngle = playerAngle - tempAngle - 0;
		}
	}
	else if (playerQuadrant == 4 && enemyQuadrant == 3) {
		if (tempAngle > playerAngle) {
			targetAngle = tempAngle - playerAngle - 0;
		}
		else {
			targetAngle = playerAngle - tempAngle - 0;
		}
	}
	else if (playerQuadrant == 1 && enemyQuadrant == 4) {
		if (tempAngle > playerAngle) {
			targetAngle = tempAngle - playerAngle - 0;
			targetAngle = 360 - targetAngle;
		}
		else {
			targetAngle = playerAngle - tempAngle - 0;
		}
	}
	else {
		return Vector3(500, 500, 0);
	}

	//
	//z-coord
	float zTargetAngle = 0;
	xDistance = Distance(playerPos.x, playerPos.y, enemyPos.x, enemyPos.y);
	distance = Distance(playerPos.x, playerPos.y, playerPos.z + 65, enemyPos.x, enemyPos.y, enemyPos.z);
	tempAngle = xDistance*sin(rightAngle*3.14159265 / 180) / distance;
	tempAngle = asin(tempAngle);
	tempAngle = tempAngle * 180 / 3.14159265;
	tempAngle = 90 - tempAngle;
	if (enemyPos.z < playerPos.z + 65) {
		tempAngle = -tempAngle;
	}
	zTargetAngle = tempAngle + playerAngles.x;
	//cout << zTargetAngle;
	//
	Vector3 finalAngle;
	finalAngle.x = targetAngle;
	finalAngle.y = zTargetAngle;
	return finalAngle;
}

//draw point on screen
void DrawEnemy(Vector3 angles, Vector3 playerP, Vector3 enemyP) {
	Vector2 point;
	point.x = 15 * (-angles.x) + 960;
	point.y = 15 * (-angles.y) + 540;
	float distance = Distance(playerP.x, playerP.y, enemyP.x, enemyP.y);
	if (point.x + (10000 / (distance + 50)) > 1920) {
		return;
	}
	Ellipse(dc, point.x - (10000 / (distance + 50)), point.y - (10000 / (distance + 50)), point.x + 10000 / (distance + 50), point.y + 10000 / (distance + 50));
}

//get enemy with the closest angle under 5 degrees
int GetClosestEnemy(Vector3 loc,int locTeam) {
	int distance = 100000000000;
	int lowestAngle = 5;
	int id = 0;
	int team = 0;
	for (int i = 0; i < 12; i++) {
		DWORD detectedEnemy = m.Read<DWORD>(ClientD + entityB + (i * entLoop));
		int detectedEnemyTeam = m.Read<int>(detectedEnemy + teamOff);
		int detectedEnemyHP = m.Read<int>(detectedEnemy + healthOff);
		if (locTeam != detectedEnemyTeam && detectedEnemyHP > 0) {
			DWORD enemyBones = m.Read<DWORD>(detectedEnemy + boneMatrix);
			Vector3 enemyLoc = GetBonePosition(8, enemyBones);
			DWORD dwTemp = m.Read<DWORD>(EngineD + engPoint);
			Vector3 angles = m.Read<Vector3>(dwTemp + viewAng);
			Vector3 targetAngles = CalcAngle(loc, enemyLoc, angles);
			if (abs(targetAngles.x) < lowestAngle && abs(targetAngles.y) < lowestAngle) {
				lowestAngle = abs(targetAngles.x);
				id = i;
				team = detectedEnemyTeam;
			}
		}
		else if (locTeam == detectedEnemyTeam) {
		}
	}
	return id;
}

//draw radar with enemy locations
void DrawRadar(Vector3 *EnemyLocations) {
	//setup radar
	for (int r = 0; r < 24; r++) {
		for (int c = 0; c < 50; c++) {
			if (r == 0) {
				cout << "-";
				continue;
			}
			else if (c == 0) {
				cout << "|";
				continue;
			}
			else if (r == 23) {
				cout << "-";
				continue;
			}
			else if (c == 49) {
				cout << "|";
				continue;
			}
			else {
				bool t = false;
				for (int i = 0; i < sizeof(EnemyLocations); i++) {
					int rr = EnemyLocations[i].x / 200 + 12;
					int cc = EnemyLocations[i].y / 100 + 25;
					int px = 0 / 200 + 12;
					int py = 0 / 200 + 25;
					//DBOUT(to_string(rr) + "\n");
					//DBOUT(to_string(cc) + "\n" + "\n");
					if (r == rr && c == cc) {
						cout << "/";
						t = true;
						break;
					}
					else if (r == px && c == py) {
						cout << "O";
						t = true;
						break;
					}
				}
				if (t == false) {
					cout << " ";
				}
			}
		}
		cout << "\n";
	}
	
}

//triggerbot
void trigB()
{
	DWORD locPlayer = m.Read<DWORD>(playerB + ClientD);
	Vector3 loc = m.Read<Vector3>(locPlayer + vecOrigin);
	
	int locTeam = m.Read<int>(locPlayer + teamOff);
	int crosshairId = m.Read<int>(locPlayer + crosshairOff);
	DWORD detectedEnemy = m.Read<DWORD>(ClientD + entityB + ((crosshairId - 1) * entLoop));
	int detectedEnemyHP = m.Read<int>(detectedEnemy + healthOff);
	int detectedEnemyTeam = m.Read<int>(detectedEnemy + teamOff);
	if (locTeam != detectedEnemyTeam)
	{
		if (detectedEnemyHP > 0) {
			//mouse_event(MOUSEEVENTF_RIGHTDOWN, NULL, NULL, NULL, NULL);
			//Sleep(100);
			mouse_event(MOUSEEVENTF_LEFTDOWN, NULL, NULL, NULL, NULL);
			Sleep(10);
			//delay for spray
			mouse_event(MOUSEEVENTF_LEFTUP, NULL, NULL, NULL, NULL);
			//mouse_event(MOUSEEVENTF_RIGHTUP, NULL, NULL, NULL, NULL);
			//delay before next burst
		}
	}
	cout << crosshairId - 1;
}

//aimbot
void aimB()
{
	DWORD locPlayer = m.Read<DWORD>(playerB + ClientD);
	Vector3 loc = m.Read<Vector3>(locPlayer + vecOrigin);

	int locTeam = m.Read<int>(locPlayer + teamOff);
	int crosshairId = m.Read<int>(locPlayer + crosshairOff);
	int id = GetClosestEnemy(loc, locTeam);
	//int id = crosshairId - 1;
	DWORD detectedEnemy = m.Read<DWORD>(ClientD + entityB + (id * entLoop));
	//Vector3 enemyLoc = m.Read<Vector3>(detectedEnemy + vecOrigin);
	DWORD enemyBones = m.Read<DWORD>(detectedEnemy + boneMatrix);
	//enemyLoc = VectorAdd(GetBonePosition(6, enemyBones), enemyLoc);
	Vector3 enemyLoc = GetBonePosition(8, enemyBones);
	int detectedEnemyHP = m.Read<int>(detectedEnemy + healthOff);
	int detectedEnemyTeam = m.Read<int>(detectedEnemy + teamOff);
	//cout << detectedEnemyTeam;
	//cout << "\n";
	DWORD dwTemp = m.Read<DWORD>(EngineD + engPoint);
	Vector3 angles = m.Read<Vector3>(dwTemp + viewAng);
	if (locTeam != detectedEnemyTeam && detectedEnemyHP > 0)
	{	
		if ((GetKeyState('E') & 0x100) == 0) {
			return;
		}
		Vector3 targetAngles = CalcAngle(loc, enemyLoc, angles);
		Sleep(2);
		POINT p;
		GetCursorPos(&p);
		cout << p.x;
		cout << targetAngles.y;
		cout << targetAngles.x;
		cout << '\n';
		if (targetAngles.x > 2) {
			SetCursorPos(p.x - 20, p.y);
		}
		else if (targetAngles.x > .05) {
			SetCursorPos(p.x - 5, p.y);
		}
		else if (targetAngles.x < -2) {
			SetCursorPos(p.x + 20, p.y);
		}

		else if (targetAngles.x < -.05) {
			SetCursorPos(p.x + 5, p.y);
		}
		if (targetAngles.y > 2) {
			SetCursorPos(p.x, p.y - 20);
		}
		else if (targetAngles.y > .2) {
			SetCursorPos(p.x, p.y - 5);
		}
		else if (targetAngles.y < -2) {
			SetCursorPos(p.x, p.y + 20);
		}

		else if (targetAngles.y < -.2) {
			SetCursorPos(p.x, p.y + 5);
		}
		//mouse_event(MOUSEEVENTF_MOVE | MOUSEEVENTF_ABSOLUTE, targetAngles.y, 0, 0, 0);
		/*if (detectedEnemyHP > 0) {
			//mouse_event(MOUSEEVENTF_RIGHTDOWN, NULL, NULL, NULL, NULL);
			//Sleep(100);
			mouse_event(MOUSEEVENTF_LEFTDOWN, NULL, NULL, NULL, NULL);
			Sleep(10);
			//delay for spray
			mouse_event(MOUSEEVENTF_LEFTUP, NULL, NULL, NULL, NULL);
			//mouse_event(MOUSEEVENTF_RIGHTUP, NULL, NULL, NULL, NULL);
			//delay before next burst
		}*/
	}
}

//wallhck
void wallB() {
	DWORD locPlayer = m.Read<DWORD>(playerB + ClientD);
	Vector3 loc = m.Read<Vector3>(locPlayer + vecOrigin);
	int locTeam = m.Read<int>(locPlayer + teamOff);
	if (GetKeyState('Q') & 0x8000) {
		wallToggle = !wallToggle;
		Sleep(200);
	}
	for (int i = 0; i < 22; i++) {
		DWORD detectedEnemy = m.Read<DWORD>(ClientD + entityB + (i * entLoop));
		int detectedEnemyTeam = m.Read<int>(detectedEnemy + teamOff);
		int detectedEnemyHP = m.Read<int>(detectedEnemy + healthOff);
		if (locTeam != detectedEnemyTeam && detectedEnemyHP > 0) {
			DWORD enemyBones = m.Read<DWORD>(detectedEnemy + boneMatrix);
			Vector3 enemyLoc = GetBonePosition(8, enemyBones); //get head location
			DWORD dwTemp = m.Read<DWORD>(EngineD + engPoint);
			Vector3 angles = m.Read<Vector3>(dwTemp + viewAng);
			Vector3 targetAngles = CalcAngle(loc, enemyLoc, angles);
			if (wallToggle) {
				DrawEnemy(targetAngles, loc, enemyLoc);
			}
		}
		else if (locTeam == detectedEnemyTeam) {
		}
	}
	Sleep(5);
}

//bhop
void bhop() {
	DWORD locPlayer = m.Read<DWORD>(playerB + ClientD);
	bool onGround = m.Read<bool>(locPlayer + flagOffset);
	
	if ((GetAsyncKeyState('9') & (1 << 16)) && onGround) {
		keybd_event(KEY9, KEY9SC, 0, 0);
		Sleep(10);
		keybd_event(KEY9, KEY9SC, KEYEVENTF_KEYUP, 0); 
	}
}

//radar (in console)
void radarB() {
	while (true) {
		DWORD locPlayer = m.Read<DWORD>(playerB + ClientD);
		Vector3 loc = m.Read<Vector3>(locPlayer + vecOrigin);
		stop = clock();
		double elapsed_secs = double(stop - start) / CLOCKS_PER_SEC;
		if (elapsed_secs < 1) {
			continue;
		}
		//reset radar
		Vector3 EnemyLocations[10];
		//loop through all enemies
		int q = 0;
		for (int i = 0; i < 12; i++) {
			//retrieve enemy position and player position
			DWORD locPlayer = m.Read<DWORD>(playerB + ClientD);
			int locTeam = m.Read<int>(locPlayer + teamOff);
			Vector3 loc = m.Read<Vector3>(locPlayer + vecOrigin);
			DWORD detectedEnemy = m.Read<DWORD>(ClientD + entityB + (i * entLoop));
			DWORD enemyBones = m.Read<DWORD>(detectedEnemy + boneMatrix);
			Vector3 enemyLoc = GetBonePosition(6, enemyBones);
			int detectedEnemyHP = m.Read<int>(detectedEnemy + healthOff);
			int detectedEnemyTeam = m.Read<int>(detectedEnemy + teamOff);
			//check if detected entity is an alive enemy
			if (locTeam != detectedEnemyTeam && detectedEnemyHP > 0)
			{
				//find enemy's relative location to player
				enemyLoc = VectorSubtract(enemyLoc, loc);
				//create and draw to a radar system (potentially use ASCII? lol)
				EnemyLocations[q] = enemyLoc;
				q++;
				//10ms delay?
				Sleep(10);
			}
		}
		DrawRadar(EnemyLocations);
		start = clock();
	}
}

//main Make sure CSGO is running before you run the program
int main() {
	start = clock();
	m.Process("csgo.exe");
	ClientD = m.Module("client.dll");
	EngineD = m.Module("engine.dll");
	////Insert Thread Methods Here////
		//thread t1(radarB);
	////						  ////
	while (1 == 1) {
		////Insert Normal Methods Here////
			bhop();
			wallB();
			aimB();
		////						  ////
	}
}
