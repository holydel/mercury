#include "mercury_utils.h"
#include <sstream>
#include <iomanip>

namespace mercury {
namespace utils {
namespace string {

std::string format_size(u64 size) {
    static const char* units[] = {"", "KB", "MB", "GB", "TB", "PB", "EB"};
    if (size < 1000ULL) {
        return std::to_string(size);
    }

    double value = static_cast<double>(size);
    int idx = 0;
    const int unit_count = static_cast<int>(sizeof(units) / sizeof(units[0]));
    while (value >= 1000.0 && idx < unit_count - 1) {
        value /= 1000.0;
        ++idx;
    }

    std::ostringstream oss;
    oss.setf(std::ios::fixed);
    oss << std::setprecision(2) << value;
    std::string s = oss.str();

    // Trim trailing zeros and optional dot
    auto dot = s.find('.');
    if (dot != std::string::npos) {
        size_t end = s.size();
        while (end > dot + 1 && s[end - 1] == '0') --end;
        if (end > dot && s[end - 1] == '.') --end;
        s.erase(end);
    }

    if (units[idx][0] != '\0') {
        s += " ";
        s += units[idx];
    }
    return s;
}

} // namespace string
} // namespace utils
} // namespace mercury
