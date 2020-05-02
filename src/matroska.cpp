#include <fstream>

#include "matroska.h"

namespace anisthesia {
namespace matroska {

Buffer::Buffer(size_t size) {
  data_.resize(size, '\0');
}

uint8_t* Buffer::data() {
  return !data_.empty() ? &data_.front() : nullptr;
}

size_t Buffer::pos() const {
  return pos_;
}

size_t Buffer::size() const {
  return data_.size();
}

void Buffer::skip(size_t size) {
  pos_ += size;
}

bool Buffer::read_encoded_value(uint32_t& value, bool clear_leading_bits) {
  uint8_t base = 0x80;
  size_t size = 0;

  for ( ; size <= 8; ++size) {
    base = 0x80 >> size;
    if (!base)
      return false;
    if (data_[pos_] & base)
      break;
  }

  for (size_t i = 0; i <= size; ++i) {
    const uint8_t b = (clear_leading_bits && i == 0) ? ~base : 0xFF;
    value |= (data_[pos_ + i] & b) << ((size - i) * 8);
  }

  pos_ += size + 1;
  return true;
}

uint32_t Buffer::read_uint32(const size_t size) {
  uint32_t value = 0;
  for (size_t i = 0; i < size; ++i) {
    value |= (data_[pos_ + i] & 0xFF) << ((size - i - 1) * 8);
  }
  pos_ += size;
  return value;
}

float Buffer::read_float(const size_t size) {
  static_assert(sizeof(uint32_t) == sizeof(float), "Invalid float size");
  switch (size) {
    case 4: {
      auto u32 = read_uint32(size);
      return reinterpret_cast<float&>(u32);  // @TODO: Is this safe?
    }
    default:
      // @TODO: Do we need to handle 64-bit values? (i.e. size == 8)
      pos_ += size;
      return 0;
  }
}

std::string Buffer::read_string(const size_t size) {
  auto result = std::string(
      reinterpret_cast<const char*>(&data_.at(pos_)), size);
  pos_ += size;
  return result;
}

////////////////////////////////////////////////////////////////////////////////

bool ReadInfoFromFile(const std::string& path, Info& info) {
  std::ifstream file(path.c_str(), std::ios::in | std::ios::binary);
  if (!file)
    return false;

  // Determine file size
  file.seekg(0, std::ios::end);
  const auto file_size = static_cast<size_t>(file.tellg());
  file.seekg(0, std::ios::beg);

  // Check EBML header
  uint8_t m[sizeof(uint32_t)];
  file.read(reinterpret_cast<char*>(m), sizeof(uint32_t));
  const uint32_t magic = (m[0] << 24) | (m[1] << 16) | (m[2] << 8) | m[3];
  if (magic != ElementId::kEBML)
    return false;  // invalid Matroska file

  uint32_t timecode_scale = kDefaultTimecodeScale;
  uint32_t track_type = 0;

  for (size_t offset = 0; offset < file_size; ) {
    // @TODO: Improve performance. fgetc takes a LOT of time (e.g. 5 seconds for
    // a 1GB file) if the file isn't cached. mkvinfo is much faster.
    Buffer buffer(0x1000);  // arbitrary size
    file.seekg(offset);
    file.read(reinterpret_cast<char*>(buffer.data()), buffer.size());

    uint32_t element_id = 0;
    uint32_t value_size = 0;
    if (!buffer.read_encoded_value(element_id, false) ||
        !buffer.read_encoded_value(value_size, true)) {
      return false;
    }

    switch (element_id) {
      default:
        buffer.skip(value_size);
        break;

      case ElementId::kSegment:
      case ElementId::kInfo:
      case ElementId::kTracks:
      case ElementId::kTrackEntry:
        // We don't want to skip the data of these elements
        break;

      case ElementId::kTimecodeScale:
        timecode_scale = buffer.read_uint32(value_size);
        break;
      case ElementId::kDuration:
        info.duration = std::chrono::duration_cast<duration_t>(
            timecode_scale_t{buffer.read_float(value_size) * timecode_scale});
        break;
      case ElementId::kTitle:
        info.title = buffer.read_string(value_size);
        break;
      case ElementId::kTrackType:
        track_type = buffer.read_uint32(value_size);
        break;
      case ElementId::kTrackName:
        if (track_type == TrackType::kVideo) {
          info.video_track_name = buffer.read_string(value_size);
        } else {
          buffer.skip(value_size);
        }
        break;
    }

    offset += buffer.pos();
  }

  return true;
}

}  // namespace matroska
}  // namespace anisthesia
