#pragma once
#include "mercury_api.h"

namespace mercury
{
    enum class ShaderStage : std::uint8_t
    {
		Unknown = 0,
		// Standart pipeline stages
        Vertex,
        TessellationControl,  // Maps to Hull in D3D12, Tessellation Control in Vulkan/Metal
        TessellationEvaluation,  // Maps to Domain in D3D12, Tessellation Evaluation in Vulkan/Metal
        Geometry,
        Fragment,  // Maps to Pixel in D3D12
        Compute,

		// Mesh pipeline stages
        Task,  // Maps to Amplification/Object in D3D12/Metal, Task in Vulkan
        Mesh,

		//Raytracing stages
        RayGeneration,
        Intersection,
        AnyHit,
        ClosestHit,
        Miss,
        Callable,

        ClusterCulling,  // VK_HUAWEI_cluster_culling_shader (Vulkan extension)

        WorkGraph,  // D3D12 Work Graphs node shaders

        MAX
    };

    enum class PipelineType : std::uint8_t
    {
        Unknown = 0,
        
        Rasterization,
        Compute,
        RayTracing,
        Mesh,

        MAX
	};

}