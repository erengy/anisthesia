#pragma once

#include <chrono>
#include <functional>
#include <string>
#include <vector>

namespace anisthesia {

using media_time_t = std::chrono::milliseconds;

enum class MediaInformationType {
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

struct MediaInformation {
  MediaInformationType type = MediaInformationType::Unknown;
  std::string value;
};

struct Media {
  MediaState state = MediaState::Unknown;
  media_time_t duration;
  media_time_t position;
  std::vector<MediaInformation> information;
};

using media_proc_t = std::function<bool(const MediaInformation&)>;

}  // namespace anisthesia
