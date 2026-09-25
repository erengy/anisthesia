#pragma once

#include <string>
#include <vector>

#include <anisthesia/media.hpp>
#include <anisthesia/player.hpp>

namespace anisthesia::lin {

struct Process {
  int id = 0;
  std::string name;
  std::string comm;
};

struct Result {
  Player player;
  Process process;
  std::vector<Media> media;
};

bool GetResults(const std::vector<Player>& players, media_proc_t media_proc,
                std::vector<Result>& results);

namespace detail {

bool ApplyStrategies(media_proc_t media_proc, std::vector<Result>& results);

}  // namespace detail

}  // namespace anisthesia::lin
