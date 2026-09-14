/*
 *  Copyright © 2026 [caomengxuan666]
 *
 *  Permission is hereby granted, free of charge, to any person obtaining a copy
 *  of this software and associated documentation files (the “Software”), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 *  The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 *  THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 *
 *  - File: cp.cpp
 *  - Username: Administrator
 *  - CopyrightYear: 2026
 */

/// @contributors:
///   - @contributor1 caomengxuan666 2507560089@qq.com
///   - @contributor2 <email2@example.com>
///   - @contributor3 <email3@example.com>
/// @Description: Implemention for cp.
/// @Version: 0.1.0
/// @License: MIT
/// @Copyright: Copyright © 2026 WinuxCmd

#include "pch/pch.h"
// include other header after pch.h
#pragma comment(lib, "shlwapi.lib")
#include "core/command_macros.h"

import std;
import core;
import utils;
/**
 * @brief CP command options definition
 *
 * This array defines all the options supported by the cp command.
 * Each option is described with its short form, long form, and description.
 * The implementation status is also indicated for each option.
 *
 * @par Options:
 *
 * - @a -a, @a --archive: Same as -dR --preserve=all [IMPLEMENTED]
 * - @a -b: Like --backup but does not accept an argument [IMPLEMENTED]
 * - @a -d: Same as --no-dereference --preserve=links [TODO]
 * - @a -f, @a --force: If an existing destination file cannot be opened, remove
 * it and try again [TODO]
 * - @a -i, @a --interactive: Prompt before overwrite [IMPLEMENTED]
 * - @a -H: Follow command-line symbolic links in SOURCE [TODO]
 * - @a -l, @a --link: Hard link files instead of copying [TODO]
 * - @a -L, @a --dereference: Always follow symbolic links in SOURCE [TODO]
 * - @a -n, @a --no-clobber: Do not overwrite an existing file and do not fail
 * [TODO]
 * - @a -P, @a --no-dereference: Never follow symbolic links in SOURCE [TODO]
 * - @a -p: Same as --preserve=mode,ownership,timestamps [TODO]
 * - @a -R, @a --recursive: Copy directories recursively [IMPLEMENTED]
 * - @a -r, @a --recursive: Copy directories recursively [IMPLEMENTED]
 * - @a -s, @a --symbolic-link: Make symbolic links instead of copying [TODO]
 * - @a -S, @a --suffix: Override the usual backup suffix [IMPLEMENTED]
 * - @a -t, @a --target-directory: Copy all SOURCE arguments into DIRECTORY
 * [IMPLEMENTED]
 * - @a -T, @a --no-target-directory: Treat DEST as a normal file [IMPLEMENTED]
 * - @a -u: Equivalent to --update[=older] [IMPLEMENTED]
 * - @a -v, @a --verbose: Explain what is being done [IMPLEMENTED]
 * - @a -x, @a --one-file-system: Stay on this file system [TODO]
 * - @a -Z: Set SELinux security context of destination file to default type
 * [TODO]
 * - @a --parent, @a --parents: Use full source file name under DIRECTORY
 * [IMPLEMENTED]
 * - @a --debug: Explain how a file is copied [IMPLEMENTED AS VERBOSE]
 * - @a -g, @a --progress-bar: Accept progress-bar flag [COMPAT NO-OP]
 */

using cmd::meta::OptionMeta;
using cmd::meta::OptionType;

// ======================================================
// Constants
// ======================================================
namespace cp_constants {
constexpr char DEFAULT_BACKUP_SUFFIX[] = "~";
}

// ======================================================
// Options (constexpr)
// ======================================================

// [GNU] -a, --archive: same as -dR --preserve=all
// [GNU] -b: like --backup but does not accept an argument
// [GNU] --backup: make a backup of each existing destination file
// [GNU] -d: same as --no-dereference --preserve=links
// [GNU] -f, --force: remove existing destination and try again
// [GNU] -i, --interactive: prompt before overwrite
// [GNU] -H: follow command-line symbolic links in SOURCE
// [GNU] -l, --link: hard link files instead of copying
// [GNU] -L, --dereference: always follow symbolic links in SOURCE
// [GNU] -n, --no-clobber: do not overwrite an existing file
// [GNU] -P, --no-dereference: never follow symbolic links in SOURCE
// [GNU] -p: same as --preserve=mode,ownership,timestamps
// [GNU] -R, --recursive: copy directories recursively
// [GNU] -r, --recursive: copy directories recursively (alias for -R)
// [GNU] -s, --symbolic-link: make symbolic links instead of copying
// [GNU] -S, --suffix: override the usual backup suffix
// [GNU] -t, --target-directory: copy all SOURCE arguments into DIRECTORY
// [GNU] -T, --no-target-directory: treat DEST as a normal file
// [GNU] --strip-trailing-slashes: remove trailing slashes from SOURCE
// [GNU] -u, --update: equivalent to --update[=older]
// [GNU] -v, --verbose: explain what is being done
// [GNU] --debug: explain how a file is copied; implies --verbose
// [EXT] -g, --progress-bar: WinuxCmd extension, not in GNU coreutils
// [GNU] -x, --one-file-system: stay on this file system
// [DIFFERS] -Z: SELinux contexts are unavailable on Windows
// [IMPLEMENTED] --remove-destination: remove each existing destination file
// before open [GNU] --attributes-only: don't copy the file data, just the
// attributes [GNU] --parents: use full source file name under DIRECTORY [GNU]
// --parent: alias for --parents [GNU] --sparse: control creation of sparse
// files [GNU] --reflink: control clone/CoW copies [GNU] --preserve: preserve
// the specified attributes [GNU] --no-preserve: don't preserve the specified
// attributes [GNU] --copy-contents: copy contents of special files when
// recursive [IMPLEMENTED] --keep-directory-symlink: follow existing symlink to
// directory Windows directory operations follow an existing destination
// directory symlink.
auto constexpr CP_OPTIONS = std::array{
    OPTION("", "--keep-directory-symlink",
           "follow existing symlink to directory"),
    OPTION("-a", "--archive", "same as -dR --preserve=all"),
    OPTION("-b", "", "like --backup but does not accept an argument"),
    OPTION("", "--backup", "make a backup of each existing destination file",
           OPTIONAL_STRING_TYPE),
    OPTION("-d", "", "same as --no-dereference --preserve=links"),
    OPTION("-f", "--force",
           "if an existing destination file cannot be opened, remove it and "
           "try again"),
    OPTION("-i", "--interactive", "prompt before overwrite"),
    OPTION("-H", "", "follow command-line symbolic links in SOURCE"),
    OPTION("-l", "--link", "hard link files instead of copying"),
    OPTION("-L", "--dereference", "always follow symbolic links in SOURCE"),
    OPTION("-n", "--no-clobber",
           "do not overwrite an existing file and do not fail"),
    OPTION("-P", "--no-dereference", "never follow symbolic links in SOURCE"),
    OPTION("-p", "", "same as --preserve=mode,ownership,timestamps"),
    OPTION("-R", "--recursive", "copy directories recursively"),
    OPTION("-r", "--recursive", "copy directories recursively"),
    OPTION("-s", "--symbolic-link", "make symbolic links instead of copying"),
    OPTION("-S", "--suffix", "override the usual backup suffix", STRING_TYPE),
    OPTION("-t", "--target-directory",
           "copy all SOURCE arguments into DIRECTORY", STRING_TYPE),
    OPTION("-T", "--no-target-directory", "treat DEST as a normal file"),
    OPTION("", "--strip-trailing-slashes",
           "remove any trailing slashes from each SOURCE argument"),
    OPTION("-u", "--update", "equivalent to --update[=older]"),
    OPTION("-v", "--verbose", "explain what is being done"),
    OPTION("", "--debug", "explain how a file is copied; implies --verbose"),
    OPTION("-g", "--progress-bar", "display a progress bar while copying"),
    OPTION("-x", "--one-file-system", "stay on this file system"),
    // [DIFFERS] SELinux security contexts are unavailable on Windows.
    OPTION("-Z", "",
           "set SELinux security context of destination file to default type"),
    OPTION(
        "", "--remove-destination",
        "remove each existing destination file before attempting to open it"),
    OPTION("", "--attributes-only",
           "don't copy the file data, just the attributes"),
    OPTION("", "--parents", "use full source file name under DIRECTORY"),
    OPTION("", "--parent", "use full source file name under DIRECTORY"),
    OPTION("", "--sparse", "control creation of sparse files", STRING_TYPE),
    OPTION("", "--reflink", "control clone/CoW copies", OPTIONAL_STRING_TYPE),
    OPTION("", "--preserve", "preserve the specified attributes", STRING_TYPE),
    OPTION("", "--no-preserve", "don't preserve the specified attributes",
           STRING_TYPE),
    OPTION("", "--copy-contents",
           "copy contents of special files when recursive"),
    OPTION("-c", "--context",
           "set SELinux security context of destination file to default type")};

// ======================================================
// Pipeline components
// ======================================================
namespace cp_pipeline {
namespace cp = core::pipeline;

auto append_expanded_source(std::vector<std::string>& source_paths,
                            std::string_view arg) -> void {
  std::string source(arg);
  if (contains_wildcard(source)) {
    auto glob_result = glob_expand(source);
    if (glob_result.expanded) {
      for (const auto& file : glob_result.files) {
        source_paths.push_back(wstring_to_utf8(file));
      }
      return;
    }
  }
  source_paths.push_back(std::move(source));
}

auto normalize_path_separators(std::string path) -> std::string {
  std::ranges::replace(path, '/', '\\');
  return path;
}

auto strip_trailing_slashes(std::string path) -> std::string {
  while (path.size() > 1 && (path.back() == '\\' || path.back() == '/')) {
    if (path.size() == 3 && path[1] == ':') break;
    path.pop_back();
  }
  return path;
}

// ----------------------------------------------
// 1. Validate arguments
// ----------------------------------------------
auto validate_arguments(const CommandContext<CP_OPTIONS.size()>& ctx)
    -> cp::Result<std::pair<std::vector<std::string>, std::string>> {
  std::vector<std::string> sourcePaths;
  std::string destPath;
  const bool strip_slashes = ctx.get<bool>("--strip-trailing-slashes", false);

  // Get target directory if specified
  std::string target_dir = ctx.get<std::string>("--target-directory", "");
  if (target_dir.empty()) {
    target_dir = ctx.get<std::string>("-t", "");
  }
  if (!target_dir.empty()) {
    bool no_target_directory = ctx.get<bool>("-T", false) ||
                               ctx.get<bool>("--no-target-directory", false);
    if (no_target_directory) {
      return std::unexpected(
          "cannot combine --target-directory (-t) and --no-target-directory "
          "(-T)\nTry 'cp --help' for more information.");
    }

    DWORD attr = native_path::attributes_w(utf8_to_wstring(target_dir));
    if (attr == INVALID_FILE_ATTRIBUTES) {
      return std::unexpected("target directory '" + target_dir +
                             "': No such file or directory");
    }
    if (!(attr & FILE_ATTRIBUTE_DIRECTORY)) {
      return std::unexpected("target directory '" + target_dir +
                             "': Not a directory");
    }

    destPath = target_dir;
    for (auto arg : ctx.positionals) {
      append_expanded_source(sourcePaths, arg);
    }
    if (sourcePaths.empty()) {
      return std::unexpected(
          "missing file operand\nTry 'cp --help' for more information.");
    }
  } else {
    // Regular case: last argument is destination
    if (ctx.positionals.empty()) {
      return std::unexpected(
          "missing file operand\nTry 'cp --help' for more information.");
    }
    if (ctx.positionals.size() < 2) {
      return std::unexpected("missing destination file operand after '" +
                             std::string(ctx.positionals[0]) +
                             "'\nTry 'cp --help' for more information.");
    }

    for (size_t i = 0; i < ctx.positionals.size() - 1; ++i) {
      append_expanded_source(sourcePaths, ctx.positionals[i]);
    }
    destPath = std::string(ctx.positionals.back());
  }

  if (strip_slashes) {
    for (auto& src : sourcePaths) {
      src = strip_trailing_slashes(std::move(src));
    }
  }

  if (sourcePaths.empty()) {
    return std::unexpected(
        "missing file operand\nTry 'cp --help' for more information.");
  }

  return std::pair{sourcePaths, destPath};
}

// ----------------------------------------------
// 2. Check if destination is directory
// ----------------------------------------------
auto check_destination(
    const std::pair<std::vector<std::string>, std::string>& paths,
    bool no_target_directory)
    -> cp::Result<std::tuple<std::vector<std::string>, std::string, bool>> {
  const auto& [sourcePaths, destPath] = paths;

  DWORD attr = native_path::attributes_w(utf8_to_wstring(destPath));
  bool destIsDir = !no_target_directory && (attr != INVALID_FILE_ATTRIBUTES) &&
                   (attr & FILE_ATTRIBUTE_DIRECTORY);

  if (sourcePaths.size() > 1 && !destIsDir) {
    // GNU 9.4: errno-style diagnostics naming the target operand.
    if (attr == INVALID_FILE_ATTRIBUTES) {
      return std::unexpected("target '" + destPath +
                             "': No such file or directory");
    }
    return std::unexpected("target '" + destPath + "': Not a directory");
  }

  return std::tuple{sourcePaths, destPath, destIsDir};
}

// ----------------------------------------------
// 3. Create directory recursively
// ----------------------------------------------
auto create_directory_recursive(const std::string& path) -> cp::Result<bool> {
  auto operand = native_path::make_api_path_operand(path);
  if (CreateDirectoryW(operand.extended.c_str(), NULL) ||
      GetLastError() == ERROR_ALREADY_EXISTS) {
    return true;
  }

  // If parent directory doesn't exist, create it first
  size_t lastSlash = path.find_last_of("\\/");
  if (lastSlash == std::string::npos) {
    return std::unexpected("cannot create directory");
  }

  std::string parentPath = path.substr(0, lastSlash);
  auto parentResult = create_directory_recursive(parentPath);
  if (!parentResult) {
    return parentResult;
  }

  // Now create the current directory
  if (CreateDirectoryW(operand.extended.c_str(), NULL) == 0) {
    return std::unexpected("cannot create directory");
  }

  return true;
}

// ----------------------------------------------
// 4. Check if path exists
// ----------------------------------------------
auto path_exists(const std::string& path) -> cp::Result<bool> {
  // Pseudo-device operands (NUL, /dev/null, /dev/std*, /proc/*/fd/N) carry
  // no file attributes but are valid copy sources like under GNU
  // (#1055/#1056).
  if (native_path::resolve_pseudo_device_w(native_path::from_utf8(path))) {
    if (native_path::pseudo_device_std_fd(path) == std::optional<int>(0)) {
      // /dev/stdin dangles (ENOENT) when fd 0 is closed — GNU reports
      // "cannot stat ... No such file or directory".
      return !file_io::stdin_is_bad();
    }
    return true;
  }
  return native_path::valid_attributes(
      native_path::attributes_w(utf8_to_wstring(path)));
}

// ----------------------------------------------
// 5. Check if path exists and is directory
// ----------------------------------------------
auto path_exists_and_is_directory(const std::string& path) -> cp::Result<bool> {
  DWORD attr = native_path::attributes_w(utf8_to_wstring(path));
  return (attr != INVALID_FILE_ATTRIBUTES) && (attr & FILE_ATTRIBUTE_DIRECTORY);
}

auto archive_enabled(const CommandContext<CP_OPTIONS.size()>& ctx) -> bool {
  return ctx.get<bool>("--archive", false) || ctx.get<bool>("-a", false);
}

auto preserve_metadata_enabled(const CommandContext<CP_OPTIONS.size()>& ctx)
    -> bool {
  // --no-preserve without argument disables all preservation
  if (ctx.has("--no-preserve") &&
      ctx.get<std::string>("--no-preserve", "").empty()) {
    return false;
  }
  if (archive_enabled(ctx) || ctx.get<bool>("-p", false)) return true;
  if (ctx.has("--preserve")) return true;
  return false;
}

auto preserve_metadata(const std::string& srcPath, const std::string& destPath)
    -> cp::Result<bool> {
  auto src_operand = native_path::make_api_path_operand(srcPath);
  auto dest_operand = native_path::make_api_path_operand(destPath);

  WIN32_FILE_ATTRIBUTE_DATA src_data{};
  if (!GetFileAttributesExW(src_operand.extended.c_str(), GetFileExInfoStandard,
                            &src_data)) {
    return std::unexpected("cannot read source metadata");
  }

  const DWORD flags = (src_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                          ? FILE_FLAG_BACKUP_SEMANTICS
                          : 0;
  HANDLE handle =
      CreateFileW(dest_operand.extended.c_str(), FILE_WRITE_ATTRIBUTES,
                  FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                  nullptr, OPEN_EXISTING, flags, nullptr);
  if (handle == INVALID_HANDLE_VALUE &&
      GetLastError() == ERROR_FILE_NOT_FOUND &&
      !(src_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
    handle = CreateFileW(dest_operand.extended.c_str(), FILE_WRITE_ATTRIBUTES,
                         FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                         nullptr, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);
  }
  if (handle == INVALID_HANDLE_VALUE) {
    return std::unexpected("cannot write destination metadata");
  }

  BOOL time_ok =
      SetFileTime(handle, &src_data.ftCreationTime, &src_data.ftLastAccessTime,
                  &src_data.ftLastWriteTime);
  CloseHandle(handle);
  if (!time_ok) {
    return std::unexpected("cannot preserve timestamps");
  }

  if (!SetFileAttributesW(dest_operand.extended.c_str(),
                          src_data.dwFileAttributes)) {
    return std::unexpected("cannot preserve attributes");
  }
  return true;
}

enum class BackupControl { none, simple, numbered, existing };

auto backup_control_name(std::string control) -> std::string {
  std::ranges::transform(control, control.begin(), [](unsigned char ch) {
    return static_cast<char>(std::tolower(ch));
  });
  return control;
}

auto parse_backup_control(std::string control) -> cp::Result<BackupControl> {
  control = backup_control_name(std::move(control));
  if (control.empty() || control == "existing" || control == "nil") {
    return BackupControl::existing;
  }
  if (control == "none" || control == "off") {
    return BackupControl::none;
  }
  if (control == "simple" || control == "never") {
    return BackupControl::simple;
  }
  if (control == "numbered" || control == "t") {
    return BackupControl::numbered;
  }
  return std::unexpected("invalid backup type");
}

auto requested_backup_control(const CommandContext<CP_OPTIONS.size()>& ctx)
    -> cp::Result<BackupControl> {
  if (ctx.get<bool>("-b", false)) {
    return BackupControl::existing;
  }
  if (ctx.has("--backup")) {
    return parse_backup_control(ctx.get<std::string>("--backup", ""));
  }
  return BackupControl::none;
}

auto backup_enabled(const CommandContext<CP_OPTIONS.size()>& ctx) -> bool {
  auto control = requested_backup_control(ctx);
  return control && *control != BackupControl::none;
}

auto backup_suffix(const CommandContext<CP_OPTIONS.size()>& ctx)
    -> std::string {
  auto suffix = ctx.get<std::string>("--suffix", "");
  if (suffix.empty()) {
    suffix = ctx.get<std::string>("-S", "");
  }
  if (suffix.empty()) {
    if (const char* env_suffix = std::getenv("SIMPLE_BACKUP_SUFFIX");
        env_suffix != nullptr && *env_suffix != '\0') {
      suffix = env_suffix;
    } else {
      suffix = cp_constants::DEFAULT_BACKUP_SUFFIX;
    }
  }
  return suffix;
}

auto numbered_backup_path(const std::wstring& dest_path) -> std::wstring {
  for (int version = 1;; ++version) {
    std::wstring candidate =
        dest_path + L".~" + std::to_wstring(version) + L"~";
    if (!native_path::valid_attributes(native_path::attributes_w(candidate))) {
      return candidate;
    }
  }
}

auto simple_backup_path(const std::wstring& dest_path,
                        const CommandContext<CP_OPTIONS.size()>& ctx)
    -> std::wstring {
  return dest_path + utf8_to_wstring(backup_suffix(ctx));
}

auto select_backup_path(const std::wstring& dest_path,
                        const CommandContext<CP_OPTIONS.size()>& ctx)
    -> cp::Result<std::optional<std::wstring>> {
  auto control = requested_backup_control(ctx);
  if (!control) return std::unexpected(control.error());

  switch (*control) {
    case BackupControl::none:
      return std::optional<std::wstring>{};
    case BackupControl::simple:
      return std::optional<std::wstring>{simple_backup_path(dest_path, ctx)};
    case BackupControl::numbered:
      return std::optional<std::wstring>{numbered_backup_path(dest_path)};
    case BackupControl::existing: {
      std::wstring first_numbered = dest_path + L".~1~";
      if (native_path::valid_attributes(
              native_path::attributes_w(first_numbered))) {
        return std::optional<std::wstring>{numbered_backup_path(dest_path)};
      }
      return std::optional<std::wstring>{simple_backup_path(dest_path, ctx)};
    }
  }
  return std::unexpected("invalid backup type");
}

auto verbose_enabled(const CommandContext<CP_OPTIONS.size()>& ctx) -> bool {
  return ctx.get<bool>("--verbose", false) || ctx.get<bool>("-v", false) ||
         ctx.get<bool>("--debug", false);
}

auto backup_existing_destination(const std::string& destPath,
                                 const CommandContext<CP_OPTIONS.size()>& ctx)
    -> cp::Result<bool> {
  if (!backup_enabled(ctx)) {
    return true;
  }

  auto dest_operand = native_path::make_api_path_operand(destPath);
  if (!native_path::valid_attributes(
          native_path::operand_target_attributes_w(dest_operand))) {
    return true;
  }

  auto backup_path = select_backup_path(dest_operand.extended, ctx);
  if (!backup_path) {
    return std::unexpected(backup_path.error());
  }
  if (!*backup_path) {
    return true;
  }
  if (!MoveFileExW(dest_operand.extended.c_str(), (*backup_path)->c_str(),
                   MOVEFILE_REPLACE_EXISTING)) {
    return std::unexpected("cannot create backup for destination");
  }
  return true;
}

auto copy_self_with_backup(const std::string& path, const std::string& destPath,
                           const CommandContext<CP_OPTIONS.size()>& ctx)
    -> cp::Result<bool> {
  bool force = ctx.get<bool>("--force", false) || ctx.get<bool>("-f", false);
  if (!force || !backup_enabled(ctx)) {
    // GNU: cp: 'a' and 'b' are the same file
    return std::unexpected("'" + path + "' and '" + destPath +
                           "' are the same file");
  }

  std::wstring wpath = utf8_to_wstring(path);
  auto backup_path = select_backup_path(wpath, ctx);
  if (!backup_path) {
    return std::unexpected(backup_path.error());
  }
  if (!*backup_path) {
    return true;
  }
  if (!CopyFileW(wpath.c_str(), (*backup_path)->c_str(), FALSE)) {
    return std::unexpected("cannot create backup for destination");
  }
  return true;
}

// lstat-style existence probe: a (possibly dangling) symlink itself counts.
auto lexists(const std::string& path) -> bool {
  std::error_code ec;
  auto status = std::filesystem::symlink_status(utf8_to_wstring(path), ec);
  return !ec && status.type() != std::filesystem::file_type::not_found;
}

auto is_symlink_path(const std::string& path) -> bool {
  std::error_code ec;
  return std::filesystem::is_symlink(
      std::filesystem::symlink_status(utf8_to_wstring(path), ec));
}

// [GNU] An existing destination is removed before a link is created.
// Directories (real ones) are not silently removed: GNU reports
// "cannot overwrite directory ... with non-directory" instead.
auto remove_destination_entry(const std::string& destPath) -> cp::Result<bool> {
  auto dest_operand = native_path::make_api_path_operand(destPath);
  DWORD attrs = native_path::operand_target_attributes_w(dest_operand);
  if (attrs == INVALID_FILE_ATTRIBUTES) {
    return true;
  }
  if (attrs & FILE_ATTRIBUTE_DIRECTORY) {
    // A directory symlink is removed like a directory; a real directory is
    // left alone so the caller can report the GNU diagnostic.
    if ((attrs & FILE_ATTRIBUTE_REPARSE_POINT) == 0) {
      return std::unexpected("cannot overwrite directory '" + destPath +
                             "' with non-directory");
    }
    if (!RemoveDirectoryW(dest_operand.extended.c_str())) {
      return std::unexpected("cannot remove '" + destPath +
                             "': " + win32_posix_error_text(GetLastError()));
    }
    return true;
  }
  SetFileAttributesW(dest_operand.extended.c_str(), FILE_ATTRIBUTE_NORMAL);
  if (!DeleteFileW(dest_operand.extended.c_str())) {
    return std::unexpected("cannot remove '" + destPath +
                           "': " + win32_posix_error_text(GetLastError()));
  }
  return true;
}

// [GNU] -s: a relative SOURCE may only be linked into the current
// directory; anything else would produce a broken link (#218, #274).
auto dest_in_current_directory(const std::string& destPath) -> bool {
  std::filesystem::path parent =
      std::filesystem::path(utf8_to_wstring(destPath)).parent_path();
  if (parent.empty()) {
    return true;
  }
  std::error_code ec;
  auto cwd = std::filesystem::current_path(ec);
  if (ec) return true;
  auto parent_abs = std::filesystem::absolute(parent, ec);
  if (ec) return true;
  // GNU: failure to stat the destination parent is ignored here; the
  // subsequent link creation reports its own error.
  if (!native_path::valid_attributes(
          native_path::attributes_w(parent_abs.wstring()))) {
    return true;
  }
  std::error_code eq_ec;
  return std::filesystem::equivalent(cwd, parent_abs, eq_ec) && !eq_ec;
}

auto create_symlink_copy(const std::string& srcPath,
                         const std::string& destPath,
                         const std::wstring& link_target, bool src_is_dir,
                         bool verbose) -> cp::Result<bool> {
  std::wstring wdest = utf8_to_wstring(destPath);
  DWORD flags = SYMBOLIC_LINK_FLAG_ALLOW_UNPRIVILEGED_CREATE;
  if (src_is_dir) flags |= SYMBOLIC_LINK_FLAG_DIRECTORY;
  if (!CreateSymbolicLinkW(wdest.c_str(), link_target.c_str(), flags)) {
    return std::unexpected("cannot create symbolic link '" + destPath +
                           "' to '" + srcPath +
                           "': " + win32_posix_error_text(GetLastError()));
  }
  if (verbose) {
    safePrint("'");
    safePrint(srcPath);
    safePrint("' -> '");
    safePrint(destPath);
    safePrint("'\n");
  }
  return true;
}

// ----------------------------------------------
// 6. Copy a single file
// ----------------------------------------------
auto copy_file(const std::string& srcPath, const std::string& destPath,
               const CommandContext<CP_OPTIONS.size()>& ctx)
    -> cp::Result<bool> {
  bool interactive =
      ctx.get<bool>("--interactive", false) || ctx.get<bool>("-i", false);
  bool verbose = verbose_enabled(ctx);
  bool no_clobber =
      ctx.get<bool>("--no-clobber", false) || ctx.get<bool>("-n", false);
  bool update = ctx.get<bool>("-u", false) || ctx.get<bool>("--update", false);
  bool remove_dest = ctx.has("--remove-destination");
  bool attrs_only = ctx.has("--attributes-only");
  bool hard_link = ctx.get<bool>("--link", false) || ctx.get<bool>("-l", false);
  bool symbolic_link =
      ctx.get<bool>("--symbolic-link", false) || ctx.get<bool>("-s", false);
  bool force = ctx.get<bool>("--force", false) || ctx.get<bool>("-f", false);
  // -d and -a imply --no-dereference: symlink sources are recreated as links.
  bool no_deref = ctx.get<bool>("-P", false) ||
                  ctx.get<bool>("--no-dereference", false) ||
                  ctx.get<bool>("-d", false) || archive_enabled(ctx);

  std::error_code equivalent_ec;
  // Use the error_code overload: pseudo-device operands such as "NUL" make
  // GetFileAttributesExW report ERROR_INVALID_PARAMETER, which the throwing
  // overload turns into an uncaught filesystem_error.
  if (!hard_link && !symbolic_link &&
      std::filesystem::exists(srcPath, equivalent_ec) && !equivalent_ec &&
      lexists(destPath) &&
      std::filesystem::equivalent(srcPath, destPath, equivalent_ec) &&
      !equivalent_ec) {
    return copy_self_with_backup(srcPath, destPath, ctx);
  }

  bool src_is_symlink = is_symlink_path(srcPath);
  bool dest_exists = lexists(destPath);

  if (no_clobber && dest_exists) {
    return true;
  }

  if (update && dest_exists) {
    WIN32_FILE_ATTRIBUTE_DATA src_data{};
    WIN32_FILE_ATTRIBUTE_DATA dest_data{};
    auto src_operand = native_path::make_api_path_operand(srcPath);
    auto dst_operand = native_path::make_api_path_operand(destPath);
    if (GetFileAttributesExW(src_operand.extended.c_str(),
                             GetFileExInfoStandard, &src_data) &&
        GetFileAttributesExW(dst_operand.extended.c_str(),
                             GetFileExInfoStandard, &dest_data)) {
      if (CompareFileTime(&src_data.ftLastWriteTime,
                          &dest_data.ftLastWriteTime) <= 0) {
        return true;
      }
    }
  }

  if (interactive && dest_exists) {
    safeErrorPrint("cp: overwrite '");
    safeErrorPrint(destPath);
    safeErrorPrint("'? ");
    char response;
    std::cin.get(response);
    if (response != 'y' && response != 'Y') {
      return true;
    }
  }

  // --remove-destination: remove existing dest before opening
  if (remove_dest && dest_exists) {
    auto dest_operand = native_path::make_api_path_operand(destPath);
    DWORD dest_attrs =
        native_path::operand_target_attributes_w(dest_operand);
    if (dest_attrs != INVALID_FILE_ATTRIBUTES) {
      if (dest_attrs & FILE_ATTRIBUTE_DIRECTORY) {
        RemoveDirectoryW(dest_operand.extended.c_str());
      } else {
        SetFileAttributesW(dest_operand.extended.c_str(),
                           FILE_ATTRIBUTE_NORMAL);
        DeleteFileW(dest_operand.extended.c_str());
      }
    }
  }

  auto backupResult = backup_existing_destination(destPath, ctx);
  if (!backupResult) {
    return backupResult;
  }

  dest_exists = lexists(destPath);

  if (hard_link) {
    std::wstring dest = utf8_to_wstring(destPath);
    std::wstring link_source = utf8_to_wstring(srcPath);
    // GNU -l dereferences the source by default; only -P/-d/-a link the
    // symlink itself.  CreateHardLinkW on a symlink operand links the
    // reparse point, which is exactly the no-dereference behaviour.
    if (!no_deref && src_is_symlink) {
      std::error_code res_ec;
      auto resolved =
          std::filesystem::canonical(utf8_to_wstring(srcPath), res_ec);
      if (!res_ec) link_source = resolved.wstring();
    }
    if (CreateHardLinkW(dest.c_str(), link_source.c_str(), nullptr)) {
      if (verbose) {
        safePrint("'");
        safePrint(srcPath);
        safePrint("' -> '");
        safePrint(destPath);
        safePrint("'\n");
      }
      return true;
    }
    DWORD link_err = GetLastError();
    if (dest_exists && (force || (no_deref && src_is_symlink))) {
      // -f, or a technically-different symlink destination being replaced
      // by a hardlink-copy of a symlink source (uutils#6531).
      auto rm = remove_destination_entry(destPath);
      if (!rm) return rm;
      if (CreateHardLinkW(dest.c_str(), link_source.c_str(), nullptr)) {
        if (verbose) {
          safePrint("'");
          safePrint(srcPath);
          safePrint("' -> '");
          safePrint(destPath);
          safePrint("'\n");
        }
        return true;
      }
      link_err = GetLastError();
    }
    return std::unexpected("cannot create hard link '" + destPath + "' to '" +
                           srcPath + "': " + win32_posix_error_text(link_err));
  }

  if (symbolic_link) {
    std::wstring source = utf8_to_wstring(srcPath);
    // [GNU] refuse a relative SOURCE unless DEST lands in the current
    // directory; otherwise the link text would resolve to the wrong place.
    bool src_absolute = std::filesystem::path(source).is_absolute() ||
                        source[0] == L'/' || source[0] == L'\\';
    if (!src_absolute && !dest_in_current_directory(destPath)) {
      return std::unexpected(
          destPath +
          ": can make relative symbolic links only in current directory");
    }
    if (dest_exists) {
      if (!force) {
        return std::unexpected("cannot create symbolic link '" + destPath +
                               "' to '" + srcPath + "': File exists");
      }
      auto rm = remove_destination_entry(destPath);
      if (!rm) return rm;
    }
    DWORD attrs = SYMBOLIC_LINK_FLAG_ALLOW_UNPRIVILEGED_CREATE;
    if (path_exists_and_is_directory(srcPath).value_or(false)) {
      attrs |= SYMBOLIC_LINK_FLAG_DIRECTORY;
    }
    if (CreateSymbolicLinkW(utf8_to_wstring(destPath).c_str(), source.c_str(),
                            attrs)) {
      if (verbose) {
        safePrint("'");
        safePrint(srcPath);
        safePrint("' -> '");
        safePrint(destPath);
        safePrint("'\n");
      }
      return true;
    }
    return std::unexpected("cannot create symbolic link '" + destPath +
                           "' to '" + srcPath +
                           "': " + win32_posix_error_text(GetLastError()));
  }

  // [GNU] -P/-d/-a: a symlink source is recreated as a link (the link text
  // is copied verbatim), not followed.
  if (no_deref && src_is_symlink) {
    std::error_code rl_ec;
    auto target =
        std::filesystem::read_symlink(utf8_to_wstring(srcPath), rl_ec);
    if (rl_ec) {
      return std::unexpected("cannot read symbolic link '" + srcPath +
                             "': " + std::string(rl_ec.message()));
    }
    if (dest_exists) {
      auto rm = remove_destination_entry(destPath);
      if (!rm) return rm;
    }
    bool src_is_dir = false;
    {
      DWORD sattrs = GetFileAttributesW(utf8_to_wstring(srcPath).c_str());
      src_is_dir = sattrs != INVALID_FILE_ATTRIBUTES &&
                   (sattrs & FILE_ATTRIBUTE_DIRECTORY);
    }
    return create_symlink_copy(srcPath, destPath, target.wstring(), src_is_dir,
                               verbose);
  }

  if (attrs_only) {
    // --attributes-only: copy only metadata, not file data
    {
      auto preserveResult = preserve_metadata(srcPath, destPath);
      if (!preserveResult) return preserveResult;
    }
    if (verbose) {
      safePrint("'");
      safePrint(srcPath);
      safePrint("' -> '");
      safePrint(destPath);
      safePrint("' (attributes only)\n");
    }
    return true;
  }

  // Check if source file exists and is readable
  errno = 0;
  std::ifstream src = file_io::open_binary_file(srcPath);
  if (!src) {
    return std::unexpected("cannot open '" + srcPath +
                           "' for reading: " + strerror(errno));
  }

  // Open destination file
  std::ofstream dest = file_io::create_binary_file(destPath);
  if (!dest) {
    const int open_err = errno;
    if (!force) {
      return std::unexpected("cannot create regular file '" + destPath +
                             "': " + strerror(open_err));
    }

    auto dest_operand = native_path::make_api_path_operand(destPath);
    SetFileAttributesW(dest_operand.extended.c_str(), FILE_ATTRIBUTE_NORMAL);
    DeleteFileW(dest_operand.extended.c_str());
    dest = file_io::create_binary_file(destPath);
    if (!dest) {
      return std::unexpected("cannot create regular file '" + destPath +
                             "': " + strerror(errno));
    }
  }

  // Copy file content
  dest << src.rdbuf();
  if (dest.bad()) {
    return std::unexpected("error writing '" + destPath + "'");
  }

  // Flush and close the files
  dest.flush();
  dest.close();
  src.close();

  if (preserve_metadata_enabled(ctx)) {
    auto preserveResult = preserve_metadata(srcPath, destPath);
    if (!preserveResult) return preserveResult;
  }

  if (verbose) {
    // OPTIMIZED: Avoid wstring concatenation
    safePrint("'");
    safePrint(srcPath);
    safePrint("' -> '");
    safePrint(destPath);
    safePrint("'\n");
  }

  return true;
}

// ----------------------------------------------
// 7. Copy directory recursively
// ----------------------------------------------
// Helper function with depth limit
auto copy_directory_helper(const std::string& srcPath,
                           const std::string& destPath,
                           const CommandContext<CP_OPTIONS.size()>& ctx,
                           int depth) -> cp::Result<bool> {
  // Prevent deep recursion
  if (depth > 100) {
    return std::unexpected("maximum recursion depth exceeded");
  }

  // Prevent copying directory into itself
  if (srcPath == destPath) {
    return true;
  }

  // Prevent copying directory where destination is a subdirectory of source
  if (destPath.find(srcPath) == 0 && destPath.size() > srcPath.size() &&
      (destPath[srcPath.size()] == '\\' || destPath[srcPath.size()] == '/')) {
    // OPTIMIZED: Avoid wstring concatenation
    safeErrorPrint("cp: cannot copy directory '");
    safeErrorPrint(srcPath);
    safeErrorPrint("' into itself '");
    safeErrorPrint(destPath);
    safeErrorPrint("'\n");
    return std::unexpected("cannot copy directory into itself");
  }

  // Create destination directory if it doesn't exist
  auto createResult = create_directory_recursive(destPath);
  if (!createResult) {
    return createResult;
  }

  // Open source directory
  std::wstring searchPath = utf8_to_wstring(srcPath) + L"\\*";
  searchPath = native_path::make_api_path_operand_w(searchPath).extended;
  WIN32_FIND_DATAW findData;
  HANDLE hFind = FindFirstFileW(searchPath.c_str(), &findData);
  if (hFind == INVALID_HANDLE_VALUE) {
    return std::unexpected("cannot open directory");
  }

  bool success = true;
  bool verbose = verbose_enabled(ctx);
  bool no_deref = ctx.get<bool>("-P", false) ||
                  ctx.get<bool>("--no-dereference", false) ||
                  ctx.get<bool>("-d", false) || archive_enabled(ctx);

  // Process each item in the directory
  do {
    // Skip . and ..
    if (wcscmp(findData.cFileName, L".") == 0 ||
        wcscmp(findData.cFileName, L"..") == 0) {
      continue;
    }

    // Get the full path of the source file/directory
    int fileNameLength = WideCharToMultiByte(CP_UTF8, 0, findData.cFileName, -1,
                                             NULL, 0, NULL, NULL);
    if (fileNameLength <= 0) {
      continue;
    }
    // Subtract 1 to exclude the null terminator
    int actualLength = fileNameLength - 1;
    std::string fileName(actualLength, 0);
    WideCharToMultiByte(CP_UTF8, 0, findData.cFileName, -1, &fileName[0],
                        fileNameLength, NULL, NULL);
    // Remove the null terminator if present
    fileName.resize(actualLength);

    std::string srcItemPath = srcPath + "\\" + fileName;
    std::string destItemPath = destPath + "\\" + fileName;

    bool is_dir_child =
        (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
    // [GNU] -P/-d/-a: a symlink child is recreated as a link, not followed.
    bool link_child =
        no_deref &&
        (findData.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) &&
        findData.dwReserved0 == IO_REPARSE_TAG_SYMLINK;

    // Check if it's a directory
    if (is_dir_child && !link_child) {
      // Verify it's actually a directory
      DWORD attr = native_path::attributes_w(utf8_to_wstring(srcItemPath));
      if (attr != INVALID_FILE_ATTRIBUTES &&
          (attr & FILE_ATTRIBUTE_DIRECTORY)) {
        // Recursively copy subdirectory with increased depth
        auto subDirResult =
            copy_directory_helper(srcItemPath, destItemPath, ctx, depth + 1);
        if (!subDirResult) {
          success = false;
        }
      }
    } else {
      // Copy file (or recreate the symlink under -P/-d/-a)
      auto fileResult = copy_file(srcItemPath, destItemPath, ctx);
      if (!fileResult) {
        safeErrorPrint("cp: ");
        safeErrorPrint(fileResult.error());
        safeErrorPrint("\n");
        success = false;
      }
    }
  } while (FindNextFileW(hFind, &findData));

  FindClose(hFind);

  if (success && preserve_metadata_enabled(ctx)) {
    auto preserveResult = preserve_metadata(srcPath, destPath);
    if (!preserveResult) return preserveResult;
  }

  return success;
}

// Public copy_directory function
auto copy_directory(const std::string& srcPath, const std::string& destPath,
                    const CommandContext<CP_OPTIONS.size()>& ctx)
    -> cp::Result<bool> {
  return copy_directory_helper(srcPath, destPath, ctx, 0);
}

// ----------------------------------------------
// 8. Process each source path
// ----------------------------------------------
auto process_source_paths(
    const std::tuple<std::vector<std::string>, std::string, bool>& pathsAndDir,
    const CommandContext<CP_OPTIONS.size()>& ctx) -> cp::Result<bool> {
  const auto& [sourcePaths, destPath, destIsDir] = pathsAndDir;
  bool recursive = ctx.get<bool>("--recursive", false);
  recursive |= ctx.get<bool>("-r", false);
  recursive |= ctx.get<bool>("-R", false);
  recursive |= archive_enabled(ctx);
  bool no_deref = ctx.get<bool>("-P", false) ||
                  ctx.get<bool>("--no-dereference", false) ||
                  ctx.get<bool>("-d", false) || archive_enabled(ctx);
  bool success = true;

  for (const auto& srcPath : sourcePaths) {
    // Check if source path exists; under -P/-d/-a a dangling symlink still
    // counts because the link itself is copied.
    bool src_exists =
        no_deref ? lexists(srcPath) : path_exists(srcPath).value_or(false);
    if (!src_exists) {
      // OPTIMIZED: Avoid wstring concatenation
      safeErrorPrint("cp: cannot stat '");
      safeErrorPrint(srcPath);
      safeErrorPrint("': No such file or directory\n");
      success = false;
      continue;
    }

    // Check if source is directory
    auto isDirResult = path_exists_and_is_directory(srcPath);
    if (!isDirResult) {
      success = false;
      continue;
    }

    bool srcIsDir = *isDirResult;
    std::string finalDestPath = destPath;

    bool parents = ctx.has("--parents") || ctx.has("--parent");
    if (destIsDir) {
      if (parents) {
        // --parents: use relative source path under destination
        // e.g., cp --parents a/b/c dest/ -> dest/a/b/c
        finalDestPath = destPath + "\\" + normalize_path_separators(srcPath);
        // Create intermediate directories
        size_t pos = 0;
        while ((pos = finalDestPath.find('\\', pos)) != std::string::npos) {
          std::string partial = finalDestPath.substr(0, pos);
          if (!partial.empty() && partial != destPath) {
            (void)create_directory_recursive(partial);
          }
          pos++;
        }
      } else {
        // If destination is a directory, append source filename
        std::wstring wsrcPath = utf8_to_wstring(srcPath);
        LPWSTR fileName = PathFindFileNameW(wsrcPath.c_str());

        finalDestPath += "\\" + wstring_to_utf8(fileName);
      }
    }

    // [GNU] -P/-d/-a never follow a symlink source: the link itself is
    // recreated at the destination, even when it points at a directory.
    if (srcIsDir && is_symlink_path(srcPath) &&
        (ctx.get<bool>("-P", false) ||
         ctx.get<bool>("--no-dereference", false) ||
         ctx.get<bool>("-d", false) || archive_enabled(ctx))) {
      srcIsDir = false;
    }

    if (srcIsDir) {
      if (recursive) {
        auto dirResult = copy_directory(srcPath, finalDestPath, ctx);
        if (!dirResult || !*dirResult) {
          // OPTIMIZED: Avoid wstring concatenation
          safeErrorPrint("cp: error copying directory '");
          safeErrorPrint(srcPath);
          safeErrorPrint("'\n");
          success = false;
        }
      } else {
        // OPTIMIZED: Avoid wstring concatenation
        safeErrorPrint("cp: -r not specified; omitting directory '");
        safeErrorPrint(srcPath);
        safeErrorPrint("'\n");
        success = false;
      }
    } else {
      auto fileResult = copy_file(srcPath, finalDestPath, ctx);
      if (!fileResult) {
        // copy_file errors are already complete GNU-style diagnostics.
        safeErrorPrint("cp: ");
        safeErrorPrint(fileResult.error());
        safeErrorPrint("\n");
        success = false;
      }
    }
  }

  return success;
}

// ----------------------------------------------
// 9. Main pipeline
// ----------------------------------------------
template <size_t N>
auto process_command(const CommandContext<N>& ctx) -> cp::Result<bool> {
  // [DIFFERS] -Z/--context: SELinux does not exist on Windows.  GNU cp on a
  // non-SELinux system accepts -Z as a silent no-op, so scripts passing it
  // must not fail (#995).
  (void)ctx.get<bool>("-Z", false);
  (void)ctx.get<bool>("--context", false);
  (void)ctx.get<bool>("-c", false);

  // [DIFFERS] --keep-directory-symlink: on Windows directory symlinks are
  // followed by default, so this flag is silently accepted as a no-op.
  (void)ctx.get<bool>("--keep-directory-symlink", false);

  // [DIFFERS] -H: follow command-line symbolic links in SOURCE.
  // On Windows, symbolic links are resolved transparently by the file system
  // layer, so this flag is a no-op.
  (void)ctx.get<bool>("-H", false);

  // [DIFFERS] -L/--dereference: always follow symbolic links in SOURCE.
  // On Windows, the default file-open behaviour already follows symlinks,
  // so this flag is a no-op.
  (void)ctx.get<bool>("-L", false);
  (void)ctx.get<bool>("--dereference", false);

  // -P/--no-dereference and -d are handled inside copy_file: a symbolic-link
  // source is recreated as a link at the destination instead of being
  // followed (GNU -d = --no-dereference --preserve=links; -a implies -d).
  // The hardlink-preservation half of --preserve=links has no Windows
  // equivalent worth emulating here.

  // [GNU] -s and -l are mutually exclusive.
  if ((ctx.get<bool>("-s", false) || ctx.get<bool>("--symbolic-link", false)) &&
      (ctx.get<bool>("-l", false) || ctx.get<bool>("--link", false))) {
    return std::unexpected(
        "cannot make both hard and symbolic links\n"
        "Try 'cp --help' for more information.");
  }

  // [DIFFERS] --sparse: control creation of sparse files.
  // Not yet implemented; would require DeviceIoControl(SET_SPARSE).
  if (ctx.has("--sparse")) {
    return std::unexpected("--sparse is not supported on Windows");
  }

  // [DIFFERS] --reflink: control clone/CoW copies.
  // Not yet implemented; would require CopyFile2 or equivalent.
  if (ctx.has("--reflink")) {
    return std::unexpected("--reflink is not supported on Windows");
  }

  // [DIFFERS] --copy-contents: copy contents of special files when recursive.
  // Windows special files (named pipes, device files) differ from POSIX;
  // this flag is not applicable.
  if (ctx.has("--copy-contents")) {
    return std::unexpected("--copy-contents is not supported on Windows");
  }

  // [COMPAT NO-OP] -g/--progress-bar: WinuxCmd extension for progress display.
  (void)ctx.get<bool>("-g", false);
  (void)ctx.get<bool>("--progress-bar", false);

  bool no_clobber =
      ctx.get<bool>("--no-clobber", false) || ctx.get<bool>("-n", false);
  if (no_clobber) {
    // [GNU 9.4] -n is deprecated in favour of --update=none.
    safeErrorPrintLn(
        "cp: warning: behavior of -n is non-portable and may change in "
        "future; use --update=none instead");
  }
  if (no_clobber && backup_enabled(ctx)) {
    // GNU >=9.10 rephrased this diagnostic; the 8.32 wording is kept.
    return std::unexpected(
        "options --backup and --no-clobber are mutually exclusive\n"
        "Try 'cp --help' for more information.");
  }

  bool no_target_directory = ctx.get<bool>("-T", false) ||
                             ctx.get<bool>("--no-target-directory", false);
  return validate_arguments(ctx)
      .and_then([&](std::pair<std::vector<std::string>, std::string> paths) {
        return check_destination(paths, no_target_directory);
      })
      .and_then([&](std::tuple<std::vector<std::string>, std::string, bool>
                        pathsAndDir) {
        return process_source_paths(pathsAndDir, ctx);
      });
}

}  // namespace cp_pipeline

// ======================================================
// Command registration
// ======================================================

REGISTER_COMMAND(
    cp,
    /* name */
    "cp",

    /* synopsis */
    "copy files and directories",

    /* description */
    "Copy SOURCE to DEST, or multiple SOURCE(s) to DIRECTORY.\n"
    "\n"
    "In the first form, copy SOURCE to DEST.\n"
    "In the second form, copy each SOURCE to DIRECTORY.",

    /* examples */
    "  cp file1.txt file2.txt       Copy file1.txt to file2.txt\n"
    "  cp -r dir1 dir2              Recursively copy dir1 to dir2\n"
    "  cp -v file.txt dir/           Verbose copy file.txt to dir/\n"
    "  cp -i file.txt file.txt       Interactive copy (prompt before "
    "overwrite)",

    /* see also */
    "mv(1), rm(1), ln(1)",

    /* author */
    "WinuxCmd",

    /* copyright */
    "Copyright © 2026 WinuxCmd",

    /* options */
    CP_OPTIONS) {
  using namespace cp_pipeline;
  using namespace core::pipeline;

  auto result = process_command(ctx);
  if (!result) {
    report_error(result, L"cp");
    return 1;
  }

  return *result ? 0 : 1;
}
