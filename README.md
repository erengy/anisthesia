# Anisthesia

*Anisthesia* is a media detection library for Windows.

- Detects running media players and web browsers
- Retrieves information about the currently playing video

## Usage

***This is a work in progress. Usage in public applications is not yet recommended.***

```cpp
#include <iostream>
#include <vector>
#include <anisthesia.hpp>

int main() {
  std::vector<anisthesia::Player> players;
  if (!anisthesia::ParsePlayersFile("data/players.anisthesia", players)) {
    return 1;
  }

  const auto media_proc = [](const anisthesia::MediaInfo&) {
    return true;  // Accept all media
  };

  std::vector<anisthesia::win::Result> results;
  if (!anisthesia::win::GetResults(players, media_proc, results)) {
    return 1;
  }

  const auto get_type = [](const anisthesia::MediaInfoType& type) {
    switch (type) {
      case anisthesia::MediaInfoType::File: return "File";
      case anisthesia::MediaInfoType::Tab: return "Tab";
      case anisthesia::MediaInfoType::Title: return "Title";
      case anisthesia::MediaInfoType::Url: return "URL";
      default: return "Other";
    }
  };

  for (const auto& result : results) {
    std::cout << result.player.name << '\n';
    for (const auto& media : result.media) {
      for (const auto& information : media.information) {
        std::cout << "\t" << get_type(information.type);
        std::cout << "\t\"" << information.value << "\"\n";
      }
    }
  }

  return 0;
}
```

## License

Licensed under the [MIT License](https://opensource.org/licenses/MIT).
