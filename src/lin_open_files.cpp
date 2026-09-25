#include <filesystem>
#include <string>
#include <string_view>
#include <system_error>

#include <anisthesia/lin_open_files.hpp>

namespace anisthesia::lin::detail {

namespace fs = std::filesystem;

bool IsSystemDirectory(const std::string& path) {
  static constexpr std::string_view directories[] = {
      "/dev/", "/proc/", "/sys/", "/run/", "/usr/", "/etc/", "/var/lib/",
  };

  for (const auto directory : directories) {
    if (path.starts_with(directory))
      return true;
  }

  return false;
}

bool VerifyPath(const fs::path& fd_path, const std::string& path) {
  // Skip sockets, pipes and anonymous files (e.g. "socket:[1234]")
  if (!path.starts_with('/'))
    return false;

  if (IsSystemDirectory(path))
    return false;

  // Files that were removed while open have this suffix
  if (path.ends_with(" (deleted)"))
    return false;

  // Following the descriptor rather than the path also works for files whose
  // path is not visible from our mount namespace
  std::error_code ec;
  return fs::is_regular_file(fd_path, ec);
}

bool EnumerateOpenFiles(int process_id, open_file_proc_t open_file_proc) {
  const fs::path fd_directory =
      fs::path("/proc") / std::to_string(process_id) / "fd";

  std::error_code ec;
  fs::directory_iterator it(fd_directory, ec);
  if (ec)
    return false;

  for (const auto& entry : it) {
    const auto path = fs::read_symlink(entry.path(), ec).string();
    if (ec || !VerifyPath(entry.path(), path))
      continue;

    if (!open_file_proc({process_id, path}))
      return false;
  }

  return true;
}

}  // namespace anisthesia::lin::detail
