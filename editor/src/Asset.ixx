module;

#include <vector>
#include <filesystem>
#include <chrono>

export module Asset;

export enum class AssetType {
	Folder,
	GenericFile,
};

export struct AssetEntry {
	std::filesystem::path path;
	std::chrono::system_clock::time_point lastModifiedDate;
};

export struct FolderAssetEntry : public AssetEntry {
	std::vector<AssetEntry*> content;
};

export struct FileAssetEntry : public AssetEntry {
	uint64_t size = 0;
};