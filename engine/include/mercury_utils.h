#pragma once

#include "mercury_api.h"
#include <string>
#include <uchar.h>

namespace mercury {
namespace utils {
namespace math {}
namespace string {
int utf8_to_utf16(const c8 *utf8_str, c16 *utf16_str, int max_length);
int utf16_to_utf8(const c16 *utf16_str, c8 *utf8_str, int max_length);
} // namespace string
namespace debug {
void output_debug_string_to_ide(const c8 *str);
void output_debug_string_to_console(const c8 *str);
}
} // namespace utils
} // namespace mercury