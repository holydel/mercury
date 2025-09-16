#pragma once

#include <mercury_api.h>

namespace mercury_imgui
{
	void Initialize();
	void Shutdown();
	void BeginFrame(void* cmdList);
	void EndFrame(void* cmdList);
	void Render();
}