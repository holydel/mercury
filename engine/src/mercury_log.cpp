#include "mercury_log.h"
#include "mercury_application.h"
#include "mercury_utils.h"
#include <cstdarg>
#include <cstdio>
#include <cstdlib>

namespace mercury {
namespace logging {

    void write_message(Severity severity, const c8* format, ...)
    {
        char8_t textBuff[1024];
        va_list args;
        va_start(args, format);
        vsnprintf((char*)textBuff, sizeof(textBuff), (const char*)format, args);
        va_end(args);

        // Compose final line (prefix + message)
        char8_t finalBuff[1200];
        snprintf((char*)finalBuff, sizeof(finalBuff), "%s", (const char*)textBuff);

        auto& loggerInfo = Application::GetCurrentApplication()->GetConfig().logger;

        if (loggerInfo.logToConsole)
        {
            utils::debug::output_debug_string_to_console(severity, finalBuff);
        }

        if (loggerInfo.logToIDE)
        {
            utils::debug::output_debug_string_to_ide(severity, finalBuff);
        }

        if (severity == Severity::Fatal)
        {
            // Optional: abort or breakpoint
#if defined(MERCURY_LL_OS_WIN32)
            __debugbreak();
#elif defined(MERCURY_LL_OS_EMSCRIPTEN)
            // Emscripten doesn't support debugbreak, use abort instead
            std::abort();
#else
            // Other platforms
            std::abort();
#endif
        }
    }
}
}
