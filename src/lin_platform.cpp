#include <algorithm>
#include <regex>
#include <string>
#include <vector>

#include <anisthesia/media.hpp>
#include <anisthesia/player.hpp>
#include <anisthesia/util.hpp>

#include <anisthesia/lin_platform.hpp>
#include <anisthesia/lin_processes.hpp>

namespace anisthesia::lin {

namespace detail {

// Applications spawn helper processes that share the same executable (e.g.
// `ipc/thumbfast` of mpv, `Web Content` of Firefox). They may keep files open
// long after the player itself is gone, so they must not be mistaken for it.
// The kernel gives each process its own name, which is what tells them apart.
bool IsHelperProcess(const Process& process) {
  if (process.comm.empty())
    return false;

  // The command name is truncated to 15 characters
  const auto name = process.name.substr(0, process.comm.size());
  return !anisthesia::detail::util::EqualStrings(name, process.comm);
}

bool IsPlayerProcess(const Process& process, const Player& player) {
  auto check_pattern = [](const std::string& pattern, const std::string& str) {
    if (pattern.empty())
      return false;
    if (pattern.front() == '^' && std::regex_match(str, std::regex(pattern)))
      return true;
    return anisthesia::detail::util::EqualStrings(pattern, str);
  };

  // Window classes are not available, so we can only check executables
  for (const auto& pattern : player.executables) {
    if (check_pattern(pattern, process.name))
      return true;
  }
  return false;
}

}  // namespace detail

////////////////////////////////////////////////////////////////////////////////

bool GetResults(const std::vector<Player>& players, media_proc_t media_proc,
                std::vector<Result>& results) {
  auto process_proc = [&](const Process& process) -> bool {
    if (detail::IsHelperProcess(process))
      return true;

    for (const auto& player : players) {
      if (detail::IsPlayerProcess(process, player)) {
        results.push_back({player, process, {}});
        break;
      }
    }
    return true;
  };

  if (!detail::EnumerateProcesses(process_proc))
    return false;

  const bool success = detail::ApplyStrategies(media_proc, results);

  // Unlike windows, processes without any media are common (e.g. helper
  // processes of the same application), so they are not worth returning.
  std::erase_if(results, [](const Result& result) {
    return result.media.empty();
  });

  return success;
}

}  // namespace anisthesia::lin
