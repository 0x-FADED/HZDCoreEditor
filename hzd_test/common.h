#pragma once

#include <stddef.h>
#include <vector>
#include <algorithm>
#include <string>
#include "xutil.h"
#include "ankerl/unordered_dense.h"

enum class GameType
{
	Invalid = 0,
	DeathStranding = 1,
	HorizonZeroDawn = 2,
};