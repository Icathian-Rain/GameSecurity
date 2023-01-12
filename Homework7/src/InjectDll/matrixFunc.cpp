#include "pch.h"
#include <iostream>
#include "config.h"

// 将世界坐标转换为屏幕坐标
bool WorldToScreen(Vec3 pos, Vec2 &screen, float matrix[16]) // 3D to 2D
{
    // 通过矩阵计算将世界坐标3D转换为剪辑坐标
    Vec4 clipCoords;
    clipCoords.x = pos.x * matrix[0] + pos.y * matrix[4] + pos.z * matrix[8] + matrix[12];
    clipCoords.y = pos.x * matrix[1] + pos.y * matrix[5] + pos.z * matrix[9] + matrix[13];
    clipCoords.z = pos.x * matrix[2] + pos.y * matrix[6] + pos.z * matrix[10] + matrix[14];
    clipCoords.w = pos.x * matrix[3] + pos.y * matrix[7] + pos.z * matrix[11] + matrix[15];

    if (clipCoords.w < 0.1f)
        return false;

    // 通过对剪辑坐标运算获取NDC坐标
    Vec3 NDC;
    NDC.x = clipCoords.x / clipCoords.w;
    NDC.y = clipCoords.y / clipCoords.w;
    NDC.z = clipCoords.z / clipCoords.w;

    // 将NDC坐标与分辨率运算获取到对应的屏幕2D坐标
    screen.x = (windowWidth / 2 * NDC.x) + (NDC.x + windowWidth / 2);
    screen.y = -(windowHeight / 2 * NDC.y) + (NDC.y + windowHeight / 2);
    return true;
}

void drawEntity(Entity entity)
{
    // 位置向量
    Vec3 enemyPos = {entity.x, entity.y, entity.z};
    // 头部位置向量
    Vec3 enemyHeadPos = {entity.headX, entity.headY, entity.headZ};
    // 玩家窗口
    Vec2 vScreen;
    // 玩家头部
    Vec2 vHead;
    if (WorldToScreen(enemyPos, vScreen, Matrix))
    {
        if (WorldToScreen(enemyHeadPos, vHead, Matrix))
        {
            // 头部高度
            float head = vHead.y - vScreen.y;
            // 宽度
            float width = head / 2;
            // 中心点
            float center = width / -2;
            // 头部上方额外区域
            float extra = head / -6;
            // 设置画刷颜色
            Brush = CreateSolidBrush(RGB(158, 66, 244));
            // 画人物框
            DrawBorderBox(vScreen.x + center, vScreen.y, width, head - extra, 1);
            DeleteObject(Brush);
            // 将人物血量转换为字符串
            char healthChar[255];
            sprintf_s(healthChar, sizeof(healthChar), " %d", (int)(entity.health));
            // 标记人物血量
            DrawString(vScreen.x, vScreen.y, TextCOLOR, healthChar);
            // 画线
            DrawLine(vScreen.x, vScreen.y);
        }
    }
}

bool WorldToScreenWithoutMatrix(Vec2& screen, float yawDiff, float pitchDiff)
{
    // 计算可视角度
    float visibleAngle = (float)((double)atan2((double)windowHeight, (double)windowWidth) * 180.0 / PI);
    // 若不可见则返回false
    if (fabs(yawDiff) > 45 || fabs(pitchDiff) > visibleAngle)
        return false;
    // 计算水平差
    int diff1 = (int)(tan(yawDiff * PI / 180) * (windowWidth / 2));
    // 计算窗口中x坐标
    screen.x = (float)(windowWidth / 2 + diff1);
    // 计算垂直差
    int diff2 = (int)(tan(pitchDiff * PI / 180) * (windowWidth / 2));
    // 计算窗口中y坐标
    screen.y = (float)(windowHeight / 2 + diff2);
    return true;
}


void drawEnityWithoutMatrix(Entity entity)
{
    Vec2 vScreen;
    Vec2 vHead;
    if(WorldToScreenWithoutMatrix(vScreen, entity.JAngularDifference.x, entity.JAngularDifference.y))
    {
        if(WorldToScreenWithoutMatrix(vHead, entity.HAngularDifference.x, entity.HAngularDifference.y))
        {
            // 头部高度
            float head = vHead.y - vScreen.y;
            // 宽度
            float width = head / 2;
            // 中心点
            float center = width / -2;
            // 1/3宽度
            float extra = head / -6;
            // 设置画刷颜色
            Brush = CreateSolidBrush(RGB(158, 66, 244));
            // 画人物框
            DrawBorderBox(vScreen.x + center, vScreen.y, width, head - extra, 1);
            DeleteObject(Brush);
            // 将人物血量转换为字符串
            char healthChar[255];
            sprintf_s(healthChar, sizeof(healthChar), " %d", (int)(entity.health));
            // 标记人物血量
            DrawString(vScreen.x, vScreen.y, TextCOLOR, healthChar);
            // 画线
            DrawLine(vScreen.x, vScreen.y);
        }
    }


}






