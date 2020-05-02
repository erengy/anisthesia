#include <map>
#include <sstream>
#include <string>
#include <vector>

#include <anisthesia/player.hpp>
#include <anisthesia/util.hpp>

namespace anisthesia {

namespace detail::parser {

enum class State {
  ExpectPlayerName,
  ExpectSection,
  ExpectWindow,
  ExpectExecutable,
  ExpectStrategy,
  ExpectType,
  ExpectWindowTitle,
};

size_t GetIndentation(const std::string& line) {
  return line.find_first_not_of('\t');
}

bool HandleIndentation(const size_t current,
                       const std::vector<Player>& players,
                       State& state) {
  // Each state has a definitive expected indentation
  const auto expected = [&state]() -> size_t {
    switch (state) {
      default:
      case State::ExpectPlayerName:
        return 0;
      case State::ExpectSection:
        return 1;
      case State::ExpectWindow:
      case State::ExpectExecutable:
      case State::ExpectStrategy:
      case State::ExpectType:
        return 2;
      case State::ExpectWindowTitle:
        return 3;
    }
  }();

  if (current > expected)
    return false;  // Disallow excessive indentation

  if (current < expected) {
    auto fix_state = [&]() {
      state = !current ? State::ExpectPlayerName : State::ExpectSection;
    };
    switch (state) {
      case State::ExpectWindow:
        if (players.back().windows.empty())
          return false;
        fix_state();
        break;
      case State::ExpectExecutable:
        if (players.back().executables.empty())
          return false;
        fix_state();
        break;
      case State::ExpectStrategy:
        if (players.back().strategies.empty())
          return false;
        fix_state();
        break;
      case State::ExpectType:
        fix_state();
        break;
      case State::ExpectWindowTitle:
        return false;
    }
  }

  return true;
}

bool HandleState(std::string& line, std::vector<Player>& players, State& state) {
  switch (state) {
    case State::ExpectPlayerName:
      players.push_back(Player());
      players.back().name = line;
      state = State::ExpectSection;
      break;

    case State::ExpectSection: {
      static const std::map<std::string, State> sections = {
        {"windows", State::ExpectWindow},
        {"executables", State::ExpectExecutable},
        {"strategies", State::ExpectStrategy},
        {"type", State::ExpectType},
      };
      util::TrimRight(line, ":");
      const auto it = sections.find(line);
      if (it == sections.end())
        return false;
      state = it->second;
      break;
    }

    case State::ExpectWindow:
      players.back().windows.push_back(line);
      break;

    case State::ExpectExecutable:
      players.back().executables.push_back(line);
      break;

    case State::ExpectStrategy: {
      static const std::map<std::string, Strategy> strategies = {
        {"window_title", Strategy::WindowTitle},
        {"open_files", Strategy::OpenFiles},
        {"ui_automation", Strategy::UiAutomation},
      };
      util::TrimRight(line, ":");
      const auto it = strategies.find(line);
      if (it == strategies.end())
        return false;
      const auto strategy = it->second;
      players.back().strategies.push_back(strategy);
      switch (strategy) {
        case Strategy::WindowTitle:
          state = State::ExpectWindowTitle;
          break;
      }
      break;
    }

    case State::ExpectType: {
      static const std::map<std::string, PlayerType> types = {
        {"default", PlayerType::Default},
        {"web_browser", PlayerType::WebBrowser},
      };
      const auto it = types.find(line);
      if (it == types.end())
        return false;
      players.back().type = it->second;
      break;
    }

    case State::ExpectWindowTitle:
      players.back().window_title_format = line;
      state = State::ExpectStrategy;
      break;
  }

  return true;
}

}  // namespace detail::parser

////////////////////////////////////////////////////////////////////////////////

bool ParsePlayersData(const std::string& data, std::vector<Player>& players) {
  if (data.empty())
    return false;

  std::istringstream stream(data);
  std::string line;
  size_t indentation = 0;
  auto state = detail::parser::State::ExpectPlayerName;

  while (std::getline(stream, line, '\n')) {
    if (line.empty())
      continue;  // Ignore empty lines

    indentation = detail::parser::GetIndentation(line);

    detail::util::TrimLeft(line, "\t");
    detail::util::TrimRight(line, "\n\r");

    if (line.empty() || line.front() == '#')
      continue;  // Ignore empty lines and comments

    if (!detail::parser::HandleIndentation(indentation, players, state))
      return false;

    if (!detail::parser::HandleState(line, players, state))
      return false;
  }

  return !players.empty();
}

bool ParsePlayersFile(const std::string& path, std::vector<Player>& players) {
  std::string data;

  if (!detail::util::ReadFile(path, data))
    return false;

  return ParsePlayersData(data, players);
}

}  // namespace anisthesia
