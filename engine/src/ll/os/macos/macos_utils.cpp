#include "ll/os.h"
#include <mercury_utils.h>

#ifdef MERCURY_LL_OS_MACOS
#include <iostream>
#include <cstring>
#include <CoreFoundation/CoreFoundation.h>

using namespace mercury;

// UTF-8 to UTF-16 conversion for macOS
int utils::string::utf8_to_utf16(const c8 *utf8_str, c16 *utf16_str, int max_length) {
    if (!utf8_str || !utf16_str || max_length <= 0) {
        return 0;
    }
    
    // Convert UTF-8 to CFString
    CFStringRef cfString = CFStringCreateWithCString(kCFAllocatorDefault, 
                                                    reinterpret_cast<const char*>(utf8_str), 
                                                    kCFStringEncodingUTF8);
    if (!cfString) {
        return 0;
    }
    
    // Get UTF-16 representation
    CFIndex length = CFStringGetLength(cfString);
    if (length >= max_length) {
        CFRelease(cfString);
        return 0;
    }
    
    CFStringGetCharacters(cfString, CFRangeMake(0, length), reinterpret_cast<UniChar*>(utf16_str));
    utf16_str[length] = u'\0';
    
    CFRelease(cfString);
    return static_cast<int>(length + 1);
}

// UTF-16 to UTF-8 conversion for macOS
int utils::string::utf16_to_utf8(const c16 *utf16_str, c8 *utf8_str, int max_length) {
    if (!utf16_str || !utf8_str || max_length <= 0) {
        return 0;
    }
    
    // Convert UTF-16 to CFString
    CFStringRef cfString = CFStringCreateWithCharacters(kCFAllocatorDefault, 
                                                       reinterpret_cast<const UniChar*>(utf16_str), 
                                                       wcslen(reinterpret_cast<const wchar_t*>(utf16_str)));
    if (!cfString) {
        return 0;
    }
    
    // Get UTF-8 representation
    CFIndex length = CFStringGetMaximumSizeForEncoding(CFStringGetLength(cfString), kCFStringEncodingUTF8);
    if (length >= max_length) {
        CFRelease(cfString);
        return 0;
    }
    
    Boolean success = CFStringGetCString(cfString, 
                                        reinterpret_cast<char*>(utf8_str), 
                                        max_length, 
                                        kCFStringEncodingUTF8);
    
    CFRelease(cfString);
    return success ? static_cast<int>(strlen(reinterpret_cast<const char*>(utf8_str)) + 1) : 0;
}

// Debug output to IDE (Xcode console)
void utils::debug::output_debug_string_to_ide(logging::Severity severity, const c8 *str) {
    // On macOS, output to stderr which is captured by Xcode
    std::cerr << reinterpret_cast<const char*>(str) << std::endl;
}

// Debug output to console with color coding
void utils::debug::output_debug_string_to_console(logging::Severity severity, const c8 *str) {
    // ANSI color codes for terminal output
    const char* colorCode = "";
    switch (severity) {
        case logging::Severity::Debug:   colorCode = "\033[36m"; break; // Cyan
        case logging::Severity::Info:    colorCode = "\033[37m"; break; // White
        case logging::Severity::Warning: colorCode = "\033[33m"; break; // Yellow
        case logging::Severity::Error:   colorCode = "\033[31m"; break; // Red
        case logging::Severity::Fatal:   colorCode = "\033[35m"; break; // Magenta
        default:                         colorCode = "\033[37m"; break; // White
    }
    
    std::cout << colorCode << reinterpret_cast<const char*>(str) << "\033[0m" << std::endl;
}

#endif
