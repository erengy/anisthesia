#include <algorithm>
#include <fstream>
#include <string>

#include <anisthesia/util.hpp>

namespace anisthesia::detail::util {

bool ReadFile(const std::string& path, std::string& data) {
  std::ifstream file(path.c_str(), std::ios::in | std::ios::binary);

  if (!file)
    return false;

  file.seekg(0, std::ios::end);
  data.resize(static_cast<size_t>(file.tellg()));
  file.seekg(0, std::ios::beg);

  file.read(&data.front(), data.size());
  file.close();

  return true;
}

bool EqualStrings(const std::string& str1, const std::string& str2) {
  auto lower_char = [](const char c) -> char {
    return ('A' <= c && c <= 'Z') ? c + ('a' - 'A') : c;
  };

  auto equal_chars = [&lower_char](const char c1, const char c2) -> bool {
    return lower_char(c1) == lower_char(c2);
  };

  return str1.size() == str2.size() &&
         std::equal(str1.begin(), str1.end(), str2.begin(), equal_chars);
}

bool TrimLeft(std::string& str, const char* chars) {
  if (str.empty())
    return false;

  const auto found = str.find_first_not_of(chars);

  if (found == 0)
    return false;

  if (found == std::string::npos) {
    str.clear();
  } else {
    str.erase(0, found);
  }

  return true;
}

bool TrimRight(std::string& str, const char* chars) {
  if (str.empty())
    return false;

  const auto found = str.find_last_not_of(chars);

  if (found == str.size() - 1)
    return false;

  if (found == std::string::npos) {
    str.clear();
  } else {
    str.resize(found + 1);
  }

  return true;
}

}  // namespace anisthesia::detail::util
