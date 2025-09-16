#include "ll/os.h"
#include <mercury_utils.h>
#include <emscripten.h>
#include <emscripten/bind.h>
#include <iostream>

#ifdef MERCURY_LL_OS_EMSCRIPTEN

// JavaScript interface for Mercury Engine
EM_JS(void, logToConsole, (const char* message), {
  console.log(UTF8ToString(message));
});

EM_JS(void, logErrorToConsole, (const char* message), {
  console.error(UTF8ToString(message));
});

int mercury::utils::string::utf8_to_utf16(const c8 *utf8_str,
                                          c16 *utf16_str, int max_length) {
  // Use EM_JS for better performance than emscripten_run_script
  return 0; // TODO: Implement proper UTF-8 to UTF-16 conversion
}

int mercury::utils::string::utf16_to_utf8(const c16 *utf16_str,
                                          c8 *utf8_str, int max_length) {
  // Use EM_JS for better performance than emscripten_run_script
  return 0; // TODO: Implement proper UTF-16 to UTF-8 conversion
}

void mercury::utils::debug::output_debug_string_to_ide(const c8 *str) {
    logToConsole((const char*)str);
}

void mercury::utils::debug::output_debug_string_to_console(const c8 *str)
{
    std::cout << (const char*)str << std::endl;
}

#endif