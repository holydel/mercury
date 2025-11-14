module;

#include <vector>
#include <string>
#include <set>
#include <algorithm>
#include <cctype>
#include <atomic>
#include <OpenImageIO/imageio.h>
#include <mercury_api.h>
using namespace mercury;

export module Asset.Image.AssetLoaderOpenImageIO;

import Asset;
import Asset.Image;

export class AssetLoaderImageOpenImageIO : public IAssetLoader{
public:
	AssetLoaderImageOpenImageIO();
	~AssetLoaderImageOpenImageIO();
	static bool LoadImageFromFile();
	std::vector<std::string> GetSupportedExtensions() const override;
	virtual FileAsset* LoadAssetDataFromFile(const std::filesystem::path& path) override;
};

namespace {
	std::atomic<int> g_loader_instances{0};
}

AssetLoaderImageOpenImageIO::AssetLoaderImageOpenImageIO()
{
	if (g_loader_instances.fetch_add(1, std::memory_order_relaxed) == 0)
	{
		std::string ignored;
		OIIO::getattribute("format_list", ignored);
	}
}

AssetLoaderImageOpenImageIO::~AssetLoaderImageOpenImageIO()
{
	if (g_loader_instances.fetch_sub(1, std::memory_order_relaxed) == 1)
	{
		// No explicit shutdown required for basic OIIO usage.
	}
}

bool AssetLoaderImageOpenImageIO::LoadImageFromFile()
{
	return false;
}

static inline void to_lower_inplace(std::string& s)
{
	std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return (char)std::tolower(c); });
}

static std::vector<std::string> tokenize_ext_list(const std::string& src)
{
	std::vector<std::string> out;
	std::string token;
	auto flush = [&]() {
		if (!token.empty()) {
			to_lower_inplace(token);
			out.push_back(token);
			token.clear();
		}
	};
	for (char ch : src)
	{
		if (ch == ' ' || ch == '\t' || ch == '\n' || ch == ',' || ch == ';' || ch == '|' || ch == ':')
			flush();
		else
			token.push_back(ch);
	}
	flush();
	return out;
}

std::vector<std::string> AssetLoaderImageOpenImageIO::GetSupportedExtensions() const
{
	std::string ext_list;
	std::vector<std::string> result;

	if (OIIO::getattribute("extension_list", ext_list))
	{
		result = tokenize_ext_list(ext_list);
	}
	else
	{
		if (!OIIO::getattribute("input_format_list", ext_list))
			OIIO::getattribute("format_list", ext_list);
		result = tokenize_ext_list(ext_list);
	}

	std::sort(result.begin(), result.end());
	result.erase(std::unique(result.begin(), result.end()), result.end());

	return result;
}


FileAsset* AssetLoaderImageOpenImageIO::LoadAssetDataFromFile(const std::filesystem::path& path)
{
	using namespace OIIO;

	ImageAsset* asset = new ImageAsset();

	auto inp = ImageInput::open(path.string().c_str());

	const ImageSpec& spec = inp->spec();

	int xres = spec.width;
	int yres = spec.height;
	int zres = spec.depth;
	int nchannels = spec.nchannels;

	asset->channelSizeBytes = spec.format.size();
	asset->width = xres;
	asset->height = yres;
	asset->depth = zres;

	asset->numChannels = nchannels;

	asset->numLayers = 1;
	asset->numMipLevels = 1;

	asset->imageData = new u8[xres * yres * zres * nchannels * asset->channelSizeBytes];

	inp->read_image(0, 0, 0, -1, spec.format, asset->imageData);

	inp->close();

	asset->CreateResourcePreview();

	return asset;
}