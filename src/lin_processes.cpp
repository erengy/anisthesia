#include <charconv>
#include <filesystem>
#include <fstream>
#include <string>
#include <system_error>

#include <anisthesia/lin_platform.hpp>
#include <anisthesia/lin_processes.hpp>

namespace anisthesia::lin::detail {

namespace fs = std::filesystem;

bool ParseProcessId(const std::string& str, int& process_id) {
  const auto end = str.data() + str.size();
  const auto [ptr, ec] = std::from_chars(str.data(), end, process_id);
  return ec == std::errc{} && ptr == end;
}

std::string GetCommandName(const fs::path& process_path) {
  std::ifstream file(process_path / "comm");
  std::string name;
  std::getline(file, name);
  return name;
}

std::string GetProcessName(const fs::path& process_path) {
  // The executable path is only readable for our own processes, which is fine
  // since we cannot read the open files of other users' processes either.
  std::error_code ec;
  const auto executable_path = fs::read_symlink(process_path / "exe", ec);
  if (!ec && !executable_path.empty())
    return executable_path.filename().string();

  // Fall back to the command name, which is truncated to 15 characters
  return GetCommandName(process_path);
}

bool EnumerateProcesses(process_proc_t process_proc) {
  std::error_code ec;
  fs::directory_iterator it("/proc", ec);
  if (ec)
    return false;

  for (const auto& entry : it) {
    Process process;
    if (!ParseProcessId(entry.path().filename().string(), process.id))
      continue;

    process.name = GetProcessName(entry.path());
    if (process.name.empty())
      continue;

    process.comm = GetCommandName(entry.path());

    if (!process_proc(process))
      break;
  }

  return true;
}

}  // namespace anisthesia::lin::detail
