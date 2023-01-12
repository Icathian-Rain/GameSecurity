#include "pch.h"
#include "config.h"
#include <cmath>

void calCs(Entity player, Vec3 pos, Vec2 &angular, Vec2 &angularDiff)
{
	float x = player.headX, y = player.headY, z = player.headZ;
	float yaw = player.yaw, pitch = player.pitch;
	float x_diff = pos.x - x,  y_diff = pos.y - y, z_diff = pos.z - z;
	float distance = sqrt(x_diff * x_diff + y_diff * y_diff);
	angular.y = (float)((double)atan2(z_diff, distance) * 180.0 / PI);
	if (distance > 0.0)
	{
		angular.x = (float)((double)atan2(x_diff/distance, y_diff/distance) * 180.0 / PI);
		angular.x = fabs(angular.x - 180);
	}
	angularDiff.x = angular.x - yaw;
	angularDiff.y = angular.y - pitch;
}

// 更新实体的朝向
void refreshEntityTowards(Entity player, Entity &entity)
{
	Vec2 angular, angularDiff;
	Vec3 pos = {entity.x, entity.y, entity.z};
	calCs(player, pos, angular, angularDiff);
	entity.JAngular.x = angular.x;
	entity.JAngular.y = angular.y;
	entity.JAngularDifference.x = angularDiff.x;
	entity.JAngularDifference.y = angularDiff.y;
	pos = {entity.headX, entity.headY, entity.headZ};
	calCs(player, pos, angular, angularDiff);
	entity.HAngular.x = angular.x;
	entity.HAngular.y = angular.y;
	entity.HAngularDifference.x = angularDiff.x;
	entity.HAngularDifference.y = angularDiff.y;
}
