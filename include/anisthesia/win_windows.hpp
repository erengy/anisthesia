#pragma once

#include <functional>

namespace anisthesia {
namespace win {

struct Process;
struct Window;

using window_proc_t = std::function<bool(const Process&, const Window&)>;

bool EnumerateWindows(window_proc_t window_proc);

}  // namespace win
}  // namespace anisthesia
