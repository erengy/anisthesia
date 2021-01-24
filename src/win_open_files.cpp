#include <map>
#include <memory>

#include <windows.h>
#include <winternl.h>

#include <anisthesia/win_open_files.hpp>
#include <anisthesia/win_util.hpp>

namespace anisthesia::win::detail {

// WARNING: This file uses internal Windows APIs. The functions and structures
// that are defined here and in <winternl.h> are subject to change.

constexpr NTSTATUS STATUS_INFO_LENGTH_MISMATCH = 0xC0000004L;
constexpr NTSTATUS STATUS_SUCCESS = 0x00000000L;

enum {
  // SystemHandleInformation is limited to 16-bit process IDs, so we use
  // SystemExtendedHandleInformation instead.
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

// This structure is defined as PUBLIC_OBJECT_TYPE_INFORMATION in Microsoft's
// <winternl.h>, while it is defined as OBJECT_TYPE_INFORMATION in GCC's header
// with actual data members instead of 'Reserved[22]'.
struct OBJECT_TYPE_INFORMATION {
  UNICODE_STRING TypeName;
  ULONG Reserved[22];
};

using buffer_t = std::unique_ptr<BYTE[]>;

////////////////////////////////////////////////////////////////////////////////

PVOID GetNtProcAddress(LPCSTR proc_name) {
  // We have to use run-time dynamic linking, because there is no associated
  // import library for the internal functions.
  return reinterpret_cast<PVOID>(
      GetProcAddress(GetModuleHandleA("ntdll.dll"), proc_name));
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
    status = win::detail::NtQuerySystemInformation(
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
    return win::detail::NtQueryObject(handle, object_information_class,
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
  // If we try to open a SYSTEM process, this function fails and the last error
  // code is ERROR_ACCESS_DENIED.
  return ::OpenProcess(PROCESS_DUP_HANDLE, false, process_id);
}

HANDLE DuplicateHandle(HANDLE process_handle, HANDLE handle) {
  HANDLE dup_handle = nullptr;
  const auto result = ::DuplicateHandle(process_handle, handle,
      ::GetCurrentProcess(), &dup_handle, 0, false, DUPLICATE_SAME_ACCESS);
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

bool VerifyObjectType(HANDLE handle, USHORT object_type_index) {
  // File type index varies between OS versions:
  //
  // Index | OS version
  // ------|-----------
  //    25 | Vista
  //    28 | XP
  //       | 7
  //    30 | 8.1
  //    31 | 8
  //       | 10
  //    34 | 10 1607 (Anniversary Update)
  //    35 | 10 1703 (Creators Update)
  //    36 | 10 1709 (Fall Creators Update)
  //       | 10 1803 (April 2018 Update)
  //       | 10 1809 (October 2018 Update)
  //    37 | 10 1903 (May 2019 Update)
  //       | 10 1909 (November 2019 Update)
  //       | 10 2004 (May 2020 Update)
  //       | 10 20H2 (October 2020 Update)
  //
  // Here we initialize the value with 0, so that it is determined at run time.
  // This is more reliable than hard-coding the values for each OS version.
  static USHORT file_type_index = 0;

  if (file_type_index)
    return object_type_index == file_type_index;

  if (!handle)
    return true;

  if (GetObjectTypeName(handle) == L"File") {
    file_type_index = object_type_index;
    return true;
  }

  return false;
}

bool VerifyAccessMask(ACCESS_MASK access_mask) {
  // Certain kinds of handles, mostly those which refer to named pipes, cause
  // some functions such as NtQueryObject and GetFinalPathNameByHandle to hang.
  // By examining the access mask, we can get some clues about the object type
  // and bail out if necessary.
  //
  // One method would be to hard-code individual masks that are known to cause
  // trouble:
  //
  // - 0x00100000 (SYNCHRONIZE access)
  // - 0x0012008d (e.g. "\Device\NamedPipe\DropboxDataPipe")
  // - 0x00120189
  // - 0x0012019f
  // - 0x0016019f (e.g. "\Device\Afd\Endpoint")
  // - 0x001a019f
  //
  // While this works in most situations, we occasionally end up skipping valid
  // file handles. This method also requires updating the blacklist when we
  // encounter other masks that cause our application to hang on users' end.
  //
  // The most common access mask for the valid files we are interested in is
  // 0x00120089, which is made up of the following access rights:
  //
  // - 0x00000001 FILE_READ_DATA
  // - 0x00000008 FILE_READ_EA
  // - 0x00000080 FILE_READ_ATTRIBUTES
  // - 0x00020000 READ_CONTROL
  // - 0x00100000 SYNCHRONIZE
  //
  // Media players must have read-access in order to play a video file, so we
  // can safely skip a handle in the absence of this basic right:
  if (!(access_mask & FILE_READ_DATA))
    return false;

  // We further assume that media players do not have any kind of write access
  // to video files:
  if ((access_mask & FILE_APPEND_DATA) ||
      (access_mask & FILE_WRITE_EA) ||
      (access_mask & FILE_WRITE_ATTRIBUTES)) {
    return false;
  }

  return true;
}

bool VerifyFileType(HANDLE handle) {
  // Skip character files, sockets, pipes, and files of unknown type
  return ::GetFileType(handle) == FILE_TYPE_DISK;
}

bool VerifyPath(const std::wstring& path) {
  if (path.empty())
    return false;

  // Skip files under system directories
  if (IsSystemDirectory(path))
    return false;

  // Skip invalid files, and directories
  const auto file_attributes = ::GetFileAttributes(path.c_str());
  if ((file_attributes == INVALID_FILE_ATTRIBUTES) ||
      (file_attributes & FILE_ATTRIBUTE_DIRECTORY)) {
    return false;
  }

  return true;
}

////////////////////////////////////////////////////////////////////////////////

bool EnumerateOpenFiles(const std::set<DWORD>& process_ids,
                        open_file_proc_t open_file_proc) {
  if (!open_file_proc)
    return false;

  std::map<DWORD, Handle> process_handles;
  for (const auto& process_id : process_ids) {
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

  for (size_t i = 0; i < system_handle_information.NumberOfHandles; ++i) {
    const auto& handle = system_handle_information.Handles[i];

    // Skip if this handle does not belong to one of our PIDs
    const auto process_id = static_cast<DWORD>(handle.UniqueProcessId);
    if (!process_ids.count(process_id))
      continue;

    // Skip if this is not a file handle
    if (!VerifyObjectType(nullptr, handle.ObjectTypeIndex))
      continue;

    // Skip if the file handle has an inappropriate access mask
    if (!VerifyAccessMask(handle.GrantedAccess))
      continue;

    // Duplicate the handle so that we can query it
    const auto process_handle = process_handles[process_id].get();
    Handle dup_handle(DuplicateHandle(process_handle, handle.HandleValue));
    if (!dup_handle)
      continue;

    // Skip if this is not a file handle, while determining file type index
    if (!VerifyObjectType(dup_handle.get(), handle.ObjectTypeIndex))
      continue;

    // Skip if this is not a disk file
    if (!VerifyFileType(dup_handle.get()))
      continue;

    OpenFile open_file;
    open_file.process_id = process_id;
    open_file.path = GetFinalPathNameByHandle(dup_handle.get());

    if (!VerifyPath(open_file.path))
      continue;

    if (!open_file_proc(open_file))
      return false;
  }

  return true;
}

}  // namespace anisthesia::win::detail
