module;

#include <vector>
#include <filesystem>
#include <chrono>
#include <mercury_shader.h>
#include <imgui.h>
#include <atomic>
#include <unordered_map>
#include "ll/graphics.h"

export module Asset;

import PropertyWindow;
import ShaderCompiler;

export enum class AssetType {
    Unknown,
    Folder,
    GenericFile,
    ShadersSet,
    Shader
};

export struct Asset : public IImGuiPropertyDrawer {
protected:
    std::string assetName = "base asset";
public:
    Asset() { NextID++; id = NextID; }
    AssetType type = AssetType::Unknown;
    int id = 0;
    static std::atomic<int> NextID;
    ImGuiID GetID() const { return id; }
    virtual const char* GetAssetNameCStr() const { return assetName.c_str(); }
    virtual void ResolveAssetName() {}
};
std::atomic<int> Asset::NextID = 0;

export struct FileAsset : public Asset {
    FileAsset() { type = AssetType::GenericFile; }
    std::filesystem::path path;
    std::chrono::system_clock::time_point lastModifiedDate;
    uint64_t size = 0;
    void ResolveAssetName() override { assetName = path.filename().string(); }
};

export struct FolderAsset : public FileAsset {
    FolderAsset() { type = AssetType::Folder; }
    std::vector<Asset*> assets;
    std::vector<FolderAsset*> subfolders;
};

// Shader-related asset types (declarations only; heavy logic implemented in another TU)
export struct ShaderSetAsset; // forward for ShaderAsset

export struct ShaderAsset : public Asset {
    ShaderSetAsset* parentSet = nullptr;
    ShaderCompiler::CompiledEntryPoint compiledData;

    mercury::ShaderStage GetStage() const { return compiledData.stage; }
    const std::string& GetEntryPointName() const { return compiledData.entryPoint; }
    void ResolveAssetName() override { assetName = compiledData.entryPoint; }
    void DrawProperty() override;

    mercury::Handle<mercury::u32> cachedShaderID;

    void CacheIfNeeded();
};

export struct ShaderSetAsset : public FolderAsset {
    ShaderSetAsset() { type = AssetType::ShadersSet; }
    void Compile(ShaderCompiler::CompileTarget requestedTargets = ShaderCompiler::CompileTarget::ALL);
    void DrawProperty() override;
    void FinalPassPreview(mercury::ll::graphics::CommandList& finalCL) override;
private:
    mercury::Handle<mercury::u32> currentPreviewPipeline;

	void DrawRasterizationPipelineProps();
	void DrawComputePipelineProps();
	void DrawRayTracingPipelineProps();
	void DrawMeshPipelineProps();

	//rasterization pipeline
	std::vector<const char*> allVertexEntryPointNames;
    std::vector<const char*> allTessControlEntryPointNames;
    std::vector<const char*> allTessEvalEntryPointNames;
    std::vector<const char*> allGeometryEntryPointNames;
	std::vector<const char*> allFragmentEntryPointNames;
	
	//compute pipeline
	std::vector<const char*> allComputeEntryPointNames;

	//mesh pipeline
	std::vector<const char*> allMeshEntryPointNames;
	std::vector<const char*> allTaskEntryPointNames;

	//raytracing pipeline
	std::vector<const char*> allRayGenEntryPointNames;
	std::vector<const char*> allIntersectEntryPointNames;
	std::vector<const char*> allAnyHitEntryPointNames;
	std::vector<const char*> allClosestHitEntryPointNames;
	std::vector<const char*> allMissEntryPointNames;
	std::vector<const char*> allCallableEntryPointNames;

    int selectedVertexEntryPoint = -1;
    int selectedTessControlEntryPoint = -1;
    int selectedTessEvalEntryPoint = -1;
	int selectedGeometryEntryPoint = -1;
    int selectedFragmentEntryPoint = -1;

	int selectedComputeEntryPoint = -1;

	int selectedMeshEntryPoint = -1;
	int selectedTaskEntryPoint = -1;

	int selectedRayGenEntryPoint = -1;
	int selectedIntersectEntryPoint = -1;
	int selectedAnyHitEntryPoint = -1;
	int selectedClosestHitEntryPoint = -1;
	int selectedMissEntryPoint = -1;
	int selectedCallableEntryPoint = -1;

    std::unordered_map<int, ShaderAsset*> vertexShaderMap;
    std::unordered_map<int, ShaderAsset*> tessControlShaderMap;
    std::unordered_map<int, ShaderAsset*> tessEvalShaderMap;
    std::unordered_map<int, ShaderAsset*> geometryShaderMap;
    std::unordered_map<int, ShaderAsset*> fragmentShaderMap;

    std::unordered_map<int, ShaderAsset*> computeShaderMap;

    std::unordered_map<int, ShaderAsset*> meshShaderMap;
    std::unordered_map<int, ShaderAsset*> taskShaderMap;

    std::unordered_map<int, ShaderAsset*> rayGenShaderMap;
    std::unordered_map<int, ShaderAsset*> intersectShaderMap;
    std::unordered_map<int, ShaderAsset*> anyHitShaderMap;
    std::unordered_map<int, ShaderAsset*> closestHitShaderMap;
    std::unordered_map<int, ShaderAsset*> missShaderMap;
    std::unordered_map<int, ShaderAsset*> callableShaderMap;

	void UpdatePreivewGraphicsPipeline();
};

// Thumbnail support (unchanged)
export struct ThumbnailAtlas {
    static constexpr int MAX_ATLASES = 16;
    static ThumbnailAtlas globalAtlases[MAX_ATLASES];
    static ThumbnailAtlas* GetGlobalAtlas(int id) {
        if (id < 0 || id >= MAX_ATLASES) return nullptr;
        return &globalAtlases[id];
    }
    ImTextureID textureID = 0;
    mercury::u32 textureSize = 0;
};
ThumbnailAtlas ThumbnailAtlas::globalAtlases[ThumbnailAtlas::MAX_ATLASES];

export struct Thumbnail {
    static constexpr int THUMBNAIL_SIZE = 128;
    mercury::u32 atlasID : 4;
    mercury::u32 indexX  : 8;
    mercury::u32 indexY  : 8;
    mercury::u32 layer   : 12;
    void ImguiDrawImage(ImVec2 size = ImVec2((float)THUMBNAIL_SIZE, (float)THUMBNAIL_SIZE)) {
        if (atlasID >= ThumbnailAtlas::MAX_ATLASES) return;
        ThumbnailAtlas* atlas = ThumbnailAtlas::GetGlobalAtlas(atlasID);
        if (!atlas || atlas->textureID == 0) return;
        ImVec2 uv0 = ImVec2(float(indexX * THUMBNAIL_SIZE) / (float)atlas->textureSize,
                            float(indexY * THUMBNAIL_SIZE) / (float)atlas->textureSize);
        ImVec2 uv1 = ImVec2(float((indexX + 1) * THUMBNAIL_SIZE) / (float)atlas->textureSize,
                            float((indexY + 1) * THUMBNAIL_SIZE) / (float)atlas->textureSize);
        ImGui::Image(atlas->textureID, size, uv0, uv1);
    }
};

