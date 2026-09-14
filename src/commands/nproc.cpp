/*
 *  Copyright © 2026 WinuxCmd
 *
 *  Permission is hereby granted, free of charge, to any person obtaining a copy
 *  of this software and associated documentation files (the "Software"), to
 *  deal in the Software without restriction, including without limitation the
 *  rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 *  sell copies of the Software, and to permit persons to whom the Software is
 *  furnished to do so, subject to the following conditions:
 *
 *  The above copyright notice and this permission notice shall be included in
 *  all copies or substantial portions of the Software.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 *  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 *  IN THE SOFTWARE.
 *
 *  - File: nproc.cpp
 *  - CopyrightYear: 2026
 */
/// @contributors:
///   - caomengxuan666 <2507560089@qq.com>
/// @Description: Implementation for nproc command.
/// @Version: 0.1.0
/// @License: MIT
/// @Copyright: Copyright © 2026 WinuxCmd

#include "pch/pch.h"
// include other header after pch.h
#include "core/command_macros.h"
import std;
import core;
import utils;

using cmd::meta::OptionMeta;
using cmd::meta::OptionType;

auto constexpr NPROC_OPTIONS =
    // [DIFFERS] --all: On Windows, GetSystemInfo always returns the total
    // count, so --all and default behavior are identical.
    std::array{OPTION("", "--all", "print number of all installed processors"),
               // [GNU]
               OPTION("", "--ignore", "ignore N processors", STRING_TYPE)};

namespace {
// [GNU] lib/nproc.c parse_omp_threads(): skip leading blanks, parse a
// positive decimal integer, then accept trailing blanks and a ','-separated
// nesting list. Anything else is treated as if the variable were unset.
auto parse_omp_threads_env(const char* threads) -> unsigned long long {
  if (threads == nullptr) return 0;
  auto is_space = [](char c) {
    return c == ' ' || c == '\t' || c == '\n' || c == '\v' || c == '\f' ||
           c == '\r';
  };
  while (*threads != '\0' && is_space(*threads)) ++threads;
  if (*threads < '0' || *threads > '9') return 0;
  const char* p = threads;
  unsigned long long value = 0;
  while (*p >= '0' && *p <= '9') {
    const unsigned long long digit = static_cast<unsigned long long>(*p - '0');
    // Saturate like strtoul; GNU does not check errno here.
    if (value > (std::numeric_limits<unsigned long long>::max() - digit) / 10) {
      value = std::numeric_limits<unsigned long long>::max();
    } else {
      value = value * 10 + digit;
    }
    ++p;
  }
  while (*p != '\0' && is_space(*p)) ++p;
  if (*p == '\0' || *p == ',') return value;
  return 0;
}

// [GNU] nproc.c parses --ignore with xnumtoumax(base 10, minimum 0): a
// leading '+' is allowed; a sign of '-' or any junk is an "invalid number"
// error.
auto parse_ignore_value(std::string_view text)
    -> std::optional<unsigned long long> {
  const char* p = text.data();
  const char* end = p + text.size();
  if (p < end && *p == '+') ++p;
  const char* digits_begin = p;
  unsigned long long value = 0;
  while (p < end && *p >= '0' && *p <= '9') {
    const unsigned long long digit = static_cast<unsigned long long>(*p - '0');
    if (value > (std::numeric_limits<unsigned long long>::max() - digit) / 10) {
      return std::nullopt;
    }
    value = value * 10 + digit;
    ++p;
  }
  if (p == digits_begin || p != end) return std::nullopt;
  return value;
}
}  // namespace

REGISTER_COMMAND(
    nproc_cmd,
    /* name */
    "nproc",

    /* synopsis */
    "nproc [OPTION]...",
    "Print the number of processing units available.\n"
    "\n"
    "This is useful for scripts that need to know how many parallel\n"
    "processes can be started.",
    "  nproc\n"
    "  nproc --all\n"
    "  nproc --ignore 1",

    /* see also */
    "sysconf(3)", "WinuxCmd", "Copyright © 2026 WinuxCmd", NPROC_OPTIONS) {
  if (!ctx.positionals.empty()) {
    safeErrorPrintLn("nproc: " +
                     winux::i18n::format("common.error.extra_operand",
                                         "extra operand '{}'",
                                         std::string(ctx.positionals.front())));
    safeErrorPrintLn(winux::i18n::format(
        "common.try_help", "Try '{} --help' for more information.", "nproc"));
    return 1;
  }

  std::optional<unsigned long long> ignore;
  const auto ignore_occurrences = ctx.string_occurrences({"--ignore"});
  if (!ignore_occurrences.empty()) {
    const std::string& text = ignore_occurrences.back().value;
    ignore = parse_ignore_value(text);
    if (!ignore.has_value()) {
      safeErrorPrintLn("nproc: " +
                       winux::i18n::format("command.nproc.error.invalid_number",
                                           "invalid number: '{}'", text));
      return 1;
    }
  }

  SYSTEM_INFO sysInfo;
  GetSystemInfo(&sysInfo);
  const DWORD numProcessors = sysInfo.dwNumberOfProcessors;

  unsigned long long nproc = numProcessors;
  if (!ctx.get<bool>("--all", false)) {
    // Number of available processors.  [GNU] lib/nproc.c honors the OpenMP
    // environment variables: a valid OMP_NUM_THREADS replaces the count
    // outright (it is not clamped to the processor count) and
    // OMP_THREAD_LIMIT bounds the result from above.
    unsigned long long omp_thread_limit =
        parse_omp_threads_env(std::getenv("OMP_THREAD_LIMIT"));
    if (omp_thread_limit == 0) {
      omp_thread_limit = std::numeric_limits<unsigned long long>::max();
    }

    const unsigned long long omp_num_threads =
        parse_omp_threads_env(std::getenv("OMP_NUM_THREADS"));
    if (omp_num_threads != 0) {
      nproc = std::min(omp_num_threads, omp_thread_limit);
    } else {
      nproc = std::min(nproc, omp_thread_limit);
    }
  }

  // [GNU] --ignore applies after the OpenMP/--all selection; the result is
  // guaranteed to be at least 1.
  if (ignore.has_value()) {
    if (*ignore < nproc) {
      nproc -= *ignore;
    } else {
      nproc = 1;
    }
  }

  safePrintLn(std::to_string(nproc));
  return 0;
}
