#pragma once

#include "mercury_api.h"

namespace mercury {
namespace log {
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

#define MLOG_DEBUG(format, ...) mercury::log::write_message(mercury::log::Severity::Debug, format, ##__VA_ARGS__)
#define MLOG_INFO(format, ...) mercury::log::write_message(mercury::log::Severity::Info, format, ##__VA_ARGS__)
#define MLOG_WARNING(format, ...) mercury::log::write_message(mercury::log::Severity::Warning, format, ##__VA_ARGS__)
#define MLOG_ERROR(format, ...) mercury::log::write_message(mercury::log::Severity::Error, format, ##__VA_ARGS__)
#define MLOG_FATAL(format, ...) mercury::log::write_message(mercury::log::Severity::Fatal, format, ##__VA_ARGS__)

