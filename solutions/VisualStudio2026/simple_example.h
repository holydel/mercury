#pragma once
#include <ll/graphics.h>

struct SimpleTrinagleRotate
{
	struct PerObjectData
	{
		float rotationAngle;
	};

	mercury::ll::graphics::ShaderBytecodeView VertexShader();
	mercury::ll::graphics::ShaderBytecodeView FragmentShader();
};