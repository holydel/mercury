#include "ll/os.h"
#include <mercury_utils.h>

#ifdef MERCURY_LL_OS_WIN32
#include <Windows.h>
#include <iostream>

using namespace mercury;

// (unchanged) UTF conversions...
int utils::string::utf8_to_utf16(const c8 *utf8_str,
                                          c16 *utf16_str, int max_length) {
  return MultiByteToWideChar(CP_UTF8, 0, (const char *)utf8_str, -1,
                             (wchar_t *)utf16_str, max_length);
}
int utils::string::utf16_to_utf8(const c16 *utf16_str,
                                          c8 *utf8_str, int max_length) {
  return WideCharToMultiByte(CP_UTF8, 0, (const wchar_t *)utf16_str, -1,
                             (char *)utf8_str, max_length, nullptr, nullptr);
}

static WORD mapSeverityToColor(logging::Severity s)
{
    // Base colors
    constexpr WORD FG_BLACK   = 0;
    constexpr WORD FG_RED     = FOREGROUND_RED;
    constexpr WORD FG_GREEN   = FOREGROUND_GREEN;
    constexpr WORD FG_BLUE    = FOREGROUND_BLUE;
    constexpr WORD FG_YELLOW  = FOREGROUND_RED | FOREGROUND_GREEN;
    constexpr WORD FG_WHITE   = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
    constexpr WORD INTENSITY  = FOREGROUND_INTENSITY;

    switch (s)
    {
    case logging::Severity::Debug:   return FG_BLUE | INTENSITY;            // bright blue
    case logging::Severity::Info:    return FG_WHITE;                       // normal white
    case logging::Severity::Warning: return FG_YELLOW | INTENSITY;          // bright yellow
    case logging::Severity::Error:   return FG_RED | INTENSITY;             // bright red
    case logging::Severity::Fatal:   return FG_RED | FOREGROUND_INTENSITY | FOREGROUND_BLUE; // magenta-ish
    default:                         return FG_WHITE;
    }
}

void utils::debug::output_debug_string_to_ide(logging::Severity severity, const c8 *str) {
  // Convert UTF-8 to UTF-16 for OutputDebugStringW
  char16_t buff[1024] = {0};
  int lastChar = string::utf8_to_utf16(str, buff, 1024);
  buff[lastChar - 1] = L'\n';  
  buff[lastChar] = L'\0';
  OutputDebugStringW((const wchar_t *)buff);
}

void utils::debug::output_debug_string_to_console(logging::Severity severity, const c8 *str)
{

    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hConsole == INVALID_HANDLE_VALUE)
    {
        // Fallback plain output
        std::cout << (const char*)str << std::endl;
        return;
    }

    CONSOLE_SCREEN_BUFFER_INFO info{};
    WORD original = 0;
    if (GetConsoleScreenBufferInfo(hConsole, &info))
        original = info.wAttributes;

    SetConsoleTextAttribute(hConsole, mapSeverityToColor(severity));
    std::cout << (const char*)str << std::endl;

    if (original)
        SetConsoleTextAttribute(hConsole, original);
}

#endif