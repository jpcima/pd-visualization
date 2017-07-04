#include "self_path.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
# include <windows.h>
# include <shlwapi.h>
#else
# include <dlfcn.h>
#endif

static char library_data;

std::string self_directory() {
#ifdef _WIN32
  HMODULE hmodule = nullptr;
  if (!GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                          GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                          &library_data, &hmodule))
    return nullptr;

  char loc[MAX_PATH];
  GetModuleFileNameA(hmodule, loc, sizeof(loc));
  PathRemoveFileSpecA(loc);
  strcat(loc, "\\");
  return loc;
#else
  const char *loc = nullptr;

  Dl_info info {};
  int ret = dladdr(&library_data, &info);
  if (ret != 0)
    loc = info.dli_fname;

  if (!loc)
    return nullptr;

  std::size_t dirlen = strrchr(loc, '/') - loc;
  return std::string(loc, dirlen);
#endif
}

std::string self_relative(const char *path) {
  std::string libdir = self_directory();
  return self_directory() + '/' + path;
}
