#pragma once

#include <ll/graphics.h>

namespace mercury::ll::graphics::embedded_shaders {

	// mercury - VS
	mercury::ll::graphics::ShaderBytecodeView TestTriangleVS();

	// mercury - PS
	mercury::ll::graphics::ShaderBytecodeView TestTrianglePS();

} // namespace mercury::ll::graphics::embedded_shaders
