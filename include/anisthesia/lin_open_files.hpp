#pragma once

#include <functional>
#include <string>

namespace anisthesia::lin::detail {

struct OpenFile {
  int process_id;
  std::string path;
};

using open_file_proc_t = std::function<bool(const OpenFile&)>;

bool EnumerateOpenFiles(int process_id, open_file_proc_t open_file_proc);

}  // namespace anisthesia::lin::detail
