#pragma once

#include <memory>
#include <string>
#include <type_traits>

#include <windows.h>
#include <unknwn.h>

namespace anisthesia::win::detail {

struct HandleDeleter {
  using pointer = HANDLE;
  void operator()(pointer p) const { ::CloseHandle(p); }
};

using Handle = std::unique_ptr<HANDLE, HandleDeleter>;

////////////////////////////////////////////////////////////////////////////////
// Alternative to Microsoft::WRL::ComPtr in <wrl/client.h>

template <typename T>
struct ComInterfaceDeleter {
  static_assert(std::is_base_of<IUnknown, T>::value, "Invalid COM interface");
  using pointer = T*;
  void operator()(pointer p) const { if (p) p->Release(); }
};

template <typename T>
using ComInterface = std::unique_ptr<T, ComInterfaceDeleter<T>>;

////////////////////////////////////////////////////////////////////////////////

std::wstring GetFileNameFromPath(const std::wstring& path);
std::wstring GetFileNameWithoutExtension(const std::wstring& filename);
bool IsSystemDirectory(const std::wstring& path);

std::string ToUtf8String(const std::wstring& str);

}  // namespace anisthesia::win::detail
