#include "canvas.h"
#include <mercury_log.h>
#include <ll/graphics.h>
#include <mercury_embedded_shaders.h>

using namespace mercury;
ll::graphics::ParameterBlockLayoutHandle gCanvasParameterBlockLayout;
ll::graphics::ParameterBlockLayoutHandle gCanvasSpriteTextureParameterBlockLayout;

ll::graphics::TextureHandle gWhiteTextureHandle;
ll::graphics::ParameterBlockHandle gWhiteTexturePlaceholder;

using namespace ll::graphics;

constexpr int CANVAS_PER_FRAME_SET_INDEX = 0;
constexpr int CANVAS_SPRITE_TEXTURE_SET_INDEX = 1;


struct CanvasFrameResources
{
	ll::graphics::BufferHandle scene2DConstantBuffer;
	ll::graphics::ParameterBlockHandle scene2DParameterBlock;
};

std::vector<CanvasFrameResources> gCanvasFrameResources;


struct SpriteTransform
{
	glm::vec2 position;
	glm::vec2 size;
	glm::vec2 uv0;
	glm::vec2 uv1;

	float angle;
	PackedColor color;
};

struct Sprite2D
{
	SpriteTransform transform;
	ll::graphics::TextureHandle texture;
};

std::vector<Sprite2D> gSpritesToDraw;

mercury::ll::graphics::PsoHandle testDedicatedSpritePSO;
mercury::ll::graphics::ShaderHandle testDedicatedSpriteVS;
mercury::ll::graphics::ShaderHandle testDedicatedSpriteFS;

std::unordered_map<u32, ll::graphics::ParameterBlockHandle> gSpriteTextureParameterBlocks;

void MercuryCanvasInitialize(int numFramesInFlight)
{
	MLOG_DEBUG(u8"MercuryCanvasInitialize - Starting");


	ll::graphics::BindingSetLayoutDescriptor layoutDesc = {};
	layoutDesc.AddSlot(ll::graphics::ShaderResourceType::UniformBuffer);

	gCanvasParameterBlockLayout = ll::graphics::gDevice->CreateParameterBlockLayout(layoutDesc, CANVAS_PER_FRAME_SET_INDEX);

	{
		ll::graphics::BindingSetLayoutDescriptor layoutDesc2 = {};
		layoutDesc2.AddSlot(ll::graphics::ShaderResourceType::SampledImage2D);
		gCanvasSpriteTextureParameterBlockLayout = ll::graphics::gDevice->CreateParameterBlockLayout(layoutDesc2, CANVAS_SPRITE_TEXTURE_SET_INDEX);

		gWhiteTextureHandle = ll::graphics::TextureHandle::GetBlackOpaqueTexture();
		gWhiteTexturePlaceholder = ll::graphics::gDevice->CreateParameterBlock(gCanvasSpriteTextureParameterBlockLayout);

		ll::graphics::ParameterBlockDescriptor pbDesc = {};
		pbDesc.AddSampledTexture2D(gWhiteTextureHandle);

		ll::graphics::gDevice->UpdateParameterBlock(gWhiteTexturePlaceholder, pbDesc);
	}
	

	gCanvasFrameResources.resize(numFramesInFlight);

	for (int i = 0; i < numFramesInFlight; ++i)
	{
		BufferDescriptor bdesc = {};
		bdesc.size = sizeof(Scene2DConstants);

		gCanvasFrameResources[i].scene2DConstantBuffer = gDevice->CreateBuffer(bdesc);

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
	dedicatedSpritePsoDesc.pushConstantSize = sizeof(SpriteTransform);
	dedicatedSpritePsoDesc.bindingSetLayouts[0].AddSlot(ll::graphics::ShaderResourceType::UniformBuffer); //simulate canvas binding set layout
	dedicatedSpritePsoDesc.bindingSetLayouts[1].AddSlot(ll::graphics::ShaderResourceType::SampledImage2D); //simulate canvas binding set layout


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
	scene2DConstants.canvasSize = glm::vec4((float)gSwapchain->GetWidth(), (float)gSwapchain->GetHeight(), 2.0f / (float)gSwapchain->GetWidth(), (ll::graphics::IsYFlipped() ? -2.0f : 2.0f) / (float)gSwapchain->GetHeight());
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
		if (sprite.texture.isValid())
		{
			auto texIt = gSpriteTextureParameterBlocks.find(sprite.texture.handle);

			if (texIt == gSpriteTextureParameterBlocks.end())
			{
				gSpriteTextureParameterBlocks[sprite.texture.handle] = ll::graphics::gDevice->CreateParameterBlock(gCanvasSpriteTextureParameterBlockLayout);
				ll::graphics::ParameterBlockDescriptor pbDesc = {};
				pbDesc.AddSampledTexture2D(sprite.texture);
				ll::graphics::gDevice->UpdateParameterBlock(gSpriteTextureParameterBlocks[sprite.texture.handle], pbDesc);
				texIt = gSpriteTextureParameterBlocks.find(sprite.texture.handle);
			}

			cl.SetParameterBlock(CANVAS_SPRITE_TEXTURE_SET_INDEX, texIt->second);
		}
		else
		{
			cl.SetParameterBlock(CANVAS_SPRITE_TEXTURE_SET_INDEX, gWhiteTexturePlaceholder);
		}
		
		cl.PushConstants(sprite.transform);
		cl.Draw(4); // Sprite is a triangle strip
	}

	gSpritesToDraw.clear();
}

void canvas::DrawSprite(glm::vec2 position, glm::vec2 size, glm::vec2 uv0, glm::vec2 uv1, float angle, PackedColor color)
{
	gSpritesToDraw.push_back({ { position, size, uv0, uv1, angle, color }, gWhiteTextureHandle });
}

void canvas::DrawSprite(ll::graphics::TextureHandle texture, glm::vec2 position, glm::vec2 size, glm::vec2 uv0, glm::vec2 uv1, float angle, PackedColor color)
{
	gSpritesToDraw.push_back({ { position, size, uv0, uv1, angle, color }, texture });
}