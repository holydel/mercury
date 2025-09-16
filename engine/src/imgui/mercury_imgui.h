#pragma once

#include <mercury_api.h>
#include <ll/graphics.h>
namespace mercury_imgui
{
	void Initialize();
	void Shutdown();
	void BeginFrame(mercury::ll::graphics::CommandList cmdList);
	void EndFrame(mercury::ll::graphics::CommandList cmdList);
	void Render();
}