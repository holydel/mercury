#pragma once
#include "mercury_api.h"
#include <glm/glm.hpp>
#include "ll/graphics.h"

namespace mercury {
	namespace canvas
	{
		void DrawSprite(glm::vec2 position, glm::vec2 size, glm::vec2 uv0, glm::vec2 uv1, float angle, PackedColor color);
		void DrawSprite(ll::graphics::TextureHandle texture, glm::vec2 position, glm::vec2 size, glm::vec2 uv0, glm::vec2 uv1, float angle, PackedColor color);

		ll::graphics::BufferHandle GetCanvasScene2DConstantsBuffer();

		void DrawDedicatedMesh(ll::graphics::BufferHandle vertexBuffer, u32 numVertices, glm::mat4 MVP);
		void DrawDedicatedIndexedMesh(ll::graphics::BufferHandle vertexBuffer, ll::graphics::BufferHandle indexBuffer, u32 numIndices, glm::mat4 MVP);
	}
}