#include "win32_argv.h"

template <class C>
std::basic_string<C> argv_to_commandG(const C *const argv[]) {
  std::basic_string<C> cmd;
  cmd.reserve(1024);
  for (const C *arg; (arg = *argv); ++argv) {
    if (!cmd.empty())
      cmd.push_back(' ');
    cmd.push_back('"');
    for (C c;; ++arg) {
      unsigned numbackslash = 0;
      while ((c = *arg) == '\\') {
        ++arg;
        ++numbackslash;
      }
      if (!c) {
        cmd.append(numbackslash * 2, '\\');
        break;
      }
      if (c == '"')
        cmd.append(numbackslash * 2 + 1, '\\');
      else
        cmd.append(numbackslash, '\\');
      cmd.push_back(c);
    }
    cmd.push_back('"');
  }
  return cmd;
}

std::string argv_to_commandA(const char *const argv[]) {
  return argv_to_commandG<char>(argv);
}

std::wstring argv_to_commandW(const wchar_t *const argv[]) {
  return argv_to_commandG<wchar_t>(argv);
}

std::basic_string<TCHAR> argv_to_command(const TCHAR *const argv[]) {
  return argv_to_commandG<TCHAR>(argv);
}
