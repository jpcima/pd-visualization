#pragma once
#include <windows.h>
#include <string>

std::string argv_to_commandA(const char *const argv[]);
std::wstring argv_to_commandW(const wchar_t *const argv[]);
std::basic_string<TCHAR> argv_to_command(const TCHAR *const argv[]);
