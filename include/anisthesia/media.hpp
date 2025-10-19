#pragma once

#include <chrono>
#include <functional>
#include <string>
#include <vector>

namespace anisthesia {

using media_time_t = std::chrono::milliseconds;

enum class MediaInfoType {
  Unknown,
  File,
  Tab,
  Title,
  Url,
};

enum class MediaState {
  Unknown,
  Playing,
  Paused,
  Stopped,
};

struct MediaInfo {
  MediaInfoType type = MediaInfoType::Unknown;
  std::string value;
};

struct Media {
  MediaState state = MediaState::Unknown;
  media_time_t duration;
  media_time_t position;
  std::vector<MediaInfo> information;
};

using media_proc_t = std::function<bool(const MediaInfo&)>;

}  // namespace anisthesia
