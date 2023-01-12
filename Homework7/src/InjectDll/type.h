#pragma once

// 向量结构体
struct Vec3
{
    float x, y, z;
};

struct Vec4
{
    float x, y, z, w;
};

struct Vec2
{
    float x, y;
};

// 实体结构体
struct Entity
{
    float x, y, z;
    float headX, headY, headZ;
    int health;
    int team;
    float yaw;
    float pitch;
    Vec2 JAngular; // 身体角度，x为水平，y为垂直
    Vec2 HAngular; // 头部角度，x为水平，y为垂直
    Vec2 JAngularDifference; // 身体角度差，x为水平，y为垂直
    Vec2 HAngularDifference; // 头部角度差，x为水平，y为垂直
    char name[16];
};



