module;

#include <imgui.h>
#include <filesystem>
#include <mercury_utils.h>
#include <functional>
#include <ll/graphics.h>

module Asset;

import Asset;
import ShaderCompiler;

using namespace mercury;
using namespace mercury::ll;

void ShaderSetAsset::Compile(ShaderCompiler::CompileTarget requestedTargets)
{
	// Clear previous shaders (simple linear delete – adjust when adding ownership management)
	for (auto* a : assets) {
		delete a;
	}
	assets.clear();

	if (!std::filesystem::exists(path) || !std::filesystem::is_regular_file(path))
		return;

	auto result = ShaderCompiler::CompileShader(path, requestedTargets);
	for (auto& ep : result.entryPoints)
	{
		auto* shader = new ShaderAsset();
		shader->parentSet = this;
		shader->compiledData = ep;
		shader->ResolveAssetName();
		assets.push_back(shader);
	}

	allVertexEntryPointNames.clear();
	allTessControlEntryPointNames.clear();
	allTessEvalEntryPointNames.clear();
	allGeometryEntryPointNames.clear();
	allFragmentEntryPointNames.clear();
	allComputeEntryPointNames.clear();
	allRayGenEntryPointNames.clear();
	allIntersectEntryPointNames.clear();
	allAnyHitEntryPointNames.clear();
	allClosestHitEntryPointNames.clear();
	allMissEntryPointNames.clear();
	allCallableEntryPointNames.clear();
	allTaskEntryPointNames.clear();
	allMeshEntryPointNames.clear();

	vertexShaderMap.clear();
	tessControlShaderMap.clear();
	tessEvalShaderMap.clear();
	geometryShaderMap.clear();
	fragmentShaderMap.clear();

	computeShaderMap.clear();

	rayGenShaderMap.clear();
	intersectShaderMap.clear();
	anyHitShaderMap.clear();
	closestHitShaderMap.clear();
	missShaderMap.clear();
	callableShaderMap.clear();

	taskShaderMap.clear();
	meshShaderMap.clear();
	struct FillEntryPointStuff
	{
		ShaderAsset* shader;
		FillEntryPointStuff(ShaderAsset* s) : shader(s) {}

		void fill(std::vector<const char*>& names, std::unordered_map<int, ShaderAsset*>& map, int& selected)
		{
			names.push_back(shader->GetEntryPointName().c_str());
			map[names.size() - 1] = shader;
			if (selected == -1)
				selected = 0;
		}
	};

	for (auto* a : assets)
	{
		auto ep = static_cast<ShaderAsset*>(a);
		auto stage = ep->GetStage();

		FillEntryPointStuff filler(ep);

		//RASTERIZE
		if (stage == ShaderStage::Vertex) filler.fill(allVertexEntryPointNames, vertexShaderMap, selectedVertexEntryPoint);
		if (stage == ShaderStage::TessellationControl) filler.fill(allTessControlEntryPointNames, tessControlShaderMap, selectedTessControlEntryPoint);
		if (stage == ShaderStage::TessellationEvaluation) filler.fill(allTessEvalEntryPointNames, tessEvalShaderMap, selectedTessEvalEntryPoint);
		if (stage == ShaderStage::Geometry) filler.fill(allGeometryEntryPointNames, geometryShaderMap, selectedGeometryEntryPoint);
		if (stage == ShaderStage::Fragment) filler.fill(allFragmentEntryPointNames, fragmentShaderMap, selectedFragmentEntryPoint);

		//COMPUTE
		if (stage == ShaderStage::Compute) filler.fill(allComputeEntryPointNames, computeShaderMap, selectedComputeEntryPoint);

		//RAYTRACING
		if (stage == ShaderStage::RayGeneration) filler.fill(allRayGenEntryPointNames, rayGenShaderMap, selectedRayGenEntryPoint);
		if (stage == ShaderStage::Intersection) filler.fill(allIntersectEntryPointNames, intersectShaderMap, selectedIntersectEntryPoint);
		if (stage == ShaderStage::AnyHit) filler.fill(allAnyHitEntryPointNames, anyHitShaderMap, selectedAnyHitEntryPoint);
		if (stage == ShaderStage::ClosestHit) filler.fill(allClosestHitEntryPointNames, closestHitShaderMap, selectedClosestHitEntryPoint);
		if (stage == ShaderStage::Miss) filler.fill(allMissEntryPointNames, missShaderMap, selectedMissEntryPoint);
		if (stage == ShaderStage::Callable) filler.fill(allCallableEntryPointNames, callableShaderMap, selectedCallableEntryPoint);

		//MESH
		if (stage == ShaderStage::Task) filler.fill(allTaskEntryPointNames, taskShaderMap, selectedTaskEntryPoint);
		if (stage == ShaderStage::Mesh) filler.fill(allMeshEntryPointNames, meshShaderMap, selectedMeshEntryPoint);

		ep->cachedShaderID.Invalidate();
	}
}

void ShaderAsset::CacheIfNeeded()
{
	if (cachedShaderID.isValid())
		return;

	graphics::ShaderBytecodeView view;
	view.data = compiledData.spirv.data();
	view.size = compiledData.spirv.size();

	cachedShaderID = graphics::gDevice->CreateShaderModule(view);
}

void ShaderSetAsset::DrawRasterizationPipelineProps()
{
	if (!allVertexEntryPointNames.empty())
		ImGui::Combo("Vertex Shader", &selectedVertexEntryPoint, allVertexEntryPointNames.data(), (int)allVertexEntryPointNames.size(), 8);

	if (!allTessControlEntryPointNames.empty())
		ImGui::Combo("Tessellation Control Shader", &selectedTessControlEntryPoint, allTessControlEntryPointNames.data(), (int)allTessControlEntryPointNames.size(), 8);

	if (!allTessEvalEntryPointNames.empty())
		ImGui::Combo("Tessellation Evaluation Shader", &selectedTessEvalEntryPoint, allTessEvalEntryPointNames.data(), (int)allTessEvalEntryPointNames.size(), 8);

	if (!allGeometryEntryPointNames.empty())
		ImGui::Combo("Geometry Shader", &selectedGeometryEntryPoint, allGeometryEntryPointNames.data(), (int)allGeometryEntryPointNames.size(), 8);

	if (!allFragmentEntryPointNames.empty())
		ImGui::Combo("Fragment Shader", &selectedFragmentEntryPoint, allFragmentEntryPointNames.data(), (int)allFragmentEntryPointNames.size(), 8);

	ImGui::Separator();

	bool canCreatePipeline = selectedVertexEntryPoint >= 0; // can be without fragment shader (e.g. transform feedback only)

	ImGui::BeginDisabled(!canCreatePipeline);
	if (ImGui::Button("Create Rasterization Pipeline"))
	{
		ShaderAsset* vertesShaderAsset = vertexShaderMap[selectedVertexEntryPoint];

		ShaderAsset* fragmentShaderAsset = selectedFragmentEntryPoint >= 0 ? fragmentShaderMap[selectedFragmentEntryPoint] : nullptr;
		ShaderAsset* tessControlShaderAsset = selectedTessControlEntryPoint >= 0 ? tessControlShaderMap[selectedTessControlEntryPoint] : nullptr;
		ShaderAsset* tessEvalShaderAsset = selectedTessEvalEntryPoint >= 0 ? tessEvalShaderMap[selectedTessEvalEntryPoint] : nullptr;
		ShaderAsset* geometryShaderAsset = selectedGeometryEntryPoint >= 0 ? geometryShaderMap[selectedGeometryEntryPoint] : nullptr;

		graphics::RasterizePipelineDescriptor desc = {};

		vertesShaderAsset->CacheIfNeeded();
		desc.vertexShader = vertesShaderAsset->cachedShaderID;

		if (tessControlShaderAsset)
		{
			tessControlShaderAsset->CacheIfNeeded();
			desc.tessControlShader = tessControlShaderAsset->cachedShaderID;
		}

		if (tessEvalShaderAsset)
		{
			tessEvalShaderAsset->CacheIfNeeded();
			desc.tessEvalShader = tessEvalShaderAsset->cachedShaderID;
		}

		if (geometryShaderAsset)
		{
			geometryShaderAsset->CacheIfNeeded();
			desc.geometryShader = geometryShaderAsset->cachedShaderID;
		}

		if (fragmentShaderAsset)
		{
			fragmentShaderAsset->CacheIfNeeded();
			desc.fragmentShader = fragmentShaderAsset->cachedShaderID;
		}

		if (currentPreviewPipeline.isValid())
		{
			graphics::gDevice->UpdatePipelineState(currentPreviewPipeline, desc);
		}
		else
		{
			currentPreviewPipeline = graphics::gDevice->CreateRasterizePipeline(desc);
		}
	}
	ImGui::EndDisabled();
}


void ShaderSetAsset::DrawComputePipelineProps()
{
	if (!allComputeEntryPointNames.empty())
		ImGui::Combo("Compute Shader", &selectedComputeEntryPoint, allComputeEntryPointNames.data(), (int)allComputeEntryPointNames.size(), 8);
}

void ShaderSetAsset::DrawRayTracingPipelineProps()
{
	if (!allRayGenEntryPointNames.empty())
		ImGui::Combo("Ray Generation Shader", &selectedRayGenEntryPoint, allRayGenEntryPointNames.data(), (int)allRayGenEntryPointNames.size(), 8);
	if (!allIntersectEntryPointNames.empty())
		ImGui::Combo("Intersection Shader", &selectedIntersectEntryPoint, allIntersectEntryPointNames.data(), (int)allIntersectEntryPointNames.size(), 8);
	if (!allAnyHitEntryPointNames.empty())
		ImGui::Combo("Any Hit Shader", &selectedAnyHitEntryPoint, allAnyHitEntryPointNames.data(), (int)allAnyHitEntryPointNames.size(), 8);
	if (!allClosestHitEntryPointNames.empty())
		ImGui::Combo("Closest Hit Shader", &selectedClosestHitEntryPoint, allClosestHitEntryPointNames.data(), (int)allClosestHitEntryPointNames.size(), 8);
	if (!allMissEntryPointNames.empty())
		ImGui::Combo("Miss Shader", &selectedMissEntryPoint, allMissEntryPointNames.data(), (int)allMissEntryPointNames.size(), 8);
	if (!allCallableEntryPointNames.empty())
		ImGui::Combo("Callable Shader", &selectedCallableEntryPoint, allCallableEntryPointNames.data(), (int)allCallableEntryPointNames.size(), 8);
}

void ShaderSetAsset::DrawMeshPipelineProps()
{
	if (!allTaskEntryPointNames.empty())
		ImGui::Combo("Task Shader", &selectedTaskEntryPoint, allTaskEntryPointNames.data(), (int)allTaskEntryPointNames.size(), 8);
	if (!allMeshEntryPointNames.empty())
		ImGui::Combo("Mesh Shader", &selectedMeshEntryPoint, allMeshEntryPointNames.data(), (int)allMeshEntryPointNames.size(), 8);
}

void ShaderSetAsset::DrawProperty()
{
	using namespace mercury;

	ImGui::SeparatorText("Shader Set");
	ImGui::Text("File: %s", path.filename().string().c_str());
	ImGui::SameLine();

	if (ImGui::SmallButton("Recompile")) {	
		Compile(ShaderCompiler::GetLiveShaderCompileTarget());	
	}

	ImGui::Separator();
	int hasRasterization = !allVertexEntryPointNames.empty() || !allTessControlEntryPointNames.empty()
		|| !allTessEvalEntryPointNames.empty() || !allGeometryEntryPointNames.empty() || !allFragmentEntryPointNames.empty();

	int hasCompute = !allComputeEntryPointNames.empty();
	int hasRayTracing = !allRayGenEntryPointNames.empty() || !allIntersectEntryPointNames.empty() || !allCallableEntryPointNames.empty()
		|| !allAnyHitEntryPointNames.empty() || !allClosestHitEntryPointNames.empty() || !allMissEntryPointNames.empty();

	int hasMesh = !allTaskEntryPointNames.empty() || !allMeshEntryPointNames.empty();

	int numDifferentPipeines = hasRasterization + hasCompute + hasRayTracing + hasMesh;

	if (numDifferentPipeines == 0)
	{
		ImGui::TextColored(ImVec4(1, 0, 0, 1), "No entry points found!");
		return;
	}

	struct CollapsingDecorator
	{
		bool needDecoration;
		CollapsingDecorator(bool needDecoration)
		{
			this->needDecoration = needDecoration;
		}

		void DrawDecorated(bool condition, const char* label, std::function<void()> drawFunc)
		{
			if (condition == false)
				return;

			if (needDecoration)
			{
				if (ImGui::CollapsingHeader(label))
				{
					ImGui::BeginChild(label, ImVec2(0, 150), true);
					drawFunc();
					ImGui::EndChild();
				}
			}
			else
			{
				ImGui::Text("%s", label);
				drawFunc();
			}

		}
	};

	CollapsingDecorator decorator(numDifferentPipeines > 1);

	decorator.DrawDecorated(hasRasterization, "Rasterization", [this]() { DrawRasterizationPipelineProps(); });
	decorator.DrawDecorated(hasCompute, "Compute", [this]() { DrawComputePipelineProps(); });
	decorator.DrawDecorated(hasMesh, "Mesh", [this]() { DrawMeshPipelineProps(); });
	decorator.DrawDecorated(hasRayTracing, "Ray Tracing", [this]() { DrawRayTracingPipelineProps(); });
}

void ShaderAsset::DrawProperty()
{
	ImGui::Text("Shader Entry Point: %s", GetEntryPointName().c_str());

	std::string shaderStageStr = mercury::utils::string::from(GetStage());
	ImGui::Text("Shader Stage: %s", shaderStageStr.c_str());
	ImGui::Separator();

	if (!compiledData.dxil.empty() && ImGui::CollapsingHeader("DXIL", ImGuiTreeNodeFlags_DefaultOpen))
	{
		ImGui::Text("Bytes: %zu", compiledData.dxil.size());
	}

	if (!compiledData.spirv.empty() && ImGui::CollapsingHeader("SPIR-V"))
	{
		ImGui::Text("Bytes: %zu (words %zu)", compiledData.spirv.size(), compiledData.spirv.size() / 4);
	}

	if (!compiledData.metal.empty() && ImGui::CollapsingHeader("Metal Source"))
	{
		ImGui::BeginChild("metal_src", ImVec2(0, 120), true);
		ImGui::TextUnformatted(compiledData.metal.data(), compiledData.metal.data() + compiledData.metal.size());
		ImGui::EndChild();
	}

	if (!compiledData.wgsl.empty() && ImGui::CollapsingHeader("WGSL Source"))
	{
		ImGui::BeginChild("wgsl_src", ImVec2(0, 120), true);
		ImGui::TextUnformatted(compiledData.wgsl.data(), compiledData.wgsl.data() + compiledData.wgsl.size());
		ImGui::EndChild();
	}

}

void ShaderSetAsset::FinalPassPreview(mercury::ll::graphics::CommandList& finalCL)
{
	if (currentPreviewPipeline.isValid())
	{
		finalCL.SetPSO(currentPreviewPipeline);
		finalCL.Draw(3, 1, 0, 0);
	}
}