#pragma once

#include <string>
#include <vector>

#include <windows.h>

#include <anisthesia/media.hpp>
#include <anisthesia/player.hpp>

namespace anisthesia {
namespace win {

struct Process {
  DWORD id = 0;
  std::wstring name;
};

struct Window {
  HWND handle = nullptr;
  std::wstring class_name;
  std::wstring text;
};

struct Result {
  Player player;
  Process process;
  Window window;
  std::vector<Media> media;
};

bool ApplyStrategies(media_proc_t media_proc, std::vector<Result>& results);

bool GetResults(const std::vector<Player>& players, media_proc_t media_proc,
                std::vector<Result>& results);

}  // namespace win
}  // namespace anisthesia
