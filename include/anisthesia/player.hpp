#pragma once

#include <string>
#include <vector>

namespace anisthesia {

enum class Strategy {
  WindowTitle,
  OpenFiles,
  UiAutomation,
};

enum class PlayerType {
  Default,
  WebBrowser,
};

struct Player {
  PlayerType type = PlayerType::Default;
  std::string name;
  std::string window_title_format;
  std::vector<std::string> windows;
  std::vector<std::string> executables;
  std::vector<Strategy> strategies;
};

bool ParsePlayersData(const std::string& data, std::vector<Player>& players);
bool ParsePlayersFile(const std::string& path, std::vector<Player>& players);

}  // namespace anisthesia
