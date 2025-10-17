#include "ll/graphics.h"
#include "mercury_shader.h"
#include "mercury_api.h"

#ifdef MERCURY_LL_GRAPHICS_METAL

using namespace mercury::ll::graphics;

// Metal embedded shaders - placeholders for now
// These would contain actual Metal shader bytecode in a real implementation

namespace mercury::ll::graphics::embedded_shaders {

ShaderBytecodeView TestTriangleVS() {
    // Placeholder for Metal vertex shader
    static const char* shaderSource = R"(
        #include <metal_stdlib>
        using namespace metal;
        
        struct VertexOut {
            float4 position [[position]];
            float4 color;
        };
        
        vertex VertexOut vs_main(uint vertexID [[vertex_id]]) {
            VertexOut out;
            float2 positions[3] = {
                float2(0.0, 0.5),
                float2(-0.5, -0.5),
                float2(0.5, -0.5)
            };
            float4 colors[3] = {
                float4(1.0, 0.0, 0.0, 1.0),
                float4(0.0, 1.0, 0.0, 1.0),
                float4(0.0, 0.0, 1.0, 1.0)
            };
            out.position = float4(positions[vertexID], 0.0, 1.0);
            out.color = colors[vertexID];
            return out;
        }
    )";
    
    ShaderBytecodeView view;
    view.data = reinterpret_cast<const mercury::u8*>(shaderSource);
    view.size = strlen(shaderSource);
    return view;
}

ShaderBytecodeView TestTrianglePS() {
    // Placeholder for Metal fragment shader
    static const char* shaderSource = R"(
        #include <metal_stdlib>
        using namespace metal;
        
        struct VertexOut {
            float4 position [[position]];
            float4 color;
        };
        
        fragment float4 ps_main(VertexOut in [[stage_in]]) {
            return in.color;
        }
    )";
    
    ShaderBytecodeView view;
    view.data = reinterpret_cast<const mercury::u8*>(shaderSource);
    view.size = strlen(shaderSource);
    return view;
}

} // namespace embedded_shaders

#endif // MERCURY_LL_GRAPHICS_METAL