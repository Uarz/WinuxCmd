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
 */
module;

#include "pch/pch.h"

export module core:plugins;

import std;
import utils;
import version;

namespace winux::plugins {
namespace fs = std::filesystem;

struct ScriptLaunch {
  std::optional<std::wstring> application;
  std::wstring command_line;
  fs::path working_dir;
};

struct ProcessResult {
  DWORD exit_code = 1;
  std::string output;
};

struct ResolvedCommand {
  std::string name;
  std::string plugin_name;
  std::string plugin_version;
  std::string description;
  std::string alias_of;
  fs::path payload;
  fs::path working_dir;
};

struct PluginRecord {
  std::string name;
  std::string version;
  std::string description;
  std::string entry;
  fs::path root;
  fs::path entry_path;
  std::vector<std::string> commands;
  std::unordered_map<std::string, std::string> aliases;
  std::vector<std::string> path_additions;
};

auto lower_ascii(std::string s) -> std::string {
  std::ranges::transform(s, s.begin(), [](unsigned char ch) {
    return static_cast<char>(std::tolower(ch));
  });
  return s;
}

auto get_env_w(const wchar_t *name) -> std::optional<std::wstring> {
  DWORD needed = GetEnvironmentVariableW(name, nullptr, 0);
  if (needed == 0) return std::nullopt;
  std::wstring value(needed, L'\0');
  DWORD written = GetEnvironmentVariableW(name, value.data(), needed);
  if (written == 0) return std::nullopt;
  if (!value.empty() && value.back() == L'\0') value.pop_back();
  return value;
}

auto set_env_utf8(std::string_view key, std::string_view value) -> void {
  const std::wstring wkey = utf8_to_wstring(std::string(key));
  const std::wstring wvalue = utf8_to_wstring(std::string(value));
  SetEnvironmentVariableW(wkey.c_str(), wvalue.c_str());
}

auto split_path_list(std::wstring_view value) -> std::vector<std::wstring> {
  std::vector<std::wstring> parts;
  size_t start = 0;
  while (start <= value.size()) {
    const size_t end = value.find(L';', start);
    const size_t len = end == std::wstring_view::npos ? value.size() - start
                                                      : end - start;
    if (len > 0) parts.emplace_back(value.substr(start, len));
    if (end == std::wstring_view::npos) break;
    start = end + 1;
  }
  return parts;
}

auto prepend_path_entries(const std::vector<std::wstring> &entries) -> void {
  auto current = get_env_w(L"PATH").value_or(L"");
  std::vector<std::wstring> combined;
  combined.reserve(entries.size() + 16);
  std::unordered_set<std::wstring> seen;
  auto add = [&](const std::wstring &path) {
    if (path.empty()) return;
    std::wstring normalized = path;
    std::ranges::transform(normalized, normalized.begin(), [](wchar_t ch) {
      return static_cast<wchar_t>(std::towlower(ch));
    });
    if (seen.insert(normalized).second) combined.push_back(path);
  };
  for (const auto &entry : entries) add(entry);
  for (const auto &part : split_path_list(current)) add(part);
  std::wstring rebuilt;
  for (size_t i = 0; i < combined.size(); ++i) {
    if (i != 0) rebuilt.push_back(L';');
    rebuilt += combined[i];
  }
  SetEnvironmentVariableW(L"PATH", rebuilt.c_str());
}

auto read_text_file(const fs::path &path) -> std::optional<std::string> {
  std::ifstream input(path, std::ios::binary);
  if (!input.is_open()) return std::nullopt;
  return std::string(std::istreambuf_iterator<char>(input),
                     std::istreambuf_iterator<char>{});
}

auto launch_process(const ScriptLaunch &launch, bool capture_stdout)
    -> std::optional<ProcessResult> {
  SECURITY_ATTRIBUTES sa{};
  sa.nLength = sizeof(sa);
  sa.bInheritHandle = TRUE;
  sa.lpSecurityDescriptor = nullptr;

  HANDLE read_pipe = INVALID_HANDLE_VALUE;
  HANDLE write_pipe = INVALID_HANDLE_VALUE;
  if (capture_stdout) {
    if (!CreatePipe(&read_pipe, &write_pipe, &sa, 0)) return std::nullopt;
    SetHandleInformation(read_pipe, HANDLE_FLAG_INHERIT, 0);
  }

  STARTUPINFOW si{};
  si.cb = sizeof(si);
  if (capture_stdout) {
    si.dwFlags |= STARTF_USESTDHANDLES;
    si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
    si.hStdOutput = write_pipe;
    si.hStdError = write_pipe;
  }

  std::wstring mutable_command = launch.command_line;
  PROCESS_INFORMATION pi{};
  BOOL ok = CreateProcessW(
      launch.application.has_value() ? launch.application->c_str() : nullptr,
      mutable_command.data(), nullptr, nullptr, TRUE, 0, nullptr,
      launch.working_dir.empty() ? nullptr : launch.working_dir.c_str(), &si,
      &pi);
  if (capture_stdout) CloseHandle(write_pipe);
  if (!ok) {
    if (capture_stdout && read_pipe != INVALID_HANDLE_VALUE) CloseHandle(read_pipe);
    return std::optional<ProcessResult>{};
  }

  WaitForSingleObject(pi.hProcess, INFINITE);
  DWORD exit_code = 1;
  GetExitCodeProcess(pi.hProcess, &exit_code);

  std::string output;
  if (capture_stdout && read_pipe != INVALID_HANDLE_VALUE) {
    char buffer[4096];
    DWORD read = 0;
    while (ReadFile(read_pipe, buffer, sizeof(buffer), &read, nullptr) &&
           read > 0) {
      output.append(buffer, buffer + read);
    }
    CloseHandle(read_pipe);
  }

  CloseHandle(pi.hThread);
  CloseHandle(pi.hProcess);
  return ProcessResult{exit_code, std::move(output)};
}

auto powershell_path() -> std::wstring { return L"powershell.exe"; }
auto cmd_path() -> std::wstring {
  wchar_t buffer[MAX_PATH]{};
  DWORD size = GetEnvironmentVariableW(L"COMSPEC", buffer, MAX_PATH);
  if (size > 0 && size < MAX_PATH) return std::wstring(buffer, size);
  return L"cmd.exe";
}

auto build_script_launch(const fs::path &script, std::span<std::string_view> args,
                         [[maybe_unused]] bool capture_stdout) -> ScriptLaunch {
  const std::wstring script_w = script.wstring();
  const std::string ext = lower_ascii(script.extension().string());
  if (ext == ".ps1") {
    std::wstring command = L"-NoProfile -ExecutionPolicy Bypass -File ";
    command += quote_windows_command_arg(script_w);
    for (auto arg : args) {
      append_windows_command_arg(command, utf8_to_wstring(std::string(arg)));
    }
    return {powershell_path(), std::move(command), script.parent_path()};
  }

  std::wstring inner = quote_windows_command_arg(script_w);
  for (auto arg : args) {
    append_windows_command_arg(inner, utf8_to_wstring(std::string(arg)));
  }
  std::wstring command = L"/d /s /c ";
  command += quote_windows_command_arg(inner);
  return {cmd_path(), std::move(command), script.parent_path()};
}

auto launch_script(const fs::path &script, std::span<std::string_view> args,
                   bool capture_stdout) -> std::optional<ProcessResult> {
  return launch_process(build_script_launch(script, args, capture_stdout),
                        capture_stdout);
}

auto resolve_payload_path(const fs::path &root, std::string_view command)
    -> fs::path {
  const std::array<std::wstring_view, 4> dirs{
      L"commands", L"bin", L"examples", L""};
  const std::array<std::wstring_view, 4> exts{L".exe", L".cmd", L".bat",
                                              L".ps1"};
  const fs::path name{std::string(command)};
  for (const auto dir : dirs) {
    for (const auto ext : exts) {
      fs::path candidate = root;
      if (!dir.empty()) candidate /= dir;
      candidate /= name;
      candidate.replace_extension(ext);
      std::error_code ec;
      if (fs::is_regular_file(candidate, ec)) return candidate;
    }
  }
  return {};
}

auto split_semicolon_list(std::wstring_view value) -> std::vector<std::wstring> {
  return split_path_list(value);
}

auto plugin_roots() -> std::vector<fs::path> {
  std::vector<fs::path> roots;
  auto add = [&](const std::optional<std::wstring> &base, std::wstring_view tail) {
    if (!base.has_value() || base->empty()) return;
    roots.emplace_back(fs::path(*base) / tail);
  };
  add(get_env_w(L"LOCALAPPDATA"), L"WinuxCmd/plugins");
  add(get_env_w(L"PROGRAMDATA"), L"WinuxCmd/plugins");
  if (auto extra = get_env_w(L"WINUXCMD_PLUGIN_PATH"); extra.has_value()) {
    for (const auto &entry : split_semicolon_list(*extra)) {
      if (!entry.empty()) roots.emplace_back(entry);
    }
  }
  return roots;
}

struct PluginState {
  bool initialized = false;
  std::vector<PluginRecord> plugins;
  std::vector<ResolvedCommand> canonical_commands;
  std::unordered_map<std::string, size_t> command_index;
};

auto &state() {
  static PluginState instance;
  return instance;
}

auto load_manifest(const fs::path &manifest_path) -> std::optional<PluginRecord> {
  auto text = read_text_file(manifest_path);
  if (!text.has_value()) return std::nullopt;
  try {
    auto manifest = nlohmann::json::parse(*text);
    if (!manifest.is_object() || manifest.value("schema", 0) != 1) {
      return std::nullopt;
    }

    PluginRecord plugin;
    plugin.name = lower_ascii(manifest.value("name", std::string{}));
    plugin.version = manifest.value("version", std::string{});
    plugin.description = manifest.value("description", std::string{});
    plugin.entry = manifest.value("entry", std::string{});
    plugin.root = manifest_path.parent_path();
    if (plugin.name.empty() || plugin.version.empty() || plugin.entry.empty()) {
      return std::nullopt;
    }

    if (manifest.contains("commands") && manifest["commands"].is_array()) {
      for (const auto &item : manifest["commands"]) {
        if (item.is_string()) plugin.commands.push_back(lower_ascii(item.get<std::string>()));
      }
    }

    if (manifest.contains("aliases") && manifest["aliases"].is_object()) {
      for (auto it = manifest["aliases"].begin(); it != manifest["aliases"].end(); ++it) {
        if (it.value().is_string()) {
          plugin.aliases.emplace(lower_ascii(it.key()), lower_ascii(it.value().get<std::string>()));
        }
      }
    }

    plugin.entry_path = plugin.root / plugin.entry;
    return plugin;
  } catch (const std::exception &) {
    return std::nullopt;
  }
}

auto apply_startup_json(const PluginRecord &plugin, const std::string &output)
    -> void {
  try {
    auto result = nlohmann::json::parse(output);
    if (!result.is_object()) return;
    auto env = result.value("env", nlohmann::json::object());
    if (!env.is_object()) return;

    std::vector<std::wstring> path_additions;
    for (auto it = env.begin(); it != env.end(); ++it) {
      if (it.key() == "PATH_ADD" && it.value().is_array()) {
        for (const auto &entry : it.value()) {
          if (entry.is_string()) {
            path_additions.push_back(utf8_to_wstring(entry.get<std::string>()));
          }
        }
        continue;
      }
      if (it.value().is_string()) {
        set_env_utf8(it.key(), it.value().get<std::string>());
      }
    }
    if (!path_additions.empty()) prepend_path_entries(path_additions);
    (void)plugin;
  } catch (const std::exception &) {
  }
}

auto load_plugin(const fs::path &manifest_path) -> void {
  auto plugin = load_manifest(manifest_path);
  if (!plugin.has_value()) return;

  if (fs::exists(plugin->entry_path)) {
    if (auto startup = launch_script(plugin->entry_path, std::span<std::string_view>{}, true); startup.has_value()) {
      if (startup->exit_code == 0 && !startup->output.empty()) {
        apply_startup_json(*plugin, startup->output);
      }
    }
  }

  PluginState &st = state();
  st.plugins.push_back(*plugin);

  auto register_command = [&](const std::string &command_name,
                              const std::string &alias_of = std::string{}) {
    const std::string key = lower_ascii(command_name);
    if (st.command_index.contains(key)) return;
    const fs::path payload = resolve_payload_path(plugin->root, command_name);
    if (payload.empty()) return;

    ResolvedCommand record;
    record.name = key;
    record.plugin_name = plugin->name;
    record.plugin_version = plugin->version;
    record.description = plugin->description.empty() ? plugin->name
                                                     : plugin->description;
    record.alias_of = alias_of;
    record.payload = payload;
    record.working_dir = plugin->root;
    st.command_index.emplace(key, st.canonical_commands.size());
    st.canonical_commands.push_back(std::move(record));
  };

  for (const auto &command_name : plugin->commands) {
    register_command(command_name);
  }

  for (const auto &[alias, target] : plugin->aliases) {
    if (st.command_index.contains(alias)) continue;
    const auto it = st.command_index.find(target);
    if (it == st.command_index.end()) continue;
    st.command_index.emplace(alias, it->second);
  }

}

struct CommandLaunch {
  std::optional<std::wstring> application;
  std::wstring command_line;
  fs::path working_dir;
};

auto build_launch(const fs::path &payload,
                  std::span<std::string_view> args) -> CommandLaunch {
  const std::string ext = lower_ascii(payload.extension().string());
  if (ext == ".exe") {
    std::wstring command = quote_windows_command_arg(payload.wstring());
    for (auto arg : args) {
      append_windows_command_arg(command, utf8_to_wstring(std::string(arg)));
    }
    return {std::nullopt, std::move(command), payload.parent_path()};
  }

  if (ext == ".ps1") {
    std::wstring command = L"-NoProfile -ExecutionPolicy Bypass -File ";
    command += quote_windows_command_arg(payload.wstring());
    for (auto arg : args) {
      append_windows_command_arg(command, utf8_to_wstring(std::string(arg)));
    }
    return {powershell_path(), std::move(command), payload.parent_path()};
  }

  std::wstring inner = quote_windows_command_arg(payload.wstring());
  for (auto arg : args) {
    append_windows_command_arg(inner, utf8_to_wstring(std::string(arg)));
  }
  std::wstring command = L"/d /s /c ";
  command += quote_windows_command_arg(inner);
  return {cmd_path(), std::move(command), payload.parent_path()};
}

auto run_launch(const CommandLaunch &launch) -> std::optional<int> {
  std::wstring mutable_command = launch.command_line;
  STARTUPINFOW si{};
  si.cb = sizeof(si);
  PROCESS_INFORMATION pi{};
  BOOL ok = CreateProcessW(
      launch.application.has_value() ? launch.application->c_str() : nullptr,
      mutable_command.data(), nullptr, nullptr, TRUE, 0, nullptr,
      launch.working_dir.empty() ? nullptr : launch.working_dir.c_str(), &si,
      &pi);
  if (!ok) {
    safeErrorPrintLn("winuxcmd: failed to launch plugin command");
    return std::optional<int>(126);
  }
  WaitForSingleObject(pi.hProcess, INFINITE);
  DWORD exit_code = 1;
  GetExitCodeProcess(pi.hProcess, &exit_code);
  CloseHandle(pi.hThread);
  CloseHandle(pi.hProcess);
  return static_cast<int>(exit_code);
}

}  // namespace

export class PluginRegistry {
 public:
  static void initialize() {
    auto &st = state();
    if (st.initialized) return;
    st.initialized = true;

    for (const auto &root : plugin_roots()) {
      std::error_code ec;
      if (!fs::exists(root, ec)) continue;
      for (const auto &entry : fs::directory_iterator(root, ec)) {
        if (ec) break;
        if (!entry.is_directory()) continue;
        const fs::path manifest = entry.path() / "plugin.json";
        if (fs::exists(manifest)) load_plugin(manifest);
      }
    }
  }

  static bool hasCommand(std::string_view name) noexcept {
    auto &st = state();
    return st.command_index.contains(lower_ascii(std::string(name)));
  }

  static std::vector<std::pair<std::string, std::string>> getAllCommands() {
    auto &st = state();
    std::vector<std::pair<std::string, std::string>> out;
    out.reserve(st.canonical_commands.size());
    for (const auto &cmd : st.canonical_commands) {
      out.emplace_back(cmd.name, cmd.plugin_name + ": " + cmd.description);
    }
    std::sort(out.begin(), out.end(), [](const auto &a, const auto &b) {
      return a.first < b.first;
    });
    return out;
  }

  static std::optional<int> dispatch(std::string_view name,
                                     std::span<std::string_view> args) {
    auto &st = state();
    const auto key = lower_ascii(std::string(name));
    auto it = st.command_index.find(key);
    if (it == st.command_index.end()) return std::nullopt;
    const auto &cmd = st.canonical_commands[it->second];
    auto launch = build_launch(cmd.payload, args);
    return run_launch(launch);
  }

  static bool printHelp(std::string_view name) {
    auto &st = state();
    const auto key = lower_ascii(std::string(name));
    auto it = st.command_index.find(key);
    if (it == st.command_index.end()) return false;
    const auto &cmd = st.canonical_commands[it->second];
    std::string help;
    help += cmd.name;
    help += " (plugin: ";
    help += cmd.plugin_name;
    help += ")\n";
    help += "Version: ";
    help += cmd.plugin_version;
    help += "\n";
    help += "Payload: ";
    help += wstring_to_utf8(cmd.payload.wstring());
    help += "\n";
    help += "Description: ";
    help += cmd.description;
    help += "\n";
    safePrintLn(utf8_to_wstring(help));
    return true;
  }
};

}  // namespace winux::plugins

export using PluginRegistry = winux::plugins::PluginRegistry;
