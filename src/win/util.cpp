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

#include <string>

#include "util.h"

namespace anisthesia {
namespace win {

std::wstring GetFileNameFromPath(const std::wstring& path) {
  const auto pos = path.find_last_of(L"/\\");
  return pos != std::wstring::npos ? path.substr(pos + 1) : path;
}

bool IsSystemDirectory(const std::wstring& path) {
  // @TODO: Use %windir% environment variable
  static const std::wstring windir = L"C:\\Windows";
  const size_t pos = path.find_first_not_of(L"\\?");
  return path.substr(pos, windir.size()) == windir;
}

}  // namespace win
}  // namespace anisthesia
