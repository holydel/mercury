#include "mercury_utils.h"
#include <sstream>
#include <iomanip>

namespace mercury {
	namespace utils {
		namespace string {

			std::string format_size(u64 size) {
				static const char* units[] = { "", "KB", "MB", "GB", "TB", "PB", "EB" };
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

			std::string from(ShaderStage stage)
			{
				switch (stage)
				{
					case ShaderStage::Unknown: return "Unknown";
					case ShaderStage::Vertex: return "Vertex";
					case ShaderStage::TessellationControl: return "TessControl";
					case ShaderStage::TessellationEvaluation: return "TessEval";
					case ShaderStage::Geometry: return "Geometry";
					case ShaderStage::Fragment: return "Fragment";
					case ShaderStage::Compute: return "Compute";
					case ShaderStage::Task: return "Task";
					case ShaderStage::Mesh: return "Mesh";
					case ShaderStage::RayGeneration: return "RayGen";
					case ShaderStage::Intersection: return "Intersect";
					case ShaderStage::AnyHit: return "AnyHit";
					case ShaderStage::ClosestHit: return "ClosestHit";
					case ShaderStage::Miss: return "Miss";
					case ShaderStage::Callable: return "Callable";
					case ShaderStage::ClusterCulling: return "ClusterCulling";
					case ShaderStage::WorkGraph: return "WorkGraph";
					default: return "Unknown";
				}
			}
		} // namespace string
	} // namespace utils
} // namespace mercury
