/* vim:set ts=2 sw=2 sts=2 et: */
/**
 * \author     Marcus Holland-Moritz (github@mhxnet.de)
 * \copyright  Copyright (c) Marcus Holland-Moritz
 *
 * This file is part of dwarfs.
 *
 * dwarfs is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * dwarfs is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with dwarfs.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <cerrno>
#include <clocale>
#include <cstdlib>
#include <iostream>

#include <folly/String.h>
#include <folly/experimental/symbolizer/SignalHandler.h>

#ifdef DWARFS_USE_EXCEPTION_TRACER
#include <folly/experimental/exception_tracer/ExceptionTracer.h>
#endif

#include "dwarfs/error.h"
#include "dwarfs/terminal.h"

namespace dwarfs {

system_error::system_error(char const* file, int line) noexcept
    : system_error(errno, file, line) {}

system_error::system_error(std::string const& s, char const* file,
                           int line) noexcept
    : system_error(s, errno, file, line) {}

system_error::system_error(std::string const& s, int err, char const* file,
                           int line) noexcept
    : std::system_error(err, std::generic_category(), s.c_str())
    , file_(file)
    , line_(line) {}

system_error::system_error(int err, char const* file, int line) noexcept
    : std::system_error(err, std::generic_category())
    , file_(file)
    , line_(line) {}

void dump_exceptions() {
#ifdef DWARFS_USE_EXCEPTION_TRACER
  auto exceptions = ::folly::exception_tracer::getCurrentExceptions();
  for (auto& exc : exceptions) {
    std::cerr << exc << "\n";
  }
#else
  std::cerr << "cannot dump exceptions\n";
#endif
}

void handle_nothrow(char const* expr, char const* file, int line) {
  std::cerr << "Expression `" << expr << "` threw `"
            << folly::exceptionStr(std::current_exception()) << "` in " << file
            << "(" << line << ")\n";
  ::abort();
}

void assertion_failed(char const* expr, std::string const& msg,
                      char const* file, int line) {
  std::cerr << "Assertion `" << expr << "` failed in " << file << "(" << line
            << "): " << msg << "\n";
  ::abort();
}

int safe_main(std::function<int(void)> fn) {
  try {
#ifndef _WIN32
    folly::symbolizer::installFatalSignalHandler();
#endif
#ifdef _WIN32
    char const* locale = "en_US.utf8";
#else
    char const* locale = "";
#endif
    try {
      std::locale::global(std::locale(locale));
    } catch (std::exception const& e) {
      std::cerr << "warning: failed to set user default locale\n";
      try {
        std::locale::global(std::locale::classic());
      } catch (std::exception const& e) {
        std::cerr << "warning: also failed to set classic locale\n";
      }
    }

    if (!std::setlocale(LC_ALL, locale)) {
      std::cerr << "warning: setlocale(LC_ALL, \"\") failed\n";
    }

    setup_terminal();

    return fn();
  } catch (system_error const& e) {
    std::cerr << "ERROR: " << folly::exceptionStr(e) << " [" << e.file() << ":"
              << e.line() << "]\n";
    dump_exceptions();
  } catch (error const& e) {
    std::cerr << "ERROR: " << folly::exceptionStr(e) << " [" << e.file() << ":"
              << e.line() << "]\n";
    dump_exceptions();
  } catch (std::exception const& e) {
    std::cerr << "ERROR: " << folly::exceptionStr(e) << "\n";
    dump_exceptions();
  } catch (...) {
    dump_exceptions();
  }
  return 1;
}

} // namespace dwarfs
