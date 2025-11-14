module;
#include <mercury_api.h>
#include <ll/graphics.h>
#include <imgui.h>

using namespace mercury;

export module Asset.Image;
import Asset;

export enum class ImageFlags : mercury::u8
{
	None = 0,
	Cubemap = 1 << 0,
};

//full backed in memory image. for virtual textures there is another asset type
export struct ImageAsset : public FileAsset {
    ImageAsset() { type = AssetType::Image; }

	u8* imageData = nullptr;

	u16 width = 1;
	u16 height = 1;

	u16 depth = 1;
	u16 numLayers = 1;

	u8 numChannels = 4;
	u8 channelSizeBytes = 1;
	u8 numMipLevels = 1;

	ll::graphics::Format format = ll::graphics::Format::RGBA8_UNORM;

	ImageFlags flags = ImageFlags::None;

	ImTextureID resourcePreviewID = 0;
	ll::graphics::TextureHandle resourcePreviewTexID;

	void DrawProperty() override;

	void CreateResourcePreview();
};


void ImageAsset::DrawProperty() {
	ImGui::Text("Image");
	ImGui::Separator();
	ImGui::Text("Dimensions: %ux%u x %u layers", width, height, numLayers);

	if (resourcePreviewID != 0) {
		ImGui::Text("Preview:");
		ImGui::Image(resourcePreviewID, ImVec2(256, 256 * ((float)height / (float)width)));
	}
}

void ImageAsset::CreateResourcePreview()
{
	if (resourcePreviewID == 0)
	{
		ll::graphics::TextureDescriptor texDesc;
		texDesc.width = width;
		texDesc.height = height;
		texDesc.depth = depth;
		texDesc.mipLevels = numMipLevels;
		texDesc.format = format;
		texDesc.initialData = imageData;

		resourcePreviewTexID = ll::graphics::gDevice->CreateTexture(texDesc);
		resourcePreviewID = static_cast<ImTextureID>(resourcePreviewTexID.CreateImguiTextureOpaqueHandle());
	}
}