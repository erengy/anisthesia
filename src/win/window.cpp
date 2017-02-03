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

#include <set>
#include <string>
#include <vector>

#include <windows.h>

#include "util.h"
#include "window.h"

namespace anisthesia {
namespace win {

std::wstring GetWindowClass(HWND hwnd) {
  WCHAR buffer[MAX_PATH];
  ::GetClassName(hwnd, buffer, MAX_PATH);
  return buffer;
}

std::wstring GetWindowTitle(HWND hwnd) {
  WCHAR buffer[MAX_PATH];
  ::GetWindowText(hwnd, buffer, MAX_PATH);
  return buffer;
}

DWORD GetWindowProcessId(HWND hwnd) {
  DWORD process_id = 0;
  ::GetWindowThreadProcessId(hwnd, &process_id);
  return process_id;
}

std::wstring GetProcessFileName(DWORD process_id) {
  // If we try to open a SYSTEM process, this function fails and the last error
  // code is ERROR_ACCESS_DENIED.
  //
  // Note that if we requested PROCESS_QUERY_INFORMATION access right instead
  // of PROCESS_QUERY_LIMITED_INFORMATION, this function would fail when used
  // to open an elevated process.
  Handle process_handle(::OpenProcess(
      PROCESS_QUERY_LIMITED_INFORMATION, FALSE, process_id));

  if (!process_handle)
    return std::wstring();

  DWORD buffer_size = MAX_PATH;
  WCHAR buffer[MAX_PATH];

  // Note that this function requires Windows Vista or above. You may use
  // GetProcessImageFileName or GetModuleFileNameEx on earlier versions.
  if (!::QueryFullProcessImageName(process_handle.get(), 0,
                                   buffer, &buffer_size)) {
    return std::wstring();
  }

  const auto result = std::wstring(buffer, buffer_size);
  return result.substr(result.find_last_of(L"/\\") + 1);
}

////////////////////////////////////////////////////////////////////////////////

bool VerifyFileName(const std::wstring& name) {
  static const std::set<std::wstring> invalid_names = {
    // System files
    L"explorer.exe",    // Windows Explorer
    L"taskeng.exe",     // Task Scheduler Engine
    L"taskhost.exe",    // Host Process for Windows Tasks
    L"taskhostex.exe",  // Host Process for Windows Tasks
    L"Taskmgr.exe",     // Task Manager

    // These applications are very common and/or they have too many windows
    L"Avira.Systray.exe",
    L"CCC.exe",  // Catalyst Control Center
    L"Photoshop.exe",
    L"Spotify.exe",
    L"Steam.exe",
  };

  return !name.empty() && !invalid_names.count(name);
}

bool VerifyWindowClass(const std::wstring& name) {
  static const std::set<std::wstring> invalid_names = {
    // System classes
    L"#32770",         // Dialog box
    L"CabinetWClass",  // Windows Explorer
    L"ComboLBox",
    L"DDEMLEvent",
    L"DDEMLMom",
    L"GDI+ Hook Window Class",
    L"IME",
    L"MSCTFIME UI",
    L"tooltips_class32",
  };

  return !name.empty() && !invalid_names.count(name);
}

bool VerifyWindowStyle(HWND hwnd) {
  const auto window_ex_style = ::GetWindowLong(hwnd, GWL_EXSTYLE);

  auto has_ex_style = [&window_ex_style](DWORD ex_style) {
    return (window_ex_style & ex_style) != 0;
  };

  if (has_ex_style(WS_EX_TOPMOST) && has_ex_style(WS_EX_TOOLWINDOW))
    return false;

  return true;
}

////////////////////////////////////////////////////////////////////////////////

BOOL CALLBACK EnumerateWindowsProc(HWND hwnd, LPARAM param) {
  if (!VerifyWindowStyle(hwnd))
    return TRUE;

  Window window;
  window.handle = hwnd;
  window.process_id = GetWindowProcessId(hwnd);
  window.title = GetWindowTitle(hwnd);

  window.class_name = GetWindowClass(hwnd);
  if (!VerifyWindowClass(window.class_name))
    return TRUE;

  window.file_name = GetProcessFileName(window.process_id);
  if (!VerifyFileName(window.file_name))
    return TRUE;

  auto& windows = *reinterpret_cast<std::vector<Window>*>(param);
  windows.push_back(window);

  return TRUE;
}

bool EnumerateWindows(std::vector<Window>& windows) {
  const auto result = ::EnumWindows(
      EnumerateWindowsProc, reinterpret_cast<LPARAM>(&windows));
  return result != FALSE;
}

}  // namespace win
}  // namespace anisthesia
