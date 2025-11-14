module;
#include <vector>
#include <string>
#include <filesystem>
#include <unordered_map>

export module AssetsLoadersRegistry;

import Asset.Image.AssetLoaderOpenImageIO;
import Asset;

export class AssetsLoadersRegistry {
	std::vector<IAssetLoader*> allLoaders;

	std::unordered_map<std::string, std::vector<IAssetLoader*>> extensionToLoadersMap;

	AssetsLoadersRegistry();
	~AssetsLoadersRegistry();

public:
	static AssetsLoadersRegistry& GetInstance();

	FileAsset* LoadAssetFromFile(const std::filesystem::path& path);

	bool Initialize();
};


AssetsLoadersRegistry& AssetsLoadersRegistry::GetInstance()
{
	static AssetsLoadersRegistry instance;
	return instance;
}

AssetsLoadersRegistry::AssetsLoadersRegistry()
{

}

AssetsLoadersRegistry::~AssetsLoadersRegistry()
{
	for (auto loader : allLoaders)
	{
		delete loader;
	}
}

bool AssetsLoadersRegistry::Initialize()
{
	allLoaders.push_back(new AssetLoaderImageOpenImageIO());

	for (int i = 0; i < allLoaders.size(); ++i)
	{
		auto loader = allLoaders[i];
		auto exts = loader->GetSupportedExtensions();
		for (const auto& ext : exts)
		{
			extensionToLoadersMap[ext].push_back(loader);
		}
	}
	return true;
}

FileAsset* AssetsLoadersRegistry::LoadAssetFromFile(const std::filesystem::path& path)
{
	FileAsset* loadedAsset = nullptr;


	//what is ext is not in lower case?
	auto ext = path.extension().string().substr(1);
	auto it = extensionToLoadersMap.find(ext); 

	if (it != extensionToLoadersMap.end())
	{
		auto& loaders = it->second;
		for (auto loader : loaders)
		{
			loadedAsset = loader->LoadAssetDataFromFile(path);
			if (loadedAsset != nullptr)
			{
				break;
			}
		}
	}

	return loadedAsset;
}