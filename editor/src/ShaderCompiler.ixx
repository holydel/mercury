module;
#pragma comment(lib, "slang.lib")
#pragma comment(lib, "slang-rt.lib")

#include <slang.h>
#include <slang-com-helper.h>
#include <slang-com-ptr.h>
#include <vector>
#include <mercury_shader.h>
#include <string>
#include <filesystem>

#include <mercury_log.h>

export module ShaderCompiler;

import EditorOptions;

//internal API
mercury::ShaderStage TranslateShaderStage(SlangStage slangStage);
SlangStage TranslateShaderStage(mercury::ShaderStage stage);

export namespace ShaderCompiler
{
	//public API
	export enum class CompileTarget : mercury::u32
	{
		NONE = 0,

		SPIRV = 1,
		DXIL = 1 << 1,
		Metal = 1 << 2,
		WGSL = 1 << 3,

		ALL = 0xFFFFFFFFu
	};

	export struct CompiledEntryPoint
	{
		std::string entryPoint;
		mercury::ShaderStage stage = mercury::ShaderStage::Unknown;

		std::vector<mercury::u8> spirv;
		std::vector<mercury::u8> dxil;
		std::vector<char> metal;
		std::vector<char> wgsl;
	};
	
	export struct CompileResult
	{
		std::vector<CompiledEntryPoint> entryPoints;
	};

	export void Initialize();
	export void Shutdown();	

	export CompileResult CompileShader(const std::filesystem::path& slangFile, CompileTarget selectedTargets = CompileTarget::ALL);

	CompileTarget GetLiveShaderCompileTarget()
	{
#ifdef MERCURY_LL_GRAPHICS_VULKAN
		return ShaderCompiler::CompileTarget::SPIRV;
#elif MERCURY_LL_GRAPHICS_DIRECT3D12
		return ShaderCompiler::CompileTarget::DXIL;
#elif MERCURY_LL_GRAPHICS_METAL
		return ShaderCompiler::CompileTarget::Metal;
#elif MERCURY_LL_GRAPHICS_WEBGPU
		return ShaderCompiler::CompileTarget::WGSL;
#endif
	}
}

using namespace slang;

Slang::ComPtr<IGlobalSession> gSlangGlobalSession = nullptr;
std::vector<slang::TargetDesc> gSupportedTargets;

slang::TargetDesc HelperDesc(SlangCompileTarget format, SlangProfileID profile)
{
	slang::TargetDesc tdesc = {};
	tdesc.format = format;
	tdesc.profile = profile;
	return tdesc;
}

void ShaderCompiler::Initialize()
{

	{
		SlangGlobalSessionDesc desc = {};
		desc.minLanguageVersion = SLANG_LANGUAGE_VERSION_LATEST;		
		//desc.apiVersion = SLANG_API_VERSION;
		//slang_createGlobalSession(SLANG_API_VERSION, &gSlangGlobalSession);
		slang_createGlobalSession2(&desc, gSlangGlobalSession.writeRef());
	}

	auto isSpirvSupported = gSlangGlobalSession->checkCompileTargetSupport(SlangCompileTarget::SLANG_SPIRV);
	auto isDxilSupported = gSlangGlobalSession->checkCompileTargetSupport(SlangCompileTarget::SLANG_DXIL);
	auto isMetalSupported = gSlangGlobalSession->checkCompileTargetSupport(SlangCompileTarget::SLANG_METAL);
	auto isWGSLSupported = gSlangGlobalSession->checkCompileTargetSupport(SlangCompileTarget::SLANG_WGSL);

	if (SLANG_SUCCEEDED(isSpirvSupported))
	{
		gSupportedTargets.emplace_back(HelperDesc(SlangCompileTarget::SLANG_SPIRV, gSlangGlobalSession->findProfile("spirv_1_6")));
	}
	else
	{
		MLOG_WARNING(u8"SPIRV target is not supported by the current Slang installation");
	}
	if (SLANG_SUCCEEDED(isMetalSupported))
	{
		gSupportedTargets.emplace_back(HelperDesc(SlangCompileTarget::SLANG_METAL, gSlangGlobalSession->findProfile("METAL_2_4")));
	}
	else
	{
		MLOG_WARNING(u8"Metal target is not supported by the current Slang installation");
	}

	if (SLANG_SUCCEEDED(isDxilSupported))
	{
		gSupportedTargets.emplace_back(HelperDesc(SlangCompileTarget::SLANG_DXIL, gSlangGlobalSession->findProfile("sm_6_9")));
	}
	else
	{
		MLOG_WARNING(u8"DXIL target is not supported by the current Slang installation");
	}

	if (SLANG_SUCCEEDED(isWGSLSupported))
	{
		gSupportedTargets.emplace_back(HelperDesc(SlangCompileTarget::SLANG_WGSL, gSlangGlobalSession->findProfile("")));
	}
	else
	{
		MLOG_WARNING(u8"WGSL target is not supported by the current Slang installation");
	}

	if (gSupportedTargets.empty())
	{
		MLOG_ERROR(u8"No supported targets found! Shader compilation will not be possible!");
		return;
	}
}

void ShaderCompiler::Shutdown()
{
	gSlangGlobalSession = nullptr;
}

mercury::ShaderStage TranslateShaderStage(SlangStage slangStage)
{
	using namespace mercury;

	switch (slangStage)
	{
	case SLANG_STAGE_VERTEX: return ShaderStage::Vertex;
	case SLANG_STAGE_HULL: return ShaderStage::TessellationControl;
	case SLANG_STAGE_DOMAIN: return ShaderStage::TessellationEvaluation;
	case SLANG_STAGE_GEOMETRY: return ShaderStage::Geometry;
	case SLANG_STAGE_FRAGMENT: return ShaderStage::Fragment;
	case SLANG_STAGE_COMPUTE: return ShaderStage::Compute;
	case SLANG_STAGE_AMPLIFICATION: return ShaderStage::Task;
	case SLANG_STAGE_MESH: return ShaderStage::Mesh;
	case SLANG_STAGE_RAY_GENERATION: return ShaderStage::RayGeneration;
	case SLANG_STAGE_INTERSECTION: return ShaderStage::Intersection;
	case SLANG_STAGE_ANY_HIT: return ShaderStage::AnyHit;
	case SLANG_STAGE_CLOSEST_HIT: return ShaderStage::ClosestHit;
	case SLANG_STAGE_MISS: return ShaderStage::Miss;
	case SLANG_STAGE_CALLABLE: return ShaderStage::Callable;
	case SLANG_STAGE_DISPATCH: return ShaderStage::WorkGraph;

	default:
		return mercury::ShaderStage::Vertex; //should not happen
	}
}

SlangStage TranslateShaderStage(mercury::ShaderStage stage)
{
	using namespace mercury;


	switch (stage)
	{
	case ShaderStage::Vertex: return SLANG_STAGE_VERTEX;
	case ShaderStage::TessellationControl: return SLANG_STAGE_HULL;
	case ShaderStage::TessellationEvaluation: return SLANG_STAGE_DOMAIN;
	case ShaderStage::Geometry: return SLANG_STAGE_GEOMETRY;
	case ShaderStage::Fragment: return SLANG_STAGE_FRAGMENT;
	case ShaderStage::Compute: return SLANG_STAGE_COMPUTE;
	case ShaderStage::Task: return SLANG_STAGE_AMPLIFICATION;
	case ShaderStage::Mesh: return SLANG_STAGE_MESH;
	case ShaderStage::RayGeneration: return SLANG_STAGE_RAY_GENERATION;
	case ShaderStage::Intersection: return SLANG_STAGE_INTERSECTION;
	case ShaderStage::AnyHit: return SLANG_STAGE_ANY_HIT;
	case ShaderStage::ClosestHit: return SLANG_STAGE_CLOSEST_HIT;
	case ShaderStage::Miss: return SLANG_STAGE_MISS;
	case ShaderStage::Callable: return SLANG_STAGE_CALLABLE;
	case ShaderStage::WorkGraph: return SLANG_STAGE_DISPATCH;

	default:
		return SLANG_STAGE_VERTEX; //should not happen
	}
}

template<typename T>
std::vector<T> BlobToVector(ISlangBlob* blob)
{
	std::vector<T> result;
	if (blob)
	{
		size_t size = blob->getBufferSize();
		const char* data = static_cast<const char*>(blob->getBufferPointer());
		size_t elementCount = size / sizeof(T);
		result.resize(elementCount);
		memset(result.data(), 0, size);
		memcpy(result.data(), data, size);
	}
	return result;
}

static ShaderCompiler::CompileTarget FromSlangFormat(SlangCompileTarget fmt)
{
	using CT = ShaderCompiler::CompileTarget;
	switch (fmt)
	{
	case SLANG_SPIRV: return CT::SPIRV;
	case SLANG_DXIL:  return CT::DXIL;
	case SLANG_METAL: return CT::Metal;
	case SLANG_WGSL:  return CT::WGSL;
	default:          return CT::NONE;
	}
}

ShaderCompiler::CompileResult ShaderCompiler::CompileShader(const std::filesystem::path& slangFile, CompileTarget requestedTargets)
{
	using namespace mercury;

	Slang::ComPtr<ISession> slangSession = nullptr;

	slang::TargetDesc selectedTargets[4] = {};
	int numSelectedTargets = 0;

	for(auto &st : gSupportedTargets)
	{
		u32 supportedFormat = (u32)FromSlangFormat(st.format);
		u32 requestedFormat = (u32)requestedTargets;
		
		if (supportedFormat & requestedFormat)
		{
			selectedTargets[numSelectedTargets++] = st;
		}
	}

	slang::SessionDesc desc = {};
	desc.targetCount = numSelectedTargets;// gSupportedTargets.size();
	desc.targets = selectedTargets;

	std::filesystem::path engineShadersPath = EditorOptions::GetMercuryEngineShadersPath();
	if (!engineShadersPath.empty())
	{
		static std::u8string u8EngineShadersPath = engineShadersPath.u8string();

		desc.searchPathCount = 1;
		const char* searchPaths[] = { (const char*)u8EngineShadersPath.c_str() };
		desc.searchPaths = searchPaths;
	}
	SlangResult res = gSlangGlobalSession->createSession(desc, slangSession.writeRef());
	
	CompileResult compileResult;

	Slang::ComPtr<ICompileRequest> request = nullptr;

	auto compileRequestCreatingResult = slangSession->createCompileRequest(request.writeRef());

	auto strSlangFileStr = slangFile.filename().string();
	
	request->addTranslationUnit(SLANG_SOURCE_LANGUAGE_SLANG, strSlangFileStr.c_str());
	request->addTranslationUnitSourceFile(0, slangFile.string().c_str());

	auto compileResultSlang = request->compile();

	const char* err_message = request->getDiagnosticOutput();

	if (SLANG_FAILED(compileResultSlang))
	{
		MLOG_ERROR(u8"Failed to compile shader %s: %s", slangFile.string().c_str(), err_message);
		return compileResult;
	}
	else
	{
		if (err_message && strlen(err_message) > 0)
		{
			MLOG_WARNING(u8"Warnings while compiling shader %s: %s", slangFile.string().c_str(), err_message);
		}
	}

	slang::IComponentType* outProgram = nullptr;
	auto gettingProgramResult = request->getProgram(&outProgram);
	
	slang::IComponentType* linkedProgram = nullptr;
	ISlangBlob* linkDiagnostics = nullptr;
	auto LinkingResult = outProgram->link(&linkedProgram, &linkDiagnostics);

	if (linkDiagnostics)
	{
		printf("Link diagnostics: %s\n", (const char*)linkDiagnostics->getBufferPointer());
	}
	
	SlangReflection* reflection = request->getReflection();

	int numEntryPoints = spReflection_getEntryPointCount(reflection);


	for (int i = 0; i < numEntryPoints; ++i)
	{
		
		auto entryPointInfo = spReflection_getEntryPointByIndex(reflection, i);
		const char* eName = spReflectionEntryPoint_getName(entryPointInfo);
		auto eStage = spReflectionEntryPoint_getStage(entryPointInfo);

		ShaderCompiler::CompiledEntryPoint compiledEntry;
		compiledEntry.entryPoint = eName;
		compiledEntry.stage = TranslateShaderStage(eStage);

		for(int j=0;j<gSupportedTargets.size();++j)
		{
			auto target = gSupportedTargets[j];
			ISlangBlob* shaderBlob = nullptr;


			auto getCodBlobeResult = request->getEntryPointCodeBlob(i, j, &shaderBlob);

			//auto getCodeResult = linkedProgram->getEntryPointCode(i, 0, &shaderBlob, &diagnosticsBlob);

			if (SLANG_FAILED(getCodBlobeResult))
			{
				ISlangBlob* diagnosticsBlob = nullptr;
				request->getDiagnosticOutputBlob(&diagnosticsBlob);
				MLOG_WARNING(u8"Diagnostics for target %d: %s\n", i, (const char*)diagnosticsBlob->getBufferPointer());
			}


			if (target.format == SlangCompileTarget::SLANG_SPIRV)
			{
				compiledEntry.spirv = BlobToVector<mercury::u8>(shaderBlob);
			}
			else if (target.format == SlangCompileTarget::SLANG_METAL)
			{
				compiledEntry.metal = BlobToVector<char>(shaderBlob);
			}
			else if (target.format == SlangCompileTarget::SLANG_WGSL)
			{
				compiledEntry.wgsl = BlobToVector<char>(shaderBlob);
			}
			else if (target.format == SlangCompileTarget::SLANG_DXIL)
			{
				compiledEntry.dxil = BlobToVector<mercury::u8>(shaderBlob);
			}
		}

		compileResult.entryPoints.push_back(compiledEntry);
	}
	return compileResult;
}