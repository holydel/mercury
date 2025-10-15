#pragma once

#include "mercury_api.h"

namespace mercury
{
	namespace ui
	{
		void* CreateFontFromFileTTF(const c8* filename, float size_pixels);
		void* CreateFontFromMemoryTTF(void* font_data, int font_size_bytes, float size_pixels);
		void DestroyFont(void* font);
		void RegenerateFontAtlas();
	}	
}
