/*
MIT License

Copyright (c) 2017 Eren Okka

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#pragma once

#include <chrono>
#include <cstdint>
#include <string>
#include <vector>

namespace anisthesia {
namespace matroska {

// Specifications for Matroska media containers:
// https://www.matroska.org/technical/specs/index.html

using duration_t = std::chrono::duration<float, std::milli>;
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

struct Info {
  duration_t duration = duration_t::zero();
  std::string title;
  std::string video_track_name;
};

bool ReadInfoFromFile(const std::string& path, Info& info);

}  // namespace matroska
}  // namespace anisthesia
