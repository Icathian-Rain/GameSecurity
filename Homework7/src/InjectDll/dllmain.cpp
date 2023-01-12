// dllmain.cpp : 定义 DLL 应用程序的入口点。
#include "pch.h"

#include <Windows.h>
#include <iostream>
#include <string>
#include <cstring>

#include "udpClient.h"
#include "config.h"
#pragma comment(lib, "ws2_32.lib")

// 全局变量
// udp客户端
UdpClient udpClient;
// AC_Client窗口句柄
HWND hwndAC_Client;
// 刷子
HBRUSH Brush;
// hdc
HDC hdcAC_client;
// 字体
HFONT Font;
// 对象矩阵
float Matrix[16];
COLORREF TextCOLOR;
COLORREF TextCOLORRED;

// 窗口信息
int windowWidth;
int windowHeight;

// 玩家基本信息
Entity player;
Entity entityList[100];

// 功能状态
// F1:无敌
bool status1 = false;
bool setHealthOnce = false;
// F2:透视1
bool status2 = false;
// F3:透视2
bool status3 = false;
// F4:自瞄
bool status4 = false;

void init()
{
    // 初始化udp客户端
    udpClient.initSocket();
    // 设置文字颜色
    TextCOLOR = RGB(255, 255, 255);
    // 获取AssaultCube窗口
    hwndAC_Client = FindWindowA(0, ("AssaultCube")); // Gets Window
    // 获取AssaultCube窗口的width
    RECT rect;
    GetWindowRect(hwndAC_Client, &rect);
    windowWidth = rect.right - rect.left;
    windowHeight = rect.bottom - rect.top;
}

// 刷新player信息
void refreshPlayer()
{
    // 获取玩家基本信息
    player.x = *(float *)((DWORD)(*(DWORD *)playerBaseOffset) + locationXOffset);
    player.y = *(float *)((DWORD)(*(DWORD *)playerBaseOffset) + locationYOffset);
    player.z = *(float *)((DWORD)(*(DWORD *)playerBaseOffset) + locationZOffset);
    player.headX = *(float *)((DWORD)(*(DWORD *)playerBaseOffset) + headXOffset);
    player.headY = *(float *)((DWORD)(*(DWORD *)playerBaseOffset) + headYOffset);
    player.headZ = *(float *)((DWORD)(*(DWORD *)playerBaseOffset) + headZOffset);
    player.health = *(DWORD *)((DWORD)(*(DWORD *)playerBaseOffset) + healthOffset);
    player.team = *(DWORD *)((DWORD)(*(DWORD *)playerBaseOffset) + teamOffset);
    player.yaw = *(float *)((DWORD)(*(DWORD *)playerBaseOffset) + yawOffset);
    player.pitch = *(float *)((DWORD)(*(DWORD *)playerBaseOffset) + pitchOffset);
    strcpy_s(player.name, (char *)((DWORD)(*(DWORD *)playerBaseOffset) + nameOffset));
    return;
}
// 刷新实体列表
void refreshEntityList()
{
    // 获取实体列表地址
    DWORD entityListBase = (DWORD)(*(DWORD *)entityListOffset);
    // 获取实体数量
    DWORD amountOfPlayers = *(DWORD *)(amountOfPlayersOffset);
    // 更新实体列表
    for (int i = 1; i < amountOfPlayers; i++)
    {
        DWORD entityBase = *(DWORD *)(entityListBase + 0x4 * i);
        if (entityBase != NULL)
        {
            entityList[i].x = *(float *)(entityBase + locationXOffset);
            entityList[i].y = *(float *)(entityBase + locationYOffset);
            entityList[i].z = *(float *)(entityBase + locationZOffset);
            entityList[i].headX = *(float *)(entityBase + headXOffset);
            entityList[i].headY = *(float *)(entityBase + headYOffset);
            entityList[i].headZ = *(float *)(entityBase + headZOffset);
            entityList[i].health = *(DWORD *)(entityBase + healthOffset);
            entityList[i].team = *(DWORD *)(entityBase + teamOffset);
            strcpy_s(entityList[i].name, (char *)(entityBase + nameOffset));
            refreshEntityTowards(player, entityList[i]);
        }
    }
    return;
}

// 无敌
void func1()
{
    *(DWORD *)((DWORD)(*(DWORD *)playerBaseOffset) + healthOffset) = 0x7FFFFFFF;
}

// 透视1
void func2()
{
    // 获取AC_client画板
    hdcAC_client = GetDC(hwndAC_Client);
    // 获取实体列表地址
    DWORD entityListBase = (DWORD)(*(DWORD *)entityListOffset);
    // 获取实体数量
    DWORD amountOfPlayers = *(DWORD *)(amountOfPlayersOffset);
    // 显示实体位置
    for (int i = 1; i < amountOfPlayers; i++)
    {
        DWORD entityBase = *(DWORD *)(entityListBase + 0x4 * i);
        if (entityBase != NULL)
        {
            
            if (entityList[i].team != player.team)
                drawEntity(entityList[i]);
        }
    }
    // 删除画板
    DeleteObject(hdcAC_client);
}

// 透视2
void func3()
{
    // 获取AC_client画板
    hdcAC_client = GetDC(hwndAC_Client);
    // 获取实体列表地址
    DWORD entityListBase = (DWORD)(*(DWORD *)entityListOffset);
    // 获取实体数量
    DWORD amountOfPlayers = *(DWORD *)(amountOfPlayersOffset);
    // 显示实体位置
    for (int i = 1; i < amountOfPlayers; i++)
    {
        DWORD entityBase = *(DWORD *)(entityListBase + 0x4 * i);
        if (entityBase != NULL)
        {
            if (entityList[i].team != player.team)
                drawEnityWithoutMatrix(entityList[i]);
        }
    }
    // 删除画板
    DeleteObject(hdcAC_client); 
}

// 自瞄
void func4()
{
    // 获取实体列表地址
    DWORD entityListBase = (DWORD)(*(DWORD *)entityListOffset);
    // 获取实体数量
    DWORD amountOfPlayers = *(DWORD *)(amountOfPlayersOffset);
    // 自动瞄准
    Entity *closestEntity = NULL;
    for (int i = 1; i < amountOfPlayers; i++)
    {
        DWORD entityBase = *(DWORD *)(entityListBase + 0x4 * i);
        if (entityBase != NULL)
        {
            // 计算准星角度差
            Entity *entityPtr = &entityList[i];
            float difference = fabs(entityPtr->HAngularDifference.x);
            if (closestEntity != nullptr)
            {
                if (entityPtr->team != player.team && entityPtr->health > 0 && entityPtr->health < 100 && difference < fabs(closestEntity->HAngularDifference.x))
                    closestEntity = entityPtr;
            }
            else
            {
                if (entityPtr->team != player.team && entityPtr->health > 0 && entityPtr->health < 100)
                    closestEntity = entityPtr;
            }
        }
    }
    if (closestEntity != nullptr)
    {
        // 设置玩家yaw
        *(float*)((DWORD)(*(DWORD*)playerBaseOffset) + yawOffset) = closestEntity->HAngular.x;
        // 设置玩家pitch
        *(float *)((DWORD)(*(DWORD *)playerBaseOffset) + pitchOffset) = closestEntity->HAngular.y;
    }
}

void setStatus()
{
    if(GetKeyState(VK_F1) & 1)
    {
        status1 = true;
        setHealthOnce = true;
    }
    else
        status1 = false;
    if(GetKeyState(VK_F2) & 1)
    {
        // 透视1,2互斥
        status3 = false;
        status2 = true;
    }
    else
        status2 = false;
    if(GetKeyState(VK_F3) & 1)
    {
        // 透视1,2互斥
        status2 = false;
        status3 = true;
    }
    else
        status3 = false;
    if(GetKeyState(VK_F4) & 1)
        status4 = true;
    else
        status4 = false;
}

void doFunc()
{
    if (status1)
    {
        func1();
    }
    else
    {
        if (setHealthOnce)
        {
            *(DWORD *)((DWORD)(*(DWORD *)playerBaseOffset) + healthOffset) = 100;
            setHealthOnce = false;
        }
    }
    if (status2)
        func2();
    if (status3)
        func3();
    if (status4 && (GetKeyState(VK_LBUTTON) & 0x8000))
        func4();
}


int mainThread()
{
    init();
    while (true)
    {
        // 刷新玩家信息
        refreshPlayer();
        // 获取视图矩阵
        memcpy(&Matrix, (PBYTE *)(viewMatrixOffset), sizeof(Matrix));
        // 刷新实体列表
        refreshEntityList();
        // 根据按键设置功能
        setStatus();
        // 执行功能
        doFunc();
    }
}

BOOL WINAPI DllMain(HMODULE hModule, DWORD dwReason, LPVOID lpReserved)
{
    if (dwReason == DLL_PROCESS_ATTACH)
    {
        DisableThreadLibraryCalls(hModule);
        CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)mainThread, NULL, NULL, NULL);
    }
    return TRUE;
}
