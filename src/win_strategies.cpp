#include <regex>

#include <anisthesia/media.hpp>

#include <anisthesia/win_open_files.hpp>
#include <anisthesia/win_platform.hpp>
#include <anisthesia/win_smtc.hpp>
#include <anisthesia/win_ui_automation.hpp>
#include <anisthesia/win_util.hpp>

namespace anisthesia::win::detail {

class Strategist {
public:
  Strategist(Result& result, media_proc_t media_proc)
      : result_(result), media_proc_(media_proc) {}

  bool ApplyStrategies();

private:
  bool AddMedia(const MediaInfo media_information);

  bool ApplyWindowTitleStrategy();
  bool ApplyOpenFilesStrategy();
  bool ApplyUiAutomationStrategy();
  bool ApplyMediaControlStrategy();

  media_proc_t media_proc_;
  Result& result_;
};

////////////////////////////////////////////////////////////////////////////////

bool Strategist::ApplyStrategies() {
  bool success = false;

  for (const auto strategy : result_.player.strategies) {
    switch (strategy) {
      case Strategy::WindowTitle:
        success |= ApplyWindowTitleStrategy();
        break;
      case Strategy::OpenFiles:
        success |= ApplyOpenFilesStrategy();
        break;
      case Strategy::UiAutomation:
        success |= ApplyUiAutomationStrategy();
        break;
      case Strategy::MediaControl:
        success |= ApplyMediaControlStrategy();
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

bool ApplyWindowTitleFormat(const std::string& format, std::string& title) {
  if (!format.empty()) {
    const std::regex pattern(format);
    std::smatch match;
    std::regex_match(title, match, pattern);

    // Use the first non-empty match result, because the regular expression may
    // contain multiple sub-expressions.
    for (size_t i = 1; i < match.size(); ++i) {
      if (!match.str(i).empty()) {
        title = match.str(i);
        return true;
      }
    }

    // Results are empty, but the match was successful
    if (!match.empty()) {
      title.clear();
      return true;
    }
  }

  return false;
}

MediaInfoType InferMediaInformationType(const std::string& str) {
  static const std::regex path_pattern(
      R"(^(?:[A-Za-z]:[/\\]|\\\\)[^<>:"/\\|?*]+)");
  if (std::regex_search(str, path_pattern)) {
    return MediaInfoType::File;
  }

  return MediaInfoType::Unknown;
}

bool Strategist::ApplyWindowTitleStrategy() {
  auto title = ToUtf8String(result_.window.text);
  ApplyWindowTitleFormat(result_.player.window_title_format, title);

  return AddMedia({InferMediaInformationType(title), title});
}

bool Strategist::ApplyOpenFilesStrategy() {
  bool success = false;

  auto open_files_proc = [this, &success](const OpenFile& open_file) -> bool {
    success |= AddMedia({MediaInfoType::File, ToUtf8String(open_file.path)});
    return true;
  };

  const std::set<DWORD> process_ids{result_.process.id};
  EnumerateOpenFiles(process_ids, open_files_proc);

  return success;
}

bool Strategist::ApplyUiAutomationStrategy() {
  auto web_browser_proc = [this](
      const WebBrowserInformation& web_browser_information) {
    auto value = ToUtf8String(web_browser_information.value);

    switch (web_browser_information.type) {
      case WebBrowserInformationType::Address:
        AddMedia({MediaInfoType::Url, value});
        break;
      case WebBrowserInformationType::Title:
        ApplyWindowTitleFormat(result_.player.window_title_format, value);
        AddMedia({MediaInfoType::Title, value});
        break;
      case WebBrowserInformationType::Tab:
        AddMedia({MediaInfoType::Tab, value});
        break;
    }
  };

  return GetWebBrowserInformation(result_.window.handle, web_browser_proc);
}

bool Strategist::ApplyMediaControlStrategy() {
  Media media;
  
  if (!GetMediaFromSMTC(media)) {
    return false;
  }

  result_.media.push_back(media);

  return true;
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

}  // namespace anisthesia::win::detail
