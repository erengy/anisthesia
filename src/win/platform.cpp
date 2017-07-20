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

#include <regex>
#include <string>

#include "platform.h"
#include "util.h"

#include "../util.h"

namespace anisthesia {
namespace win {

bool IsPlayerWindow(const Window& window, const Player& player) {
  auto check_pattern = [](const std::string& pattern, const std::string& str) {
    if (pattern.empty())
      return false;
    if (pattern.front() == '^' && std::regex_match(str, std::regex(pattern)))
      return true;
    return util::EqualStrings(pattern, str);
  };

  auto check_windows = [&]() {
    for (const auto& pattern : player.windows) {
      if (check_pattern(pattern, ToUtf8String(window.class_name)))
        return true;
    }
    return false;
  };

  auto check_executables = [&]() {
    for (const auto& pattern : player.executables) {
      if (check_pattern(pattern, ToUtf8String(window.process_file_name)))
        return true;
    }
    return false;
  };

  return check_windows() && check_executables();
}

}  // namespace win
}  // namespace anisthesia
