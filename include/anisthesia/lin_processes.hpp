#pragma once

#include <functional>

namespace anisthesia::lin {

struct Process;

namespace detail {

using process_proc_t = std::function<bool(const Process&)>;

bool EnumerateProcesses(process_proc_t process_proc);

}  // namespace detail

}  // namespace anisthesia::lin
