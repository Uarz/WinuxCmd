/*
 *  Copyright (c) 2026 [caomengxuan666]
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
 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 *
 *  - File: winuxcmd_unit_test.cpp
 *  - Username: Administrator
 *  - CopyrightYear: 2026
 */
#include "framework/winuxtest.h"

TEST(winuxcmd, winuxcmd_help_alias_shows_toplevel_help) {
  Pipeline p;
  p.add(L"winuxcmd.exe", {L"help"});
  auto r = p.run();

  EXPECT_EQ(r.exit_code, 1);
  EXPECT_TRUE(
      r.stdout_text.find("WinuxCmd - Windows Compatible Linux Command Set") !=
      std::string::npos);
}

TEST(winuxcmd, winuxcmd_dash_h_is_not_help_alias) {
  Pipeline p;
  p.add(L"winuxcmd.exe", {L"-h"});
  auto r = p.run();

  EXPECT_EQ(r.exit_code, 127);
  EXPECT_TRUE(r.stdout_text.empty());
  EXPECT_TRUE(r.stderr_text.find("winuxcmd: command not found: -h") !=
              std::string::npos);
}

TEST(winuxcmd, winuxcmd_help_command_topic_shows_command_help) {
  Pipeline p;
  p.add(L"winuxcmd.exe", {L"help", L"sort"});
  auto r = p.run();

  EXPECT_EQ(r.exit_code, 0);
  EXPECT_TRUE(r.stdout_text.find("Usage: sort [OPTION]... [FILE]...") !=
              std::string::npos);
  EXPECT_TRUE(r.stdout_text.find("--compress-program") != std::string::npos);
}

TEST(winuxcmd, winuxcmd_help_works_for_positional_sentinel_options) {
  // Commands whose only OptionMeta is a positional sentinel (empty short and
  // long names, non-empty description) used to make print_help throw
  // std::out_of_range on opt.short_name.substr(1) and print nothing (#1065).
  for (const wchar_t* cmd : {L"printf", L"yes", L"tsort", L"sleep"}) {
    Pipeline p;
    p.add(cmd, {L"--help"});
    auto r = p.run();

    EXPECT_EQ(r.exit_code, 0);
    EXPECT_FALSE(r.stdout_text.empty());
    EXPECT_TRUE(r.stdout_text.find("Usage:") != std::string::npos);
  }
}

TEST(winuxcmd, winuxcmd_trailing_separator_on_file_fails_safely) {
  // #1052: a trailing separator requires a directory target. On a regular
  // file it must fail without opening/truncating the file (data loss).
  TempDir tmp;
  tmp.write("reg.txt", "CONTENT\n");
  tmp.write("src.txt", "SRC\n");

  {
    Pipeline p;
    p.set_cwd(tmp.wpath());
    p.add(L"cat.exe", {L"reg.txt/"});
    auto r = p.run();
    EXPECT_NE(r.exit_code, 0);
    EXPECT_TRUE(r.stderr_text.find("Not a directory") != std::string::npos);
  }
  {
    Pipeline p;
    p.set_cwd(tmp.wpath());
    p.set_stdin("x\n");
    p.add(L"tee.exe", {L"reg.txt/"});
    auto r = p.run();
    EXPECT_NE(r.exit_code, 0);
    EXPECT_FALSE(r.stderr_text.empty());
  }
  {
    Pipeline p;
    p.set_cwd(tmp.wpath());
    p.add(L"cp.exe", {L"src.txt", L"reg.txt/"});
    auto r = p.run();
    EXPECT_NE(r.exit_code, 0);
    EXPECT_FALSE(r.stderr_text.empty());
  }
  {
    Pipeline p;
    p.set_cwd(tmp.wpath());
    p.add(L"rm.exe", {L"reg.txt/"});
    auto r = p.run();
    EXPECT_NE(r.exit_code, 0);
  }

  EXPECT_EQ(tmp.read("reg.txt"), "CONTENT\n");
}

TEST(winuxcmd, winuxcmd_trailing_separator_on_directory_still_works) {
  // #1052 must not break valid directory operands: cp into "dir/" copies
  // inside it, and "mkdir newdir/" still creates the directory.
  TempDir tmp;
  tmp.write("src.txt", "SRC\n");
  std::filesystem::create_directory(tmp.path / "realdir");

  {
    Pipeline p;
    p.set_cwd(tmp.wpath());
    p.add(L"cp.exe", {L"src.txt", L"realdir/"});
    auto r = p.run();
    EXPECT_EQ(r.exit_code, 0);
    EXPECT_EQ(tmp.read("realdir/src.txt"), "SRC\n");
  }
  {
    Pipeline p;
    p.set_cwd(tmp.wpath());
    p.add(L"mkdir.exe", {L"newdir/"});
    auto r = p.run();
    EXPECT_EQ(r.exit_code, 0);
    EXPECT_TRUE(std::filesystem::is_directory(tmp.path / "newdir"));
  }
}

TEST(winuxcmd, winuxcmd_unknown_command_does_not_fallback_to_shell) {
  Pipeline p;
  p.add(L"winuxcmd.exe", {L"definitely-not-a-winuxcmd-command"});
  auto r = p.run();

  EXPECT_EQ(r.exit_code, 127);
  EXPECT_TRUE(r.stdout_text.empty());
  EXPECT_TRUE(
      r.stderr_text.find(
          "winuxcmd: command not found: definitely-not-a-winuxcmd-command") !=
      std::string::npos);
}
