#include "ll/os.h"
#include <mercury_utils.h>

#ifdef MERCURY_LL_OS_LINUX
#include <iostream>
#include <cstring>

int mercury::utils::string::utf8_to_utf16(const char8_t *utf8_str,
                                          char16_t *utf16_str, int max_length) {
    // On Linux, we typically work with UTF-8 directly
    // This conversion is simplified as Linux primarily uses UTF-8
    int i = 0;
    const char* src = reinterpret_cast<const char*>(utf8_str);
    while (i < max_length - 1 && *src) {
        utf16_str[i++] = *src++;
    }
    utf16_str[i] = u'\0';
    return i + 1;
}

int mercury::utils::string::utf16_to_utf8(const char16_t *utf16_str,
                                          char8_t *utf8_str, int max_length) {
    // On Linux, we typically work with UTF-8 directly
    // This conversion is simplified as Linux primarily uses UTF-8
    int i = 0;
    while (i < max_length - 1 && utf16_str[i]) {
        utf8_str[i] = static_cast<char8_t>(utf16_str[i]);
        i++;
    }
    utf8_str[i] = '\0';
    return i + 1;
}

void mercury::utils::debug::output_debug_string_to_ide(const char8_t *str) {
    // On Linux, output to stderr which is typically captured by IDEs
    std::cerr << reinterpret_cast<const char*>(str) << std::endl;
}

void mercury::utils::debug::output_debug_string_to_console(const char8_t *str) {
    // On Linux, output to stdout for console
    std::cout << reinterpret_cast<const char*>(str) << std::endl;
}

#endif