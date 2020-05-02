#include <string>

#include <windows.h>

#include <anisthesia/win_util.hpp>

namespace anisthesia {
namespace win {

std::wstring GetFileNameFromPath(const std::wstring& path) {
  const auto pos = path.find_last_of(L"/\\");
  return pos != std::wstring::npos ? path.substr(pos + 1) : path;
}

std::wstring GetFileNameWithoutExtension(const std::wstring& filename) {
  const auto pos = filename.find_last_of(L".");
  return pos != std::wstring::npos ? filename.substr(0, pos) : filename;
}

bool IsSystemDirectory(const std::wstring& path) {
  // @TODO: Use %windir% environment variable
  static const std::wstring windir = L"C:\\Windows";
  const size_t pos = path.find_first_not_of(L"\\?");
  return path.substr(pos, windir.size()) == windir;
}

std::string ToUtf8String(const std::wstring& str) {
  auto wide_char_to_multi_byte = [&str](LPSTR output, int size) -> int {
    return ::WideCharToMultiByte(CP_UTF8, 0, str.c_str(), str.size(),
                                 output, size, nullptr, nullptr);
  };

  if (!str.empty()) {
    const auto size = wide_char_to_multi_byte(nullptr, 0);
    if (size > 0) {
      std::string output(size, '\0');
      wide_char_to_multi_byte(&output.front(), size);
      return output;
    }
  }

  return std::string();
}

}  // namespace win
}  // namespace anisthesia
