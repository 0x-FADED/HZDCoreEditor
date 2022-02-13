#pragma once

#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <stddef.h>
#include <vector>
#include <algorithm>
#include <string>
#include <robin_hood_hashing/include/robin_hood.h>

#include "xutil.h"

enum class GameType
{
	Invalid = 0,
	DeathStranding = 1,
	HorizonZeroDawn = 2,
};