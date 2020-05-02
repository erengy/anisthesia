#include <regex>
#include <string>
#include <vector>

#include <anisthesia/media.hpp>
#include <anisthesia/player.hpp>
#include <anisthesia/util.hpp>

#include <anisthesia/win_platform.hpp>
#include <anisthesia/win_util.hpp>
#include <anisthesia/win_windows.hpp>

namespace anisthesia {
namespace win {

bool IsPlayerWindow(const Process& process, const Window& window,
                    const Player& player) {
  auto check_pattern = [](const std::string& pattern, const std::string& str) {
    if (pattern.empty())
      return false;
    if (pattern.front() == '^' && std::regex_match(str, std::regex(pattern)))
      return true;
    return util::EqualStrings(pattern, str);
  };

  auto check_windows = [&]() {
    for (const auto& pattern : player.windows) {
      if (check_pattern(pattern, ToUtf8String(window.class_name)))
        return true;
    }
    return false;
  };

  auto check_executables = [&]() {
    for (const auto& pattern : player.executables) {
      if (check_pattern(pattern, ToUtf8String(process.name)))
        return true;
    }
    return false;
  };

  return check_windows() && check_executables();
}

////////////////////////////////////////////////////////////////////////////////

bool GetResults(const std::vector<Player>& players, media_proc_t media_proc,
                std::vector<Result>& results) {
  auto window_proc = [&](const Process& process, const Window& window) -> bool {
    for (const auto& player : players) {
      if (IsPlayerWindow(process, window, player)) {
        results.push_back({player, process, window, {}});
        break;
      }
    }
    return true;
  };

  if (!EnumerateWindows(window_proc))
    return false;

  if (!ApplyStrategies(media_proc, results))
    return false;

  return true;
}

}  // namespace win
}  // namespace anisthesia
