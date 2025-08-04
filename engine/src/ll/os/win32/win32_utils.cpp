#include "ll/os.h"
#include <mercury_utils.h>

#ifdef MERCURY_LL_OS_WIN32
#define NOMINMAX
#include <Windows.h>
#include <iostream>

int mercury::utils::string::utf8_to_utf16(const char8_t *utf8_str,
                                          char16_t *utf16_str, int max_length) {
  return MultiByteToWideChar(CP_UTF8, 0, (const char *)utf8_str, -1,
                             (wchar_t *)utf16_str, max_length);
}
int mercury::utils::string::utf16_to_utf8(const char16_t *utf16_str,
                                          char8_t *utf8_str, int max_length) {
  return WideCharToMultiByte(CP_UTF8, 0, (const wchar_t *)utf16_str, -1,
                             (char *)utf8_str, max_length, nullptr, nullptr);
}

void mercury::utils::debug::output_debug_string_to_ide(const char8_t *str) {
  // Convert UTF-8 to UTF-16 for OutputDebugStringW
  char16_t buff[1024] = {0};
  string::utf8_to_utf16(str, buff, 1024);
  OutputDebugStringW((const wchar_t *)buff);
  
  // Also output to console with proper encoding
  // SetConsoleOutputCP(CP_UTF8);
  printf("Debug: %S\n", (const wchar_t *)buff);
}

void mercury::utils::debug::output_debug_string_to_console(const char8_t *str)
{
    std::cout << (const char*)str << std::endl;
}

#endif