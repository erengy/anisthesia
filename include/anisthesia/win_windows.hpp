#pragma once

#include <functional>

namespace anisthesia::win {

struct Process;
struct Window;

namespace detail {

using window_proc_t = std::function<bool(const Process&, const Window&)>;

bool EnumerateWindows(window_proc_t window_proc);

}  // namespace detail

}  // namespace anisthesia::win
