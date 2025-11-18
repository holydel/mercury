#pragma once
#include <mercury_api.h>
#include <glm/glm.hpp>

namespace mercury
{
	namespace geometry
	{
        struct DedicatedStaticMeshVertex
        {
            glm::vec3 position;
            glm::vec3 normal;
            glm::vec3 tangent;
            glm::vec3 binormal;
            glm::vec2 uv0;
            glm::vec2 uv1;
            glm::vec4 color;

            DedicatedStaticMeshVertex()
                : position(0.0f), normal(0.0f, 1.0f, 0.0f), tangent(1.0f, 0.0f, 0.0f),
                binormal(0.0f, 0.0f, 1.0f), uv0(0.0f), uv1(0.0f), color(1.0f) {
            }
        };

        struct StaticMeshData
        {
            std::vector<DedicatedStaticMeshVertex> vertices;
			std::vector<u16> indices;

            void RecomputeTangents();
        };

		StaticMeshData CreatePlaneMesh(float width = 16.0f, float height = 16.0f, u16 subdivisionsWidth = 8, u16 subdivisionsHeight = 8);
		StaticMeshData CreateCubeMesh(float size = 2.0f);
		StaticMeshData CreateGeoSphereMesh(float radius = 1.0f, u8 subdivisions = 4);
	}
}