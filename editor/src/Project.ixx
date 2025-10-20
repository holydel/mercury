module;

#include <filesystem>
#include <simdjson/simdjson.h>
#include <unordered_set>
#include <set>
#include <functional>
#include <mercury_log.h>

export module Project;

import Asset;
import ShellOS;
import EditorOptions;

export class Project {
	std::filesystem::path rootPath;
	std::filesystem::path assetsPath = "";
	std::filesystem::path shadersPath = "";

	simdjson::ondemand::parser parser;

	FolderAsset* rootAssetsFolder = nullptr;
	FolderAsset* rootShadersFolder = nullptr;

	std::unordered_map<std::filesystem::path, ShaderSetAsset*> shadersMap;

public:
	Project() = default;
	~Project() = default;

	FolderAsset* GetRootAssetsFolder() { return rootAssetsFolder; }
	FolderAsset* GetRootShadersFolder() { return rootShadersFolder; }

	void LoadFromFolder(const char8_t* folderPath) {
		std::filesystem::path path = std::filesystem::path(folderPath);

		if (std::filesystem::exists(path) && std::filesystem::is_directory(path)) {
			rootPath = path;
			auto project_json_path = rootPath / "mproject.json";

			if (std::filesystem::exists(project_json_path)) {
				
				if(!assetsPath.empty())
					ShellOS::RemoveFolderWatcher(assetsPath);

				if (!shadersPath.empty())
					ShellOS::RemoveFolderWatcher(shadersPath);

				assetsPath = rootPath / "assets";
				shadersPath = rootPath / "shaders";

				std::vector<std::string> excludeFolders = { ".vs"};

				ShellOS::SetupFolderWatcher(assetsPath, [this](const std::filesystem::path& changedPath, ShellOS::WatcherAction action)
					{
						std::cout << "Asset changed: " << changedPath << std::endl;
						//RescanAssets();
					}, excludeFolders);

				ShellOS::SetupFolderWatcher(shadersPath, [this](const std::filesystem::path& changedPath, ShellOS::WatcherAction action)
					{
						std::cout << "Shader changed: " << changedPath << " action:" << (int)action << std::endl;

						if (changedPath.extension() == ".slang")
						{
							if (std::filesystem::exists(changedPath))
							{
								auto ftime = std::filesystem::last_write_time(changedPath);
								auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(ftime - std::filesystem::file_time_type::clock::now()
									+ std::chrono::system_clock::now());
								std::time_t cftime = std::chrono::system_clock::to_time_t(sctp);
								MLOG_INFO(u8"Changed action: %d", (int)action);

								if (action & ShellOS::WatcherAction::FileNameChanged)
								{
									auto it = shadersMap.find(changedPath);

									if(it != shadersMap.end())
									{
										MLOG_INFO(u8"Recompile shader: %s", changedPath.u8string().c_str());
										it->second->Compile();
									}
									else
									{
										MLOG_INFO(u8"New shader detected: %s", changedPath.u8string().c_str());
										// New shader file
										auto* shaderSet = new ShaderSetAsset();
										shaderSet->path = changedPath;
										shaderSet->size = std::filesystem::file_size(changedPath);
										shaderSet->Compile();
										if (rootShadersFolder)
										{
											rootShadersFolder->assets.push_back(shaderSet);
											rootShadersFolder->subfolders.push_back(shaderSet);
											shaderSet->ResolveAssetName();
										}
										shadersMap[changedPath] = shaderSet;
									}
									MLOG_INFO(u8"Recompile now");
								}
							}
						}
						//RescanShaders();
					}, excludeFolders);

				simdjson::padded_string json;
				simdjson::ondemand::document doc;

				simdjson::padded_string::load(project_json_path.string()).get(json);
				parser.iterate(json).get(doc);

				simdjson::ondemand::value project;
				doc["project"].get(project);

				std::string_view project_name;
				project["name"].get(project_name);
				std::cout << "Project Name: " << project_name << std::endl;

				std::string_view project_version;
				project["version"].get(project_version);
				std::cout << "Project Version: " << project_version << std::endl;

				RescanAssets();
				RescanShaders();
			}
		}
	}

	void SaveCurrent() {}

	void CreateNew(const char8_t* folderPath) {
		auto builtin_simdjson_impl = simdjson::builtin_implementation()->name();
		std::cout << builtin_simdjson_impl << std::endl;

		std::filesystem::path path = std::filesystem::path(folderPath);

		if (!std::filesystem::exists(path)) {
			std::filesystem::create_directories(path);
		}

		rootPath = path;
		assetsPath = rootPath / "assets";
		shadersPath = rootPath / "shaders";

		if (!std::filesystem::exists(assetsPath)) {
			std::filesystem::create_directories(assetsPath);
		}
		if (!std::filesystem::exists(shadersPath)) {
			std::filesystem::create_directories(shadersPath);
		}
	}

	// Names of folders to skip entirely
	inline static const std::set<std::string> filterFolderNamesForShaders = { ".vs" };

private:
	// Generic recursive folder scanner
	// onFile(parentFolder, dirEntry) is called for each regular file
	// decideEnterDir(path) decides whether to descend into a directory
	void ScanFolderGeneric(
		FolderAsset* parent,
		const std::function<void(FolderAsset*, const std::filesystem::directory_entry&)>& onFile,
		const std::function<bool(const std::filesystem::path&)>& decideEnterDir = [](auto const&) { return true; })
	{
		if (!parent || !std::filesystem::is_directory(parent->path))
			return;

		for (const auto& entry : std::filesystem::directory_iterator(parent->path))
		{
			if (entry.is_directory())
			{
				if (!decideEnterDir(entry.path()))
					continue;

				auto* newFolder = new FolderAsset();
				newFolder->path = entry.path();
				parent->subfolders.push_back(newFolder);
				parent->assets.push_back(newFolder);
				newFolder->ResolveAssetName();

				ScanFolderGeneric(newFolder, onFile, decideEnterDir);
				continue;
			}

			if (entry.is_regular_file())
			{
				onFile(parent, entry);
			}
		}
	}

public:
	void ScanFolderForShader(FolderAsset* parent)
	{
		auto onFile = [this](FolderAsset* folder, const std::filesystem::directory_entry& entry)
			{
				if (entry.path().extension() == ".slang")
				{
					auto* shaderSet = new ShaderSetAsset();
					shaderSet->path = entry.path();
					shaderSet->size = std::filesystem::file_size(entry.path());
					shaderSet->Compile();
					folder->assets.push_back(shaderSet);
					folder->subfolders.push_back(shaderSet);
					shaderSet->ResolveAssetName();

					shadersMap[entry.path()] = shaderSet;
				}
			};

		auto decideEnterDir = [](const std::filesystem::path& p) -> bool
			{
				return filterFolderNamesForShaders.find(p.filename().string()) == filterFolderNamesForShaders.end();
			};

		ScanFolderGeneric(parent, onFile, decideEnterDir);
	}

	void ScanFolderForAssets(FolderAsset* parent)
	{
		auto onFile = [](FolderAsset* folder, const std::filesystem::directory_entry& entry)
			{
				auto* fileAsset = new FileAsset();
				fileAsset->path = entry.path();
				fileAsset->size = std::filesystem::file_size(entry.path());
				fileAsset->ResolveAssetName();
				folder->assets.push_back(fileAsset);
			};

		ScanFolderGeneric(parent, onFile);
	}

	void RescanShaders()
	{
		rootShadersFolder = new FolderAsset();
		rootShadersFolder->path = shadersPath;
		rootShadersFolder->ResolveAssetName();

		auto engineSystemShaders = new FolderAsset();
		engineSystemShaders->path = EditorOptions::GetMercuryEngineShadersPath();

		auto editorSystemShaders = new FolderAsset();
		editorSystemShaders->path = EditorOptions::GetMercuryEditorShadersPath();

		rootShadersFolder->subfolders.push_back(engineSystemShaders);
		rootShadersFolder->subfolders.push_back(editorSystemShaders);

		ScanFolderForShader(engineSystemShaders);
		ScanFolderForShader(editorSystemShaders);
		ScanFolderForShader(rootShadersFolder);

		engineSystemShaders->SetSystemFolder("Engine Internal");
		editorSystemShaders->SetSystemFolder("Editor Internal");

	}

	void RescanAssets()
	{
		rootAssetsFolder = new FolderAsset();
		rootAssetsFolder->path = assetsPath;
		rootAssetsFolder->ResolveAssetName();
		ScanFolderForAssets(rootAssetsFolder);
	}
};