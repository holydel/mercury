#pragma once
#include "mercury_api.h"
#include <glm/glm.hpp>
#include "ll/graphics.h"

namespace mercury {
	namespace canvas
	{
		void DrawSprite(glm::vec2 position, glm::vec2 size, glm::vec2 uv0, glm::vec2 uv1, float angle, PackedColor color);

		ll::graphics::BufferHandle GetCanvasScene2DConstantsBuffer();
	}
}