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

#include "platform.h"
#include "strategies.h"
#include "util.h"

#include "strategy/open_files.h"
#include "strategy/ui_automation.h"
#include "strategy/window_title.h"

#include "../media.h"

namespace anisthesia {
namespace win {

bool AddMedia(MediaInformationType type, const std::string& value,
              Result& result) {
  if (value.empty())
    return false;

  switch (type) {
    case MediaInformationType::File:
      // @TODO: Callback function to verify file
      break;
  }

  Media media;
  media.information.push_back({type, value});
  result.media.push_back(std::move(media));

  return true;
}

////////////////////////////////////////////////////////////////////////////////

bool ApplyWindowTitleStrategy(Result& result) {
  auto title = ToUtf8String(result.window.text);

  if (!result.player.window_title_format.empty()) {
    const std::regex pattern(result.player.window_title_format);
    std::smatch match;
    std::regex_match(title, match, pattern);

    if (match.size() >= 2)
      title = match[1].str();
  }

  return AddMedia(MediaInformationType::Unknown, title, result);
}

bool ApplyOpenFilesStrategy(Result& result) {
  const std::set<DWORD> process_ids{result.window.process_id};

  enum_files_proc_t proc = [&result](const OpenFile& open_file) -> bool {
    AddMedia(MediaInformationType::File, ToUtf8String(open_file.path), result);
    return true;
  };

  return EnumerateFiles(process_ids, proc);
}

bool ApplyUiAutomationStrategy(Result& result) {
  web_browser_information_proc_t proc = [&result](
      WebBrowserInformationType type, const std::wstring& str) -> bool {
    switch (type) {
      case WebBrowserInformationType::Address:
        if (!AddMedia(MediaInformationType::Url, ToUtf8String(str), result))
          return false;
        break;
      case WebBrowserInformationType::Title:
      case WebBrowserInformationType::Tab:
        AddMedia(MediaInformationType::Title, ToUtf8String(str), result);
        break;
    }
    return true;
  };

  return GetWebBrowserInformation(result.window.handle, proc);
}

////////////////////////////////////////////////////////////////////////////////

bool ApplyStrategies(Result& result) {
  for (const auto strategy : result.player.strategies) {
    switch (strategy) {
      case Strategy::WindowTitle:
        if (ApplyWindowTitleStrategy(result))
          return true;
        break;
      case Strategy::OpenFiles:
        if (ApplyOpenFilesStrategy(result))
          return true;
        break;
      case Strategy::UiAutomation:
        if (ApplyUiAutomationStrategy(result))
          return true;
        break;
    }
  }

  return false;
}

bool ApplyStrategies(std::vector<Result>& results) {
  bool success = false;

  for (auto& result : results) {
    success |= ApplyStrategies(result);
  }

  return success;
}

}  // namespace win
}  // namespace anisthesia
