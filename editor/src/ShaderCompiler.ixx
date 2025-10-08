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
#include <fstream>
#include <sstream>
#include <iomanip>
#include <cctype>

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
		DXIL  = 1 << 1,
		Metal = 1 << 2,
		WGSL  = 1 << 3,

		ALL = 0xFFFFFFFFu
	};

	// Bitmask operators for CompileTarget
	export constexpr inline CompileTarget operator|(CompileTarget a, CompileTarget b)
	{
		return static_cast<CompileTarget>(static_cast<mercury::u32>(a) | static_cast<mercury::u32>(b));
	}
	export constexpr inline CompileTarget operator&(CompileTarget a, CompileTarget b)
	{
		return static_cast<CompileTarget>(static_cast<mercury::u32>(a) & static_cast<mercury::u32>(b));
	}
	export constexpr inline CompileTarget& operator|=(CompileTarget& a, CompileTarget b)
	{
		a = a | b; return a;
	}
	export constexpr inline CompileTarget& operator&=(CompileTarget& a, CompileTarget b)
	{
		a = a & b; return a;
	}

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

	export struct RebuildShaderDesc
	{
		std::filesystem::path shadersFolder;
		std::filesystem::path outputHeaderPath;
		std::filesystem::path outputSourceSPIRVPath;
		std::filesystem::path outputSourceDXILPath;
		std::filesystem::path outputSourceMetalPath;
		std::filesystem::path outputSourceWGSLPath;
	};

	export void Initialize();
	export void Shutdown();	

	export CompileResult CompileShader(const std::filesystem::path& slangFile, CompileTarget selectedTargets = CompileTarget::ALL);

	// New API with structured descriptor
	export void RebuildEmbdeddedShaders(const RebuildShaderDesc& desc);
	
	// Legacy API (for backward compatibility or simple cases)
	export void RebuildEmbeddedShaders(const std::filesystem::path& shaderSourceDir, const std::filesystem::path& outputDir);

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
		static const char* searchPaths[] = { (const char*)u8EngineShadersPath.c_str() };
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

// Helper functions for embedded shader generation
std::string SanitizeIdentifier(const std::string& name)
{
	std::string result = name;
	// Replace invalid characters with underscore
	for (char& c : result)
	{
		if (!std::isalnum(static_cast<unsigned char>(c)) && c != '_')
			c = '_';
	}
	return result;
}

std::string GetStagePrefix(mercury::ShaderStage stage)
{
	using namespace mercury;
	switch (stage)
	{
	case ShaderStage::Vertex: return "VS";
	case ShaderStage::Fragment: return "PS";
	case ShaderStage::Compute: return "CS";
	case ShaderStage::Geometry: return "GS";
	case ShaderStage::TessellationControl: return "HS";
	case ShaderStage::TessellationEvaluation: return "DS";
	case ShaderStage::Mesh: return "MS";
	case ShaderStage::Task: return "AS";
	default: return "Unknown";
	}
}

void WriteByteArray(std::ofstream& out, const std::vector<mercury::u8>& data, const std::string& indent)
{
	out << indent << "static const mercury::u8 data[] = {\n" << indent << "\t";
	for (size_t i = 0; i < data.size(); ++i)
	{
		if (i > 0)
		{
			out << ", ";
			if (i % 16 == 0)
				out << "\n" << indent << "\t";
		}
		out << "0x" << std::hex << std::setfill('0') << std::setw(2) << (int)data[i] << std::dec;
	}
	out << "\n" << indent << "};\n";
}

void WriteCharArray(std::ofstream& out, const std::vector<char>& data, const std::string& indent)
{
	out << indent << "static const char data[] = {\n" << indent << "\t";
	for (size_t i = 0; i < data.size(); ++i)
	{
		if (i > 0)
		{
			out << ", ";
			if (i % 16 == 0)
				out << "\n" << indent << "\t";
		}
		// Write as unsigned to avoid negative values
		out << "0x" << std::hex << std::setfill('0') << std::setw(2) << ((int)(unsigned char)data[i]) << std::dec;
	}
	out << "\n" << indent << "};\n";
}

void ShaderCompiler::RebuildEmbdeddedShaders(const RebuildShaderDesc& desc)
{
	if (!std::filesystem::exists(desc.shadersFolder) || !std::filesystem::is_directory(desc.shadersFolder))
	{
		MLOG_ERROR(u8"Shader source directory does not exist: %s", desc.shadersFolder.string().c_str());
		return;
	}

	// Find all .slang files
	std::vector<std::filesystem::path> slangFiles;
	for (const auto& entry : std::filesystem::recursive_directory_iterator(desc.shadersFolder))
	{
		if (entry.is_regular_file() && entry.path().extension() == ".slang")
		{
			slangFiles.push_back(entry.path());
		}
	}

	if (slangFiles.empty())
	{
		MLOG_WARNING(u8"No .slang files found in: %s", desc.shadersFolder.string().c_str());
		return;
	}

	// Compile all shaders
	struct CompiledShaderData
	{
		std::string functionName;
		std::string baseName;
		mercury::ShaderStage stage;
		std::vector<mercury::u8> spirv;
		std::vector<mercury::u8> dxil;
		std::vector<char> metal;
		std::vector<char> wgsl;
	};

	std::vector<CompiledShaderData> allShaders;

	for (const auto& slangFile : slangFiles)
	{
		MLOG_INFO(u8"Compiling embedded shader: %s", slangFile.filename().string().c_str());
		
		auto compileResult = CompileShader(slangFile, CompileTarget::ALL);
		
		if (compileResult.entryPoints.empty())
		{
			MLOG_WARNING(u8"No entry points found in: %s", slangFile.filename().string().c_str());
			continue;
		}

		std::string baseName = slangFile.stem().string();
		baseName = SanitizeIdentifier(baseName);

		for (const auto& entry : compileResult.entryPoints)
		{
			CompiledShaderData shaderData;
			shaderData.baseName = baseName;
			shaderData.stage = entry.stage;
			shaderData.functionName = entry.entryPoint;
			shaderData.spirv = entry.spirv;
			shaderData.dxil = entry.dxil;
			shaderData.metal = entry.metal;
			shaderData.wgsl = entry.wgsl;

			allShaders.push_back(std::move(shaderData));
		}
	}

	if (allShaders.empty())
	{
		MLOG_WARNING(u8"No shaders compiled successfully");
		return;
	}

	// Generate header file
	{
		std::filesystem::create_directories(desc.outputHeaderPath.parent_path());
		std::ofstream header(desc.outputHeaderPath);
		
		header << "#pragma once\n\n";
		header << "#include <ll/graphics.h>\n\n";
		header << "namespace mercury::ll::graphics::embedded_shaders {\n\n";

		for (const auto& shader : allShaders)
		{
			header << "\t// " << shader.baseName << " - " << GetStagePrefix(shader.stage) << "\n";
			header << "\tmercury::ll::graphics::ShaderBytecodeView " << shader.functionName << "();\n\n";
		}

		header << "} // namespace mercury::ll::graphics::embedded_shaders\n";
		header.close();
		
		MLOG_INFO(u8"Generated header: %s", desc.outputHeaderPath.string().c_str());
	}

	// Generate SPIRV implementation
	if (!desc.outputSourceSPIRVPath.empty())
	{
		std::filesystem::create_directories(desc.outputSourceSPIRVPath.parent_path());
		std::ofstream spirv(desc.outputSourceSPIRVPath);
		
		spirv << "#include \"" << desc.outputHeaderPath.filename().string() << "\"\n\n";
		spirv << "#ifdef MERCURY_LL_GRAPHICS_VULKAN\n\n";
		spirv << "namespace mercury::ll::graphics::embedded_shaders {\n\n";

		for (const auto& shader : allShaders)
		{
			if (shader.spirv.empty()) continue;

			spirv << "mercury::ll::graphics::ShaderBytecodeView " << shader.functionName << "()\n{\n";
			WriteByteArray(spirv, shader.spirv, "\t");
			spirv << "\treturn { data, sizeof(data) };\n";
			spirv << "}\n\n";
		}

		spirv << "} // namespace mercury::ll::graphics::embedded_shaders\n";
		spirv << "#endif";
		spirv.close();
		
		MLOG_INFO(u8"Generated SPIRV implementation: %s", desc.outputSourceSPIRVPath.string().c_str());
	}

	// Generate DXIL implementation
	if (!desc.outputSourceDXILPath.empty())
	{
		std::filesystem::create_directories(desc.outputSourceDXILPath.parent_path());
		std::ofstream dxil(desc.outputSourceDXILPath);
		
		dxil << "#include \"" << desc.outputHeaderPath.filename().string() << "\"\n\n";
		dxil << "#ifdef MERCURY_LL_GRAPHICS_D3D12\n\n";
		dxil << "namespace mercury::ll::graphics::embedded_shaders {\n\n";

		for (const auto& shader : allShaders)
		{
			if (shader.dxil.empty()) continue;

			dxil << "mercury::ll::graphics::ShaderBytecodeView " << shader.functionName << "()\n{\n";
			WriteByteArray(dxil, shader.dxil, "\t");
			dxil << "\treturn { data, sizeof(data) };\n";
			dxil << "}\n\n";
		}

		dxil << "} // namespace mercury::ll::graphics::embedded_shaders\n";
		dxil << "#endif";
		dxil.close();
		
		MLOG_INFO(u8"Generated DXIL implementation: %s", desc.outputSourceDXILPath.string().c_str());
	}

	// Generate Metal implementation
	if (!desc.outputSourceMetalPath.empty())
	{
		std::filesystem::create_directories(desc.outputSourceMetalPath.parent_path());
		std::ofstream metal(desc.outputSourceMetalPath);
		
		metal << "#include \"" << desc.outputHeaderPath.filename().string() << "\"\n\n";
		metal << "#ifdef MERCURY_LL_GRAPHICS_METAL\n\n";
		metal << "namespace mercury::ll::graphics::embedded_shaders {\n\n";

		for (const auto& shader : allShaders)
		{
			if (shader.metal.empty()) continue;

			metal << "mercury::ll::graphics::ShaderBytecodeView " << shader.functionName << "()\n{\n";
			WriteCharArray(metal, shader.metal, "\t");
			metal << "\treturn { data, sizeof(data) };\n";
			metal << "}\n\n";
		}

		metal << "} // namespace mercury::ll::graphics::embedded_shaders\n";
		metal << "#endif";
		metal.close();
		
		MLOG_INFO(u8"Generated Metal implementation: %s", desc.outputSourceMetalPath.string().c_str());
	}

	// Generate WGSL implementation
	if (!desc.outputSourceWGSLPath.empty())
	{
		std::filesystem::create_directories(desc.outputSourceWGSLPath.parent_path());
		std::ofstream wgsl(desc.outputSourceWGSLPath);
		
		wgsl << "#include \"" << desc.outputHeaderPath.filename().string() << "\"\n\n";
		wgsl << "#ifdef MERCURY_LL_GRAPHICS_WEBGPU\n\n";
		wgsl << "namespace mercury::ll::graphics::embedded_shaders {\n\n";

		for (const auto& shader : allShaders)
		{
			if (shader.wgsl.empty()) continue;

			wgsl << "mercury::ll::graphics::ShaderBytecodeView " << shader.functionName << "()\n{\n";
			WriteCharArray(wgsl, shader.wgsl, "\t");
			wgsl << "\treturn { data, sizeof(data) };\n";
			wgsl << "}\n\n";
		}

		wgsl << "} // namespace mercury::ll::graphics::embedded_shaders\n";
		wgsl << "#endif";
		wgsl.close();
		
		MLOG_INFO(u8"Generated WGSL implementation: %s", desc.outputSourceWGSLPath.string().c_str());
	}

	MLOG_INFO(u8"Successfully rebuilt %zu embedded shaders", allShaders.size());
}

// Legacy API implementation (uses simplified directory structure)
void ShaderCompiler::RebuildEmbeddedShaders(const std::filesystem::path& shaderSourceDir, const std::filesystem::path& outputDir)
{
	RebuildShaderDesc desc;
	desc.shadersFolder = shaderSourceDir;
	desc.outputHeaderPath = outputDir / "engine_embedded_shaders.h";
	desc.outputSourceSPIRVPath = outputDir / "engine_embedded_shaders_spirv.cpp";
	desc.outputSourceDXILPath = outputDir / "engine_embedded_shaders_dxil.cpp";
	desc.outputSourceMetalPath = outputDir / "engine_embedded_shaders_metal.cpp";
	desc.outputSourceWGSLPath = outputDir / "engine_embedded_shaders_wgsl.cpp";
	
	RebuildEmbdeddedShaders(desc);
}