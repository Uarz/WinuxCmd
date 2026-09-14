/*
 *  Copyright © 2026 [caomengxuan666]
 *
 *  Permission is hereby granted, free of charge, to any person obtaining a copy
 *  of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 *  The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 */
module;

#include "pch/pch.h"

export module utils:native_path;

import std;
import :utf8;

namespace native_path {

export auto from_utf8(std::string_view path) -> std::wstring {
  return utf8_to_wstring(path);
}

export auto to_utf8(std::wstring_view path) -> std::string {
  return wstring_to_utf8(path);
}

export auto normalize_separators(std::wstring path) -> std::wstring {
  for (auto& ch : path) {
    if (ch == L'/') ch = L'\\';
  }
  return path;
}

export auto normalize_separators_utf8(std::string path) -> std::string {
  for (auto& ch : path) {
    if (ch == '/') ch = '\\';
  }
  return path;
}

export auto is_separator(wchar_t ch) -> bool {
  return ch == L'\\' || ch == L'/';
}

export auto is_separator(char ch) -> bool { return ch == '\\' || ch == '/'; }

export auto unc_root_length(std::wstring_view path) -> std::optional<size_t> {
  if (path.size() < 3 || !is_separator(path[0]) || !is_separator(path[1])) {
    return std::nullopt;
  }

  const auto server_end = path.find_first_of(L"\\/", 2);
  if (server_end == std::wstring_view::npos) return path.size();

  const auto share_start = server_end + 1;
  const auto share_end = path.find_first_of(L"\\/", share_start);
  if (share_end == std::wstring_view::npos) return path.size();
  return share_end;
}

export auto unc_root_length(std::string_view path) -> std::optional<size_t> {
  if (path.size() < 3 || !is_separator(path[0]) || !is_separator(path[1])) {
    return std::nullopt;
  }

  const auto server_end = path.find_first_of("\\/", 2);
  if (server_end == std::string_view::npos) return path.size();

  const auto share_start = server_end + 1;
  const auto share_end = path.find_first_of("\\/", share_start);
  if (share_end == std::string_view::npos) return path.size();
  return share_end;
}

export auto strip_trailing_separators(std::wstring_view path)
    -> std::wstring_view {
  while (path.size() > 1 && is_separator(path.back())) {
    if (path.size() == 3 && path[1] == L':') break;
    if (auto root_len = unc_root_length(path);
        root_len && path.size() <= *root_len + 1) {
      break;
    }
    path.remove_suffix(1);
  }
  return path;
}

export auto strip_trailing_separators(std::string_view path)
    -> std::string_view {
  while (path.size() > 1 && is_separator(path.back())) {
    if (path.size() == 3 && path[1] == ':') break;
    if (auto root_len = unc_root_length(path);
        root_len && path.size() <= *root_len + 1) {
      break;
    }
    path.remove_suffix(1);
  }
  return path;
}

export auto normalize_api_operand_w(std::wstring_view path) -> std::wstring {
  std::wstring normalized(strip_trailing_separators(path));
  if (normalized.size() >= 2 && is_separator(normalized[0]) &&
      ((normalized[1] >= L'a' && normalized[1] <= L'z') ||
       (normalized[1] >= L'A' && normalized[1] <= L'Z')) &&
      (normalized.size() == 2 || is_separator(normalized[2]))) {
    std::wstring drive_path;
    wchar_t drive = normalized[1];
    if (drive >= L'a' && drive <= L'z') {
      drive = static_cast<wchar_t>(drive - L'a' + L'A');
    }
    drive_path.push_back(drive);
    drive_path.append(L":\\");
    if (normalized.size() > 3) {
      drive_path.append(normalized.substr(3));
    }
    return normalize_separators(std::move(drive_path));
  }
  return normalized;
}

export auto resolve_pseudo_device_w(std::wstring_view path)
    -> std::optional<std::wstring>;

export auto normalize_api_operand(std::string_view path) -> std::string {
  // Delegate to the wide implementation so MSYS/Git-Bash style operands such
  // as "/d/repo/file" are converted to "D:\repo\file" exactly like
  // make_api_path_operand does; otherwise return the stripped path unchanged.
  // POSIX pseudo-devices resolve to their Windows equivalents first so tools
  // reading through this boundary (od, dd, ...) see /dev/null etc. (#276).
  const std::wstring wide = from_utf8(path);
  if (auto pseudo = resolve_pseudo_device_w(wide)) {
    return to_utf8(*pseudo);
  }
  return to_utf8(normalize_api_operand_w(wide));
}

export auto to_extended_path(std::wstring_view path) -> std::wstring {
  if (path.size() >= 4 && path.compare(0, 4, L"\\\\?\\") == 0) {
    return std::wstring(path);
  }

  std::wstring native(path);
  wchar_t abs_buf[32768];
  DWORD len = GetFullPathNameW(native.c_str(), 32768, abs_buf, nullptr);
  if (len == 0 || len >= 32768) return native;

  std::wstring absolute(abs_buf, len);
  if (absolute.size() >= 2 && absolute.compare(0, 2, L"\\\\") == 0) {
    return L"\\\\?\\UNC\\" + absolute.substr(2);
  }
  return L"\\\\?\\" + absolute;
}

export struct ApiPathOperand {
  std::wstring original;
  std::wstring normalized;
  std::wstring extended;
  bool had_trailing_separator = false;
};

// [GNU] POSIX pseudo-devices that GNU environments expose under /dev but
// that have no directory entry on Windows. Mapping them at the shared path
// boundary (instead of relying on an external runtime directory such as
// niubash's dev/) keeps tools like dd/tee/cat/pr working when they receive
// literal /dev/* operands (#276 follow-up, uutils#9745).
export auto resolve_pseudo_device_w(std::wstring_view path)
    -> std::optional<std::wstring> {
  if (path == L"/dev/null") return std::wstring(L"NUL");
  if (path == L"/dev/stdin") return std::wstring(L"CONIN$");
  if (path == L"/dev/stdout") return std::wstring(L"CONOUT$");
  if (path == L"/dev/stderr") return std::wstring(L"CONOUT$");
  if (path == L"/dev/tty") return std::wstring(L"CONIN$");
  return std::nullopt;
}

// Which inherited standard stream an operand names, if any.
//
// POSIX /dev/stdin, /dev/stdout and /dev/stderr are symlinks to /proc/self/fd/N:
// they resolve to *this process's own descriptors*, so `cat /dev/stdin < file`
// reads the file and `tee /dev/stdout > file` writes the file. Mapping them onto
// the DOS console devices loses that redirection, and for stdin it is worse than
// a wrong result - opening CONIN$ when descriptor 0 is a pipe makes the command
// wait on the console forever instead of reading its input (#276 follow-up).
//
// Open sites must therefore consume the inherited handle instead of a path.
// resolve_pseudo_device_w still answers with the console device name so that
// attribute probes (ls, stat, test) continue to see a valid character device.
export enum class StandardStream : unsigned char {
  none = 0,
  in,
  out,
  err,
};

export auto standard_stream_w(std::wstring_view path) -> StandardStream {
  if (path == L"/dev/stdin" || path == L"/dev/fd/0") return StandardStream::in;
  if (path == L"/dev/stdout" || path == L"/dev/fd/1") return StandardStream::out;
  if (path == L"/dev/stderr" || path == L"/dev/fd/2") return StandardStream::err;
  return StandardStream::none;
}

export auto standard_stream(std::string_view path) -> StandardStream {
  return standard_stream_w(from_utf8(path));
}

// Duplicate the inherited standard handle so a tool reads or writes the stream
// the shell actually handed it, redirection included. The caller owns the
// returned handle. INVALID_HANDLE_VALUE when the descriptor is unavailable -
// for example when the shell closed it with `<&-`, which GNU also reports as a
// bad file descriptor rather than as an empty stream (#973).
export auto duplicate_standard_handle(StandardStream stream) -> HANDLE {
  DWORD id = STD_INPUT_HANDLE;
  if (stream == StandardStream::out) {
    id = STD_OUTPUT_HANDLE;
  } else if (stream == StandardStream::err) {
    id = STD_ERROR_HANDLE;
  }

  const HANDLE source = GetStdHandle(id);
  if (source == nullptr || source == INVALID_HANDLE_VALUE) {
    return INVALID_HANDLE_VALUE;
  }

  HANDLE copy = INVALID_HANDLE_VALUE;
  if (!DuplicateHandle(GetCurrentProcess(), source, GetCurrentProcess(), &copy,
                       0, FALSE, DUPLICATE_SAME_ACCESS)) {
    return INVALID_HANDLE_VALUE;
  }
  return copy;
}

export auto make_api_path_operand_w(std::wstring_view path) -> ApiPathOperand {
  ApiPathOperand operand;
  operand.original = std::wstring(path);
  if (auto pseudo = resolve_pseudo_device_w(operand.original)) {
    // DOS device names (NUL, CONIN$, CONOUT$) must NOT carry the \\?\
    // prefix: the \\?\ namespace bypasses Win32 device-name resolution, and
    // GetFullPathNameW would resolve "NUL" against the current directory
    // instead. Keep the device name verbatim in both normalized and extended.
    operand.normalized = *pseudo;
    operand.extended = *pseudo;
    operand.had_trailing_separator = false;
    return operand;
  }
  operand.normalized = normalize_api_operand_w(operand.original);
  operand.extended = to_extended_path(operand.normalized);
  operand.had_trailing_separator =
      operand.normalized.size() != operand.original.size();
  return operand;
}

export auto make_api_path_operand(std::string_view path) -> ApiPathOperand {
  return make_api_path_operand_w(from_utf8(path));
}

// True when the operand is a bare DOS device name.
//
// Device names resolve through the Win32 device namespace, which the \\?\
// extended-length prefix bypasses: to_extended_path would run
// GetFullPathNameW("NUL"), resolve the name against the current directory and
// hand back "\\?\<cwd>\NUL" - an ordinary, missing file. Every Win32 call must
// therefore receive these names verbatim, which is exactly the invariant
// make_api_path_operand_w establishes when it resolves a pseudo-device; this
// predicate lets the attribute probes honour it too. Verified with
// GetFileAttributesW: "NUL" -> 0x20 (valid), "\\?\<cwd>\NUL" -> INVALID.
// Keep the list in sync with resolve_pseudo_device_w, which is where the
// pseudo-devices that reach this function are produced.
export auto is_dos_device_name_w(std::wstring_view path) -> bool {
  path = strip_trailing_separators(path);

  // An already explicit device namespace ("\\.\NUL") needs no adjustment.
  if (path.size() > 4 && path.compare(0, 4, L"\\\\.\\") == 0) return true;

  // A device name only resolves when it is the whole operand: "dir\NUL" names
  // an ordinary file, and so does "\\?\C:\dir\NUL".
  if (path.find_first_of(L"\\/") != std::wstring_view::npos) return false;

  std::wstring upper;
  upper.reserve(path.size());
  for (const wchar_t ch : path) {
    upper.push_back((ch >= L'a' && ch <= L'z')
                        ? static_cast<wchar_t>(ch - L'a' + L'A')
                        : ch);
  }

  if (upper == L"NUL" || upper == L"CON" || upper == L"PRN" || upper == L"AUX" ||
      upper == L"CONIN$" || upper == L"CONOUT$") {
    return true;
  }
  return upper.size() == 4 &&
         (upper.starts_with(L"COM") || upper.starts_with(L"LPT")) &&
         upper[3] >= L'1' && upper[3] <= L'9';
}

// True when the operand denotes a Windows character device, either directly as
// a DOS device name or as a POSIX pseudo-device that resolves onto one. GNU
// reports /dev/null as a character special file, so `test -c /dev/null` is true
// and `test -b /dev/null` is false there; both predicates need this answer.
export auto is_character_device_w(std::wstring_view path) -> bool {
  if (auto pseudo = resolve_pseudo_device_w(path)) {
    return is_dos_device_name_w(*pseudo);
  }
  return is_dos_device_name_w(path);
}

export auto is_character_device(std::string_view path) -> bool {
  return is_character_device_w(from_utf8(path));
}

export auto attributes_w(std::wstring_view path) -> DWORD {
  // A DOS device name must reach the API verbatim; see is_dos_device_name_w.
  if (is_dos_device_name_w(path)) {
    const std::wstring verbatim(strip_trailing_separators(path));
    return GetFileAttributesW(verbatim.c_str());
  }

  // Keep all attribute probes on the same extended-path API boundary as
  // file_io. This avoids MAX_PATH failures for otherwise valid paths.
  const std::wstring native = path.starts_with(L"\\\\?\\")
                                  ? std::wstring(path)
                                  : to_extended_path(path);
  return GetFileAttributesW(native.c_str());
}

// Full attribute record for an operand that may name a character device.
//
// GetFileAttributesExW is the one attribute API that rejects DOS device names
// outright. Probed against kernel32 on Windows 11:
//
//   GetFileAttributesW("NUL")      -> 0x20   (FILE_ATTRIBUTE_ARCHIVE)
//   GetFileAttributesExW("NUL")    -> FALSE, ERROR_INVALID_PARAMETER (87)
//   GetFileAttributesExW("CONIN$") -> FALSE, ERROR_INVALID_FUNCTION (1)
//
// So a tool that renders a full stat record cannot use the Ex variant for
// /dev/* operands: the device resolves correctly and then the query fails. This
// helper keeps both APIs behind one device-aware call - device names are
// answered from GetFileAttributesW with a zeroed size/time record, which is
// exactly what a character device reports anyway (GNU prints a size of 0 for
// /dev/null). Ordinary paths keep the Ex variant so long paths and reparse
// points behave as before.
export auto file_attribute_data_w(std::wstring_view path,
                                  WIN32_FILE_ATTRIBUTE_DATA& out) -> bool {
  out = WIN32_FILE_ATTRIBUTE_DATA{};

  std::wstring probe;
  if (auto pseudo = resolve_pseudo_device_w(path)) {
    probe = *pseudo;
  } else {
    probe.assign(path);
  }

  if (is_dos_device_name_w(probe)) {
    const std::wstring verbatim(strip_trailing_separators(probe));
    const DWORD attrs = GetFileAttributesW(verbatim.c_str());
    if (attrs == INVALID_FILE_ATTRIBUTES) return false;
    out.dwFileAttributes = attrs;
    return true;
  }

  const std::wstring native =
      probe.starts_with(L"\\\\?\\") ? probe : to_extended_path(probe);
  return GetFileAttributesExW(native.c_str(), GetFileExInfoStandard, &out) !=
         0;
}

export auto file_attribute_data(std::string_view path,
                                WIN32_FILE_ATTRIBUTE_DATA& out) -> bool {
  return file_attribute_data_w(from_utf8(path), out);
}

export auto valid_attributes(DWORD attrs) -> bool {
  return attrs != INVALID_FILE_ATTRIBUTES;
}

export auto attributes_are_directory(DWORD attrs) -> bool {
  return valid_attributes(attrs) && (attrs & FILE_ATTRIBUTE_DIRECTORY) != 0;
}

export auto attributes_are_regular_file(DWORD attrs) -> bool {
  return valid_attributes(attrs) && (attrs & FILE_ATTRIBUTE_DIRECTORY) == 0;
}

export auto attributes_are_reparse_point(DWORD attrs) -> bool {
  return valid_attributes(attrs) && (attrs & FILE_ATTRIBUTE_REPARSE_POINT) != 0;
}

export auto current_directory_w() -> std::wstring {
  DWORD len = GetCurrentDirectoryW(0, nullptr);
  if (len == 0) return {};

  std::wstring buffer(len, L'\0');
  DWORD written = GetCurrentDirectoryW(len, buffer.data());
  if (written == 0 || written >= len) return {};
  buffer.resize(written);
  return buffer;
}

export auto current_directory() -> std::string {
  return to_utf8(current_directory_w());
}

export auto parent_path_w(std::wstring_view path) -> std::wstring {
  std::filesystem::path parsed{std::wstring(path)};
  return parsed.parent_path().wstring();
}

export auto filename_w(std::wstring_view path) -> std::wstring {
  std::filesystem::path parsed{std::wstring(path)};
  return parsed.filename().wstring();
}

export auto stem_w(std::wstring_view path) -> std::wstring {
  std::filesystem::path parsed{std::wstring(path)};
  return parsed.stem().wstring();
}

export auto extension_w(std::wstring_view path) -> std::wstring {
  std::filesystem::path parsed{std::wstring(path)};
  return parsed.extension().wstring();
}

export auto parent_path(std::string_view path) -> std::string {
  return to_utf8(parent_path_w(from_utf8(path)));
}

export auto filename(std::string_view path) -> std::string {
  return to_utf8(filename_w(from_utf8(path)));
}

export auto stem(std::string_view path) -> std::string {
  return to_utf8(stem_w(from_utf8(path)));
}

export auto extension(std::string_view path) -> std::string {
  return to_utf8(extension_w(from_utf8(path)));
}

export auto exists_w(std::wstring_view path) -> bool {
  return valid_attributes(attributes_w(path));
}

export auto is_directory_w(std::wstring_view path) -> bool {
  return attributes_are_directory(attributes_w(path));
}

export auto is_regular_file_w(std::wstring_view path) -> bool {
  return attributes_are_regular_file(attributes_w(path));
}

export auto exists(std::string_view path) -> bool {
  return exists_w(from_utf8(path));
}

export auto is_directory(std::string_view path) -> bool {
  return is_directory_w(from_utf8(path));
}

export auto is_regular_file(std::string_view path) -> bool {
  return is_regular_file_w(from_utf8(path));
}

export auto create_directory_w(std::wstring_view path) -> bool {
  std::wstring native(path);
  if (CreateDirectoryW(native.c_str(), nullptr)) return true;
  DWORD error = GetLastError();
  if (error != ERROR_ALREADY_EXISTS) return false;
  return is_directory_w(native);
}

export auto create_directories_w(std::wstring_view path) -> bool {
  std::error_code ec;
  if (std::filesystem::create_directories(std::filesystem::path(path), ec)) {
    return true;
  }
  if (!ec) return is_directory_w(path);
  return is_directory_w(path);
}

export auto create_directory(std::string_view path) -> bool {
  return create_directory_w(from_utf8(path));
}

export auto create_directories(std::string_view path) -> bool {
  return create_directories_w(from_utf8(path));
}

export auto join_w(std::wstring_view base, std::wstring_view relative)
    -> std::wstring {
  std::filesystem::path joined = std::filesystem::path(std::wstring(base)) /
                                 std::filesystem::path(std::wstring(relative));
  return joined.make_preferred().wstring();
}

export auto join(std::string_view base, std::string_view relative)
    -> std::string {
  return to_utf8(join_w(from_utf8(base), from_utf8(relative)));
}

}  // namespace native_path
