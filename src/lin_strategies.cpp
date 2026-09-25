#include <anisthesia/media.hpp>

#include <anisthesia/lin_open_files.hpp>
#include <anisthesia/lin_platform.hpp>

namespace anisthesia::lin::detail {

class Strategist {
public:
  Strategist(Result& result, media_proc_t media_proc)
      : result_(result), media_proc_(media_proc) {}

  bool ApplyStrategies();

private:
  bool AddMedia(const MediaInfo media_information);

  bool ApplyOpenFilesStrategy();

  Result& result_;
  media_proc_t media_proc_;
};

////////////////////////////////////////////////////////////////////////////////

bool Strategist::ApplyStrategies() {
  bool success = false;

  for (const auto strategy : result_.player.strategies) {
    switch (strategy) {
      case Strategy::OpenFiles:
        success |= ApplyOpenFilesStrategy();
        break;
      // There is no generic way to read window titles on Wayland, and web
      // browsers are expected to be handled via MPRIS by the application.
      case Strategy::WindowTitle:
      case Strategy::UiAutomation:
        break;
    }
  }

  return success;
}

bool ApplyStrategies(media_proc_t media_proc, std::vector<Result>& results) {
  bool success = false;

  for (auto& result : results) {
    Strategist strategist(result, media_proc);
    success |= strategist.ApplyStrategies();
  }

  return success;
}

////////////////////////////////////////////////////////////////////////////////

bool Strategist::ApplyOpenFilesStrategy() {
  bool success = false;

  auto open_files_proc = [this, &success](const OpenFile& open_file) -> bool {
    success |= AddMedia({MediaInfoType::File, open_file.path});
    return true;
  };

  EnumerateOpenFiles(result_.process.id, open_files_proc);

  return success;
}

////////////////////////////////////////////////////////////////////////////////

bool Strategist::AddMedia(const MediaInfo media_information) {
  if (media_information.value.empty())
    return false;

  if (!media_proc_(media_information))
    return false;

  Media media;
  media.information.push_back(media_information);
  result_.media.push_back(std::move(media));

  return true;
}

}  // namespace anisthesia::lin::detail
