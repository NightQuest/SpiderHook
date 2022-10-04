#pragma once

// System headers
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <errorrep.h>
#include <dwmapi.h>
#include <commctrl.h>

#include <cstdint>
#include <unordered_map>
#include <memory>
#include <thread>
#include <mutex>
#include <regex>

// Dependencies
#include "capstone/capstone.h"

// Engine
#include "Engine/Engine.h"

// Application
#include "Application.h"
#include "SpiderHook.h"
