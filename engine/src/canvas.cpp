#include "canvas.h"
#include <mercury_log.h>
#include <ll/graphics.h>
#include <mercury_embedded_shaders.h>

using namespace mercury;
ll::graphics::ParameterBlockLayoutHandle gCanvasParameterBlockLayout;
using namespace ll::graphics;

constexpr int CANVAS_PER_FRAME_SET_INDEX = 0;


struct CanvasFrameResources
{
	ll::graphics::BufferHandle scene2DConstantBuffer;
	ll::graphics::ParameterBlockHandle scene2DParameterBlock;
};

std::vector<CanvasFrameResources> gCanvasFrameResources;


struct Sprite2D
{
	glm::vec2 position;
	glm::vec2 size;
	glm::vec2 uv0;
	glm::vec2 uv1;

	float angle;
	PackedColor color;
};

std::vector<Sprite2D> gSpritesToDraw;

mercury::ll::graphics::PsoHandle testDedicatedSpritePSO;
mercury::ll::graphics::ShaderHandle testDedicatedSpriteVS;
mercury::ll::graphics::ShaderHandle testDedicatedSpriteFS;

void MercuryCanvasInitialize(int numFramesInFlight)
{
	MLOG_DEBUG(u8"MercuryCanvasInitialize - Starting");


	ll::graphics::BindingSetLayoutDescriptor layoutDesc = {};
	layoutDesc.AddSlot(ll::graphics::ShaderResourceType::UniformBuffer);

	gCanvasParameterBlockLayout = ll::graphics::gDevice->CreateParameterBlockLayout(layoutDesc, CANVAS_PER_FRAME_SET_INDEX);

	gCanvasFrameResources.resize(numFramesInFlight);

	for (int i = 0; i < numFramesInFlight; ++i)
	{
		gCanvasFrameResources[i].scene2DConstantBuffer = gDevice->CreateBuffer(sizeof(Scene2DConstants));

		gCanvasFrameResources[i].scene2DParameterBlock = ll::graphics::gDevice->CreateParameterBlock(gCanvasParameterBlockLayout);

		ll::graphics::ParameterBlockDescriptor pbDesc = {};
		pbDesc.AddBuffer(gCanvasFrameResources[i].scene2DConstantBuffer);
		ll::graphics::gDevice->UpdateParameterBlock(gCanvasFrameResources[i].scene2DParameterBlock, pbDesc);

	}

	testDedicatedSpriteVS = ll::graphics::gDevice->CreateShaderModule(ll::graphics::embedded_shaders::DedicatedSpriteVS());
	testDedicatedSpriteFS = ll::graphics::gDevice->CreateShaderModule(ll::graphics::embedded_shaders::DedicatedSpritePS());

	ll::graphics::RasterizePipelineDescriptor dedicatedSpritePsoDesc = {};
	dedicatedSpritePsoDesc.vertexShader = testDedicatedSpriteVS;// testDedicatedSpriteVS;
	dedicatedSpritePsoDesc.fragmentShader = testDedicatedSpriteFS;
	dedicatedSpritePsoDesc.pushConstantSize = sizeof(Sprite2D);
	dedicatedSpritePsoDesc.bindingSetLayouts[0].AddSlot(ll::graphics::ShaderResourceType::UniformBuffer); //simulate canvas binding set layout

	dedicatedSpritePsoDesc.primitiveTopology = ll::graphics::PrimitiveTopology::TriangleStrip;
	testDedicatedSpritePSO = ll::graphics::gDevice->CreateRasterizePipeline(dedicatedSpritePsoDesc);
}

void MercuryCanvasShutdown()
{

	for (auto &cfr : gCanvasFrameResources)
	{
		gDevice->DestroyBuffer(cfr.scene2DConstantBuffer);
		cfr.scene2DConstantBuffer.Invalidate();
	}

	gCanvasFrameResources.clear();

	ll::graphics::gDevice->DestroyParameterBlockLayout(gCanvasParameterBlockLayout);
	MLOG_DEBUG(u8"MercuryCanvasShutdown - Starting");
}

void MercuryCanvasTick(mercury::ll::graphics::CommandList& cl, int frameInFlightIndex)
{
	static float ctime = 0.0f;
	ctime += 0.016f; // TODO: Replace with actual delta time

	Scene2DConstants scene2DConstants = {};
	scene2DConstants.canvasSize = glm::vec4((float)gSwapchain->GetWidth(), (float)gSwapchain->GetHeight(), 2.0f / (float)gSwapchain->GetWidth(), 2.0f / (float)gSwapchain->GetHeight());
	scene2DConstants.time = ctime;
	scene2DConstants.deltaTime = 0.016f; // TODO: Replace with actual delta time
	scene2DConstants.prerptationMatrix = glm::mat2x2(1.0f); // Identity matrix for now

	gDevice->UpdateBuffer(gCanvasFrameResources[frameInFlightIndex].scene2DConstantBuffer, &scene2DConstants, sizeof(Scene2DConstants));

	//cl.SetParameterBlockLayout(CANVAS_PER_FRAME_SET_INDEX, gCanvasParameterBlockLayout);

	cl.SetPSO(testDedicatedSpritePSO);
	//cl.SetUniformBuffer(0, gCanvasFrameResources[frameInFlightIndex].scene2DConstantBuffer);
	cl.SetParameterBlock(CANVAS_PER_FRAME_SET_INDEX, gCanvasFrameResources[frameInFlightIndex].scene2DParameterBlock);

	for(auto& sprite : gSpritesToDraw)
	{
		cl.PushConstants(sprite);
		cl.Draw(4); // Sprite is a triangle strip
	}

	gSpritesToDraw.clear();
}

void canvas::DrawSprite(glm::vec2 position, glm::vec2 size, glm::vec2 uv0, glm::vec2 uv1, float angle, PackedColor color)
{
	gSpritesToDraw.push_back({ position, size, uv0, uv1, angle, color });
}