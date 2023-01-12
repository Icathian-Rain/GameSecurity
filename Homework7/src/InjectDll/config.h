#pragma once
// 偏移信息
#include <Windows.h>
#include "type.h"
#include "udpClient.h"

// 结构体偏移信息
#define entityListOffset (0x58AC04)
#define viewMatrixOffset (0x57DFD0)
#define playerBaseOffset (0x58AC00)
#define amountOfPlayersOffset (0x0058AC0C)
// 实体结构偏移信息
#define healthOffset (0xEC)
#define locationXOffset (0x28)
#define locationYOffset (0x2C)
#define locationZOffset (0x30)
#define headXOffset (0x4)
#define headYOffset (0x8)
#define headZOffset (0xC)
#define teamOffset (0x30c)
#define yawOffset (0x34)
#define pitchOffset (0x38)
#define nameOffset (0x205)
// 窗口信息

// PI
#define PI (3.14159265358979323846f)

extern UdpClient udpClient;
extern HWND hwndAC_Client;
extern HBRUSH Brush;
extern HDC hdcAC_client;
extern HFONT Font;
extern float Matrix[16];
extern COLORREF TextCOLOR;
extern COLORREF TextCOLORRED;
extern int windowWidth;
extern int windowHeight;

void DrawFilledRect(int x, int y, int w, int h);
void DrawBorderBox(int x, int y, int w, int h, int thickness);
void DrawLine(int targetX, int targetY);
void DrawString(int x, int y, COLORREF color, const char* text);

bool WorldToScreen(Vec3 pos, Vec2& screen, float matrix[16]);
void drawEntity(Entity entity);

bool WorldToScreenWithoutMatrix(Vec2& screen, float yawDiff, float pitchDiff);
void drawEnityWithoutMatrix(Entity entity);

void calCs(Entity player, Vec3 pos, Vec2 &angular, Vec2 &angularDiff);
void refreshEntityTowards(Entity player, Entity &entity);