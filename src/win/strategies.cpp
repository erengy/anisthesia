/*
MIT License

Copyright (c) 2017 Eren Okka

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include <regex>

#include "../media.h"

#include "open_files.h"
#include "platform.h"
#include "ui_automation.h"
#include "util.h"

namespace anisthesia {
namespace win {

class Strategist {
public:
  Strategist(Result& result, media_proc_t media_proc)
      : result_(result), media_proc_(media_proc) {}

  bool ApplyStrategies();

private:
  bool AddMedia(const MediaInformation media_information);

  bool ApplyWindowTitleStrategy();
  bool ApplyOpenFilesStrategy();
  bool ApplyUiAutomationStrategy();

  media_proc_t media_proc_;
  Result& result_;
};

////////////////////////////////////////////////////////////////////////////////

bool Strategist::ApplyStrategies() {
  for (const auto strategy : result_.player.strategies) {
    switch (strategy) {
      case Strategy::WindowTitle:
        if (ApplyWindowTitleStrategy())
          return true;
        break;
      case Strategy::OpenFiles:
        if (ApplyOpenFilesStrategy())
          return true;
        break;
      case Strategy::UiAutomation:
        if (ApplyUiAutomationStrategy())
          return true;
        break;
    }
  }

  return false;
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

MediaInformationType InferMediaInformationType(const std::string& str) {
  auto type = MediaInformationType::Unknown;

  static const std::regex local_path_pattern("[A-Z]:[/\\\\].+");
  if (std::regex_match(str, local_path_pattern)) {
    type = MediaInformationType::File;
  }

  return type;
}

bool Strategist::ApplyWindowTitleStrategy() {
  auto title = ToUtf8String(result_.window.text);
  ApplyWindowTitleFormat(result_.player.window_title_format, title);

  return AddMedia({InferMediaInformationType(title), title});
}

bool Strategist::ApplyOpenFilesStrategy() {
  bool success = false;

  auto open_files_proc = [this, &success](const OpenFile& open_file) -> bool {
    success = AddMedia(
        {MediaInformationType::File, ToUtf8String(open_file.path)});
    return !success;
  };

  const std::set<DWORD> process_ids{result_.process.id};
  EnumerateOpenFiles(process_ids, open_files_proc);

  return success;
}

bool Strategist::ApplyUiAutomationStrategy() {
  auto web_browser_proc = [this](
      const WebBrowserInformation& web_browser_information) -> bool {
    auto value = ToUtf8String(web_browser_information.value);

    switch (web_browser_information.type) {
      case WebBrowserInformationType::Address:
        AddMedia({MediaInformationType::Url, value});
        break;
      case WebBrowserInformationType::Title:
        ApplyWindowTitleFormat(result_.player.window_title_format, value);
        AddMedia({MediaInformationType::Title, value});
        break;
      case WebBrowserInformationType::Tab:
        AddMedia({MediaInformationType::Tab, value});
        break;
    }

    return true;
  };

  return GetWebBrowserInformation(result_.window.handle, web_browser_proc);
}

////////////////////////////////////////////////////////////////////////////////

bool Strategist::AddMedia(const MediaInformation media_information) {
  if (media_information.value.empty())
    return false;

  if (!media_proc_(media_information))
    return false;

  Media media;
  media.information.push_back(media_information);
  result_.media.push_back(std::move(media));

  return true;
}

}  // namespace win
}  // namespace anisthesia
