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

#include <algorithm>
#include <fstream>
#include <string>

#include "util.h"

namespace anisthesia {
namespace util {

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

}  // namespace util
}  // namespace anisthesia
