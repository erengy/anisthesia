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

#include <memory>

#include <windows.h>
#include <winternl.h>

#include "process.h"
#include "util.h"

namespace anisthesia {
namespace win {

using buffer_t = std::unique_ptr<BYTE[]>;

constexpr NTSTATUS STATUS_INFO_LENGTH_MISMATCH = 0xC0000004L;
constexpr NTSTATUS STATUS_SUCCESS = 0x00000000L;

enum {
  SystemExtendedHandleInformation = 64,
};

struct SYSTEM_HANDLE_TABLE_ENTRY_INFO_EX {
  PVOID Object;
  ULONG_PTR UniqueProcessId;
  HANDLE HandleValue;
  ACCESS_MASK GrantedAccess;
  USHORT CreatorBackTraceIndex;
  USHORT ObjectTypeIndex;
  ULONG HandleAttributes;
  ULONG Reserved;
};

struct SYSTEM_HANDLE_INFORMATION_EX {
  ULONG_PTR NumberOfHandles;
  ULONG_PTR Reserved;
  SYSTEM_HANDLE_TABLE_ENTRY_INFO_EX Handles[1];
};

struct OBJECT_TYPE_INFORMATION {
  UNICODE_STRING TypeName;
  ULONG Reserved[22];
};

////////////////////////////////////////////////////////////////////////////////

PVOID GetLibraryProcAddress(LPCSTR module_name, LPCSTR proc_name) {
  return reinterpret_cast<PVOID>(
      GetProcAddress(GetModuleHandleA(module_name), proc_name));
}

PVOID GetNtProcAddress(LPCSTR proc_name) {
  return GetLibraryProcAddress("ntdll.dll", proc_name);
}

NTSTATUS NtQuerySystemInformation(
    SYSTEM_INFORMATION_CLASS system_information_class,
    PVOID system_information,
    ULONG system_information_length,
    PULONG return_length) {
  static const auto nt_query_system_information =
      reinterpret_cast<decltype(::NtQuerySystemInformation)*>(
          GetNtProcAddress("NtQuerySystemInformation"));
  return nt_query_system_information(system_information_class,
                                     system_information,
                                     system_information_length,
                                     return_length);
}

NTSTATUS NtQueryObject(
    HANDLE handle,
    OBJECT_INFORMATION_CLASS object_information_class,
    PVOID object_information,
    ULONG object_information_length,
    PULONG return_length) {
  static const auto nt_query_object =
      reinterpret_cast<decltype(::NtQueryObject)*>(
          GetNtProcAddress("NtQueryObject"));
  return nt_query_object(handle,
                         object_information_class,
                         object_information,
                         object_information_length,
                         return_length);
}

////////////////////////////////////////////////////////////////////////////////

inline bool NtSuccess(NTSTATUS status) {
  return status >= 0;
}

buffer_t QuerySystemInformation(
      SYSTEM_INFORMATION_CLASS system_information_class) {
  ULONG size = 1 << 20;                //  1 MiB
  constexpr ULONG kMaxSize = 1 << 24;  // 16 MiB

  buffer_t buffer(new BYTE[size]);
  NTSTATUS status = STATUS_SUCCESS;

  do {
    ULONG return_length = 0;
    status = win::NtQuerySystemInformation(
        system_information_class, buffer.get(), size, &return_length);
    if (status == STATUS_INFO_LENGTH_MISMATCH) {
      size = (return_length > size) ? return_length : (size * 2);
      buffer.reset(new BYTE[size]);
    }
  } while (status == STATUS_INFO_LENGTH_MISMATCH && size < kMaxSize);

  if (!NtSuccess(status))
    buffer.reset();

  return buffer;
}

buffer_t QueryObject(HANDLE handle,
                     OBJECT_INFORMATION_CLASS object_information_class,
                     ULONG size) {
  buffer_t buffer(new BYTE[size]);
  ULONG return_length = 0;

  const auto query_object = [&]() {
    return win::NtQueryObject(handle, object_information_class,
                              buffer.get(), size, &return_length);
  };

  auto status = query_object();
  if (status == STATUS_INFO_LENGTH_MISMATCH) {
    size = return_length;
    buffer.reset(new BYTE[size]);
    status = query_object();
  }

  if (!NtSuccess(status))
    buffer.reset();

  return buffer;
}

////////////////////////////////////////////////////////////////////////////////

HANDLE OpenProcess(DWORD process_id) {
  return ::OpenProcess(PROCESS_DUP_HANDLE, false, process_id);
}

HANDLE DuplicateHandle(HANDLE process_handle, HANDLE handle) {
  HANDLE dup_handle = nullptr;
  const auto result = ::DuplicateHandle(process_handle, handle,
                                        GetCurrentProcess(), &dup_handle,
                                        0, false, DUPLICATE_SAME_ACCESS);
  return result ? dup_handle : nullptr;
}

buffer_t GetSystemHandleInformation() {
  return QuerySystemInformation(
      static_cast<SYSTEM_INFORMATION_CLASS>(SystemExtendedHandleInformation));
}

std::wstring GetUnicodeString(const UNICODE_STRING& unicode_string) {
  if (!unicode_string.Length)
    return std::wstring();

  return std::wstring(unicode_string.Buffer,
                      unicode_string.Length / sizeof(WCHAR));
}

std::wstring GetObjectTypeName(HANDLE handle) {
  const auto buffer = QueryObject(handle, ObjectTypeInformation,
                                  sizeof(OBJECT_TYPE_INFORMATION));
  if (!buffer)
    return std::wstring();

  const auto& type_information =
      *reinterpret_cast<OBJECT_TYPE_INFORMATION*>(buffer.get());

  return GetUnicodeString(type_information.TypeName);
}

std::wstring GetFinalPathNameByHandle(HANDLE handle) {
  std::wstring buffer(MAX_PATH, '\0');

  auto get_final_path_name_by_handle = [&]() {
    return ::GetFinalPathNameByHandle(handle, &buffer.front(), buffer.size(),
        FILE_NAME_NORMALIZED | VOLUME_NAME_DOS);
  };

  auto result = get_final_path_name_by_handle();
  if (result > buffer.size()) {
    buffer.resize(result, '\0');
    result = get_final_path_name_by_handle();
  }

  if (result < buffer.size())
    buffer.resize(result);

  return buffer;
}

////////////////////////////////////////////////////////////////////////////////

inline USHORT GetFileTypeIndex() {
  // File type index varies between OS versions:
  //
  // - 25: Windows Vista
  // - 28: Windows XP, Windows 7
  // - 30: Windows 8.1
  // - 31: Windows 8, Windows 10
  // - 34: Windows 10 Anniversary Update
  //
  // Here we return 0, so that the value is determined at run time. This is more
  // reliable than hard-coding the values for each OS version.
  return 0;
}

bool VerifyAccessMask(ULONG access_mask) {
  // Skip access masks that may cause certain functions such as NtQueryObject
  // and GetFinalPathNameByHandle to hang. These masks usually belong to named
  // pipes.
  switch (access_mask) {
    case 0x00100000:  // SYNCHRONIZE access
    case 0x0012008d:  // e.g. "\Device\NamedPipe\DropboxDataPipe"
    case 0x00120189:
    case 0x0012019f:
    case 0x0016019f:  // e.g. "\Device\Afd\Endpoint"
    case 0x001a019f:
      return false;
    default:
      return true;
  }

  return true;
}

bool VerifyPathName(const std::wstring& path) {
  if (path.empty())
    return false;

  // Skip files under system directories
  // @TODO: Use %windir% environment variable
  const std::wstring windir = L"C:\\Windows";
  const size_t pos = path.find_first_not_of(L"\\?");
  if (path.substr(pos, windir.size()) == windir)
    return false;

  // Skip invalid files, and directories
  const auto file_attr = ::GetFileAttributes(path.c_str());
  if ((file_attr == INVALID_FILE_ATTRIBUTES) ||
      (file_attr & FILE_ATTRIBUTE_DIRECTORY)) {
    return false;
  }

  return true;
}

bool EnumerateFiles(std::map<DWORD, std::vector<std::wstring>>& files) {
  std::map<DWORD, Handle> process_handles;
  for (const auto pair : files) {
    const auto process_id = pair.first;
    const auto handle = OpenProcess(process_id);
    if (handle)
      process_handles[process_id] = Handle(handle);
  }
  if (process_handles.empty())
    return false;

  auto system_handle_information_buffer = GetSystemHandleInformation();
  if (!system_handle_information_buffer)
    return false;

  const auto& system_handle_information =
      *reinterpret_cast<SYSTEM_HANDLE_INFORMATION_EX*>(
          system_handle_information_buffer.get());
  if (!system_handle_information.NumberOfHandles)
    return false;

  static auto file_type_index = GetFileTypeIndex();

  for (size_t i = 0; i < system_handle_information.NumberOfHandles; ++i) {
    const auto& handle = system_handle_information.Handles[i];

    // Skip if this handle does not belong to one of our PIDs
    const auto process_id = static_cast<DWORD>(handle.UniqueProcessId);
    if (!process_handles.count(process_id))
      continue;

    // Skip if this is not a file handle
    if (file_type_index && handle.ObjectTypeIndex != file_type_index)
      continue;

    // Skip access masks that are known to cause trouble
    if (!VerifyAccessMask(handle.GrantedAccess))
      continue;

    // Duplicate the handle so that we can query it
    const auto process_handle = process_handles[process_id].get();
    Handle dup_handle(DuplicateHandle(process_handle, handle.HandleValue));
    if (!dup_handle)
      continue;

    // Determine file type index, for future reference
    if (!file_type_index) {
      if (GetObjectTypeName(dup_handle.get()) == L"File") {
        file_type_index = handle.ObjectTypeIndex;
      } else {
        continue;
      }
    }

    // Skip if this is not a disk file (i.e. it is a character file, a socket,
    // a pipe, or a file of unknown type)
    if (::GetFileType(dup_handle.get()) != FILE_TYPE_DISK)
      continue;

    const auto path = GetFinalPathNameByHandle(dup_handle.get());
    if (VerifyPathName(path))
      files[process_id].push_back(path);
  }

  return true;
}

}  // namespace win
}  // namespace anisthesia
