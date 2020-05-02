#pragma once

#include <chrono>
#include <cstdint>
#include <string>
#include <vector>

// Specifications for Matroska media containers:
// https://www.matroska.org/technical/specs/index.html

namespace anisthesia::matroska {

namespace detail {

using timecode_scale_t = std::chrono::duration<float, std::nano>;
constexpr uint32_t kDefaultTimecodeScale = 1000000;  // 1 milliseconds

enum ElementId {
  // EBML Header
  kEBML = 0x1A45DFA3,
  // Segment
  kSegment = 0x18538067,
  // Segment Information
  kInfo = 0x1549A966,
  kTimecodeScale = 0x2AD7B1,
  kDuration = 0x4489,
  kTitle = 0x7BA9,
  // Track
  kTracks = 0x1654AE6B,
  kTrackEntry = 0xAE,
  kTrackType = 0x83,
  kTrackName = 0x536E,
};

enum TrackType {
  kVideo = 1,
};

class Buffer {
public:
  Buffer(size_t size);

  uint8_t* data();
  size_t pos() const;
  size_t size() const;
  void skip(size_t size);

  bool read_encoded_value(uint32_t& value, bool clear_leading_bits);
  uint32_t read_uint32(const size_t size);
  float read_float(const size_t size);
  std::string read_string(const size_t size);

private:
  std::vector<uint8_t> data_;
  size_t pos_ = 0;
};

}  // namespace detail

using duration_t = std::chrono::duration<float, std::milli>;

struct Info {
  duration_t duration = duration_t::zero();
  std::string title;
  std::string video_track_name;
};

bool ReadInfoFromFile(const std::string& path, Info& info);

}  // namespace anisthesia::matroska
