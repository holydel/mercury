#pragma once

#include "mercury_api.h"

namespace mercury {
namespace logging {
    enum class Severity : u8 {
        Debug,
        Info,
        Warning,
        Error,
        Fatal
    };

    void write_message(Severity severity, const c8* format, ...);
}
}

#define MLOG_DEBUG(format, ...) mercury::logging::write_message(mercury::logging::Severity::Debug, format, ##__VA_ARGS__)
#define MLOG_INFO(format, ...) mercury::logging::write_message(mercury::logging::Severity::Info, format, ##__VA_ARGS__)
#define MLOG_WARNING(format, ...) mercury::logging::write_message(mercury::logging::Severity::Warning, format, ##__VA_ARGS__)
#define MLOG_ERROR(format, ...) mercury::logging::write_message(mercury::logging::Severity::Error, format, ##__VA_ARGS__)
#define MLOG_FATAL(format, ...) mercury::logging::write_message(mercury::logging::Severity::Fatal, format, ##__VA_ARGS__)

