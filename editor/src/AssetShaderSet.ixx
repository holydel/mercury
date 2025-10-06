module;

#include <imgui.h>
#include <string>

import ShaderCompiler;
import Asset;

module Asset:ShaderSet;

struct ShaderSetAsset;

export struct ShaderAsset : Asset
{
	ShaderSetAsset* parentSet = nullptr;

	ShaderCompiler::CompiledEntryPoint compiledData;

	mercury::ShaderStage GetStage() const
	{
		return compiledData.stage;
	}

	std::string GetEntryPointName() const
	{
		return compiledData.entryPoint;
	}

	void ResolveAssetName() override
	{
		assetName = compiledData.entryPoint;
	}
};

export module Asset:ShaderSet;

export struct ShaderSetAsset : public FolderAsset {
	ShaderSetAsset()
	{
		type = AssetType::ShadersSet;
	}

	//bool compiled = false;
	void Compile();
	void DrawImguiPreview();
};

void ShaderSetAsset::Compile()
{
	auto result = ShaderCompiler::CompileShader(path);

	for (auto& s : result.entryPoints)
	{
		ShaderAsset* newShader = new ShaderAsset();
		newShader->parentSet = this;
		newShader->compiledData = s;
		newShader->ResolveAssetName();
		assets.push_back(newShader);
	}
}

void ShaderSetAsset::DrawImguiPreview()
{
}