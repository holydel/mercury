module;

#include <filesystem>
#include <simdjson.h>

export module Project;

// Define for each architecture simdjson supports

export class MProject {
	std::filesystem::path rootPath;
	std::filesystem::path assetsPath;
	std::filesystem::path shadersPath;

	simdjson::ondemand::parser parser;


	public:
	MProject() = default;
	~MProject() = default;

	void LoadFromFolder(const char8_t* folderPath) {
		std::filesystem::path path = std::filesystem::path(folderPath);

		if(std::filesystem::exists(path) && std::filesystem::is_directory(path)) {
			rootPath = path;
			assetsPath = rootPath / "assets";
			shadersPath = rootPath / "shaders";

			auto project_json_path = rootPath / "mproject.json";
			if(std::filesystem::exists(project_json_path)) {
				// Load project configuration here
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
			}
			else
			{
				
			}
			// Load project files here
		} else {
			// Handle error: folder does not exist or is not a directory
		}
	}

	void SaveCurrent()
	{

	}

	void CreateNew(const char8_t* folderPath)
	{
		auto builtin_simdjson_impl = simdjson::builtin_implementation()->name();

		std::cout << builtin_simdjson_impl << std::endl;

		std::filesystem::path path = std::filesystem::path(folderPath);

		if(!std::filesystem::exists(path)) {
			std::filesystem::create_directories(path);
		}
		
		rootPath = path;
		assetsPath = rootPath / "assets";
		shadersPath = rootPath / "shaders";

		if(!std::filesystem::exists(assetsPath)) {
			std::filesystem::create_directories(assetsPath);
		}
		if(!std::filesystem::exists(shadersPath)) {
			std::filesystem::create_directories(shadersPath);
		}

		//doc.root().g
	}

	void RescanAssets()
	{

	}
};