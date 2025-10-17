#pragma once

#include <ll/graphics.h>

namespace mercury::ll::graphics::embedded_shaders {

	// mercury - VS
	mercury::ll::graphics::ShaderBytecodeView TestTriangleVS();

	// mercury - PS
	mercury::ll::graphics::ShaderBytecodeView TestTrianglePS();

	// mercury - VS
	mercury::ll::graphics::ShaderBytecodeView DedicatedSpriteVS();

	// mercury - PS
	mercury::ll::graphics::ShaderBytecodeView DedicatedSpriteColorPS();

	// mercury - PS
	mercury::ll::graphics::ShaderBytecodeView DedicatedSpritePS();

} // namespace mercury::ll::graphics::embedded_shaders
