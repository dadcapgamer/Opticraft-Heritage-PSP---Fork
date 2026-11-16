#include "RedstoneUpdateInfo.h"

RedstoneUpdateInfo::RedstoneUpdateInfo()
	: x(0), y(0), z(0), updateTime(0)
{
}

RedstoneUpdateInfo::RedstoneUpdateInfo(int_t i, int_t j, int_t k, long_t l)
{
	x = i;
	y = j;
	z = k;
	updateTime = l;
}
