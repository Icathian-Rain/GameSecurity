#include "pch.h"
#include <Windows.h>
#include "config.h"

void DrawFilledRect(int x, int y, int w, int h)
{
    RECT rect = { x, y, x + w, y + h };
    FillRect(hdcAC_client, &rect, Brush);
}


void DrawBorderBox(int x, int y, int w, int h, int thickness)
{
    DrawFilledRect(x, y, w, thickness);
    DrawFilledRect(x, y, thickness, h);
    DrawFilledRect((x + w), y, thickness, h);
    DrawFilledRect(x, y + h, w + thickness, thickness);
}



void DrawLine(int targetX, int targetY)
{
    MoveToEx(hdcAC_client, 960, 1080, NULL);
    LineTo(hdcAC_client, targetX, targetY);
}


void DrawString(int x, int y, COLORREF color, const char* text)
{
    SetTextAlign(hdcAC_client, TA_CENTER | TA_NOUPDATECP);

    SetBkColor(hdcAC_client, RGB(0, 0, 0));
    SetBkMode(hdcAC_client, TRANSPARENT);

    SetTextColor(hdcAC_client, color);

    SelectObject(hdcAC_client, Font);

    TextOutA(hdcAC_client, x, y, text, strlen(text));

    DeleteObject(Font);
}