#include "ll/os.h"
#include <mercury_utils.h>

#ifdef MERCURY_LL_OS_ANDROID
#include <android/log.h>
#include <android/native_activity.h>
#include <android/window.h>
#include <iostream>

using namespace mercury;

// (unchanged) UTF conversions...
int utils::string::utf8_to_utf16(const char8_t* utf8_str,
    char16_t* utf16_str, int max_length) {
    return 0; //TODO: implement
}
int utils::string::utf16_to_utf8(const char16_t* utf16_str,
    char8_t* utf8_str, int max_length) {
    return 0; //TODO: implement
}

void utils::debug::output_debug_string_to_ide(logging::Severity severity, const char8_t* str) {
    // Convert UTF-8 to UTF-16 for OutputDebugStringW
    __android_log_print(ANDROID_LOG_DEBUG, "TAG", "%s", (const char*)str);
}

void utils::debug::output_debug_string_to_console(logging::Severity severity, const char8_t* str)
{

}

#endif