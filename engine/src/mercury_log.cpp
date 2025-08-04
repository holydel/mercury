#include "mercury_log.h"
#include "mercury_application.h"
#include "mercury_utils.h"
#include <cstdarg>

namespace mercury {
namespace log {
    void write_message(Severity severity, const c8* format, ...)
    {
        va_list args;
        va_start(args, format);
        char8_t buff[1024];
        vsnprintf((char*)buff, sizeof(buff), (const char*)format, args);
        va_end(args);

        auto &loggerInfo =Application::GetCurrentApplication()->GetConfig().logger;

        if (loggerInfo.logToConsole)
        {
            utils::debug::output_debug_string_to_console(buff);
        }

        if (loggerInfo.logToIDE)
        {
            utils::debug::output_debug_string_to_ide(buff);
        }
    }
}
}
