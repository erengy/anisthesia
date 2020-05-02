#pragma once

#include <string>

namespace anisthesia {
namespace util {

bool ReadFile(const std::string& path, std::string& data);

bool EqualStrings(const std::string& str1, const std::string& str2);
bool TrimLeft(std::string& str, const char* chars);
bool TrimRight(std::string& str, const char* chars);

}  // namespace util
}  // namespace anisthesia
