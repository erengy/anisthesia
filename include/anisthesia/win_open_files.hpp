#pragma once

#include <functional>
#include <set>
#include <string>

#include <windows.h>

namespace anisthesia {
namespace win {

struct OpenFile {
  DWORD process_id;
  std::wstring path;
};

using open_file_proc_t = std::function<bool(const OpenFile&)>;

bool EnumerateOpenFiles(const std::set<DWORD>& process_ids,
                        open_file_proc_t open_file_proc);

}  // namespace win
}  // namespace anisthesia
