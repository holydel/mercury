#include "mercury_api.h"
#include <string>
namespace mercury {
namespace utils {
namespace math {}
namespace string {
int utf8_to_utf16(const char8_t *utf8_str, char16_t *utf16_str, int max_length);
int utf16_to_utf8(const char16_t *utf16_str, char8_t *utf8_str, int max_length);
} // namespace string
namespace debug {
void output_debug_string_to_ide(const char8_t *str);
}
} // namespace utils
} // namespace mercury