#pragma once

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX             // Avoid conflict with C++ std::min and std::max
#define STRICT               // Enforce type safety
#define WINVER 0x0A00        // Target Windows 10
#define _WIN32_WINNT 0x0A00  // Match WINVER

#include <windows.h>

#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <format>
#include <functional>
#include <map>
#include <random>
#include <string>
#include <vector>

