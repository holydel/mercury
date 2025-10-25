#pragma once

#include <ll/graphics.h>

namespace mercury::ll::graphics::embedded_shaders {

	// dedicated_sprite - VS
	mercury::ll::graphics::ShaderBytecodeView DedicatedSpriteVS();

	// dedicated_sprite - PS
	mercury::ll::graphics::ShaderBytecodeView DedicatedSpriteColorPS();

	// dedicated_sprite - PS
	mercury::ll::graphics::ShaderBytecodeView DedicatedSpritePS();

	// mercury - VS
	mercury::ll::graphics::ShaderBytecodeView TestTriangleVS();

	// mercury - VS
	mercury::ll::graphics::ShaderBytecodeView TestTriangleRotatedVS();

	// mercury - PS
	mercury::ll::graphics::ShaderBytecodeView TestTrianglePS();

} // namespace mercury::ll::graphics::embedded_shaders
