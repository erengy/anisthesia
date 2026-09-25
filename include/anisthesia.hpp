#pragma once

#include <anisthesia/media.hpp>
#include <anisthesia/player.hpp>

#ifdef _WIN32
#include <anisthesia/win_platform.hpp>
#elif defined(__linux__)
#include <anisthesia/lin_platform.hpp>
#endif
