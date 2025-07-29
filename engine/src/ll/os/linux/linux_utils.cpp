#include "ll/m_os.h"
#include <mercury_utils.h>

#ifdef MERCURY_LL_OS_LINUX
#include <iconv.h>
#include <locale.h>
#include <stdio.h>
#include <string.h>
#include <wchar.h>


int mercury::utils::string::utf8_to_utf16(const char8_t *utf8_str,
                                          char16_t *utf16_str, int max_length) {
  return 0;
}
int mercury::utils::string::utf16_to_utf8(const char16_t *utf16_str,
                                          char8_t *utf8_str, int max_length) {
  return 0;
}

void mercury::utils::debug::output_debug_string_to_ide(const char8_t *str) {
  // Convert UTF-8 to UTF-16 for OutputDebugStringW
  // char16_t buff[1024] = {0};
  // string::utf8_to_utf16(str, buff, 1024);
  // OutputDebugStringW((const wchar_t *)buff);

  // Also output to console with proper encoding
  // SetConsoleOutputCP(CP_UTF8);
  // printf("Debug: %S\n", (const wchar_t *)buff);
}

#endif