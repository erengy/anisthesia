#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Media.Control.h>

#include <anisthesia/media.hpp>
#include <anisthesia/win_smtc.hpp>
#include <anisthesia/win_util.hpp>
#include <chrono>

// System Media Transport Controls reference:
// https://learn.microsoft.com/en-us/windows/uwp/audio-video-camera/integrate-with-systemmediatransportcontrols
// https://learn.microsoft.com/en-us/uwp/api/windows.media.systemmediatransportcontrols

namespace MediaControl = winrt::Windows::Media::Control;

using PlaybackStatus = MediaControl::GlobalSystemMediaTransportControlsSessionPlaybackStatus;
using SessionManager = MediaControl::GlobalSystemMediaTransportControlsSessionManager;
using winrt::Windows::Foundation::TimeSpan;
using winrt::Windows::Media::MediaPlaybackType;

namespace anisthesia::win::detail {

MediaState ToMediaState(const PlaybackStatus playback_status) {
  // clang-format off
  switch (playback_status) {
    case PlaybackStatus::Playing: return MediaState::Playing;
    case PlaybackStatus::Paused: return MediaState::Paused;
    case PlaybackStatus::Stopped: return MediaState::Stopped;
    default: return MediaState::Unknown;
  }
  // clang-format on
}

std::chrono::milliseconds ToMilliseconds(const TimeSpan& time_span) {
  return std::chrono::duration_cast<std::chrono::milliseconds>(time_span);
}

////////////////////////////////////////////////////////////////////////////////

bool GetMediaFromSMTC(Media& media) {
  winrt::init_apartment(winrt::apartment_type::single_threaded);

  const auto session_manager = SessionManager::RequestAsync().get();
  if (!session_manager) {
    return false;
  }

  const auto current_session = session_manager.GetCurrentSession();
  if (!current_session) {
    return false;
  }

  const auto media_properties = current_session.TryGetMediaPropertiesAsync().get();
  if (!media_properties) {
    return false;
  }
  if (media_properties.PlaybackType().Value() != MediaPlaybackType::Video) {
    return false;
  }

  const auto title = ToUtf8String(media_properties.Title().c_str());
  if (title.empty()) {
    return false;
  }

  media.information.emplace_back(MediaInfoType::Title, title);

  const auto playback_info = current_session.GetPlaybackInfo();
  if (playback_info) {
    media.state = ToMediaState(playback_info.PlaybackStatus());
  }

  const auto timeline_properties = current_session.GetTimelineProperties();
  if (timeline_properties) {
    media.position = ToMilliseconds(timeline_properties.Position());
    media.duration = ToMilliseconds(timeline_properties.EndTime());
  }

  return true;
}

}  // namespace anisthesia::win::detail
