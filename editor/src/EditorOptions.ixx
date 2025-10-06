module;
#include <imgui.h>
#include <string>
#include <filesystem>
#include <mercury_log.h>

export module EditorOptions;

import PersistentKeyValueStorage;
import ShellOS;

static bool isEditorOptionsWindowOpen = false;
static char gMercuryEnginePath[260];
static bool gIsEnginePathValid = false;

static std::filesystem::path gEngineRootPath = "";
static std::filesystem::path gEngineShadersPath = "";
static std::filesystem::path gEditorRootPath = "";
static std::filesystem::path gEditorShadersPath = "";

void SaveEditorOptions()
{
	PersistentKeyValueStorage::SetValue("MercuryEnginePath", gMercuryEnginePath);
}

bool isDirectoryExists(const std::filesystem::path& path)
{
	return std::filesystem::exists(path) && std::filesystem::is_directory(path);
}

void ValidateEnginePath(const std::string& u8path)
{
	std::filesystem::path path(u8path);

	gIsEnginePathValid = false;

	if(std::filesystem::is_empty(path))	
		return;
	
	if(!isDirectoryExists(path))
		return;

	std::filesystem::path enginePath;
	std::filesystem::path engineShadersPath;
	std::filesystem::path editorPath;
	std::filesystem::path editorShadersPath;

	enginePath = path / "engine";
	engineShadersPath = enginePath / "shaders";
	editorPath = path / "editor";
	editorShadersPath = editorPath / "shaders";

	if (!isDirectoryExists(enginePath) || !isDirectoryExists(engineShadersPath))
		return;
	if (!isDirectoryExists(editorPath) || !isDirectoryExists(editorShadersPath))
		return;

	if(gEngineRootPath != enginePath)
	{
		// If the engine path has changed, we might need to do some reinitialization here
		// For now, just log it
		MLOG_INFO(u8"Engine path changed to: %s\n", enginePath.u8string().c_str());

		gEngineRootPath = enginePath;
		gEngineShadersPath = engineShadersPath;
		gEditorRootPath = editorPath;
		gEditorShadersPath = editorShadersPath;

		ShellOS::SetupFolderWatcher(gEngineShadersPath, [](const std::filesystem::path& changedPath, ShellOS::WatcherAction action)
		{
			MLOG_INFO(u8"Engine shader changed: %s\n", changedPath.u8string().c_str());
		// Here you would trigger a recompile or reload of the changed shader
		});

		ShellOS::SetupFolderWatcher(gEditorShadersPath, [](const std::filesystem::path& changedPath, ShellOS::WatcherAction action)
		{
			MLOG_INFO(u8"Editor shader changed: %s\n", changedPath.u8string().c_str());
		// Here you would trigger a recompile or reload of the changed shader
		});
	}




	MLOG_INFO(u8"Engine path successfully validated: %s\n", enginePath.u8string().c_str());
	gIsEnginePathValid = true;
}

export namespace EditorOptions
{
	void Initialize()
	{
		auto enginePath = PersistentKeyValueStorage::GetStringValue("MercuryEnginePath","");

		ValidateEnginePath(enginePath);

		if (gIsEnginePathValid)
		{
			strncpy_s(gMercuryEnginePath, enginePath.c_str(), sizeof(gMercuryEnginePath) - 1);
		}		
	}

	void ShowEditorOptionsWindow()
	{
		isEditorOptionsWindowOpen = true;
	}

	void ProcessImguiWindow()
	{
		if (isEditorOptionsWindowOpen)
		{
			if (ImGui::Begin("Editor Options", &isEditorOptionsWindowOpen))
			{
				ImGui::InputText("Mercury Engine Path", gMercuryEnginePath, sizeof(gMercuryEnginePath));

				ImGui::SameLine();
				if (ImGui::Button("Browse..."))
				{
					auto selectedPath = ShellOS::SelectFolderDialog(std::filesystem::path(gMercuryEnginePath));
					if (!selectedPath.empty())
					{
						auto str_path = selectedPath.u8string();

						ValidateEnginePath(selectedPath.string());

						if(gIsEnginePathValid)
						{
							strncpy_s(gMercuryEnginePath, (const char*)str_path.c_str(), sizeof(gMercuryEnginePath) - 1);
							gMercuryEnginePath[sizeof(gMercuryEnginePath) - 1] = '\0';
						}
					}
				}

				if(!gIsEnginePathValid)
				{
					ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Invalid Mercury Engine Path");
				}

				if (ImGui::Button("OK", ImVec2(120, 0)))
				{
					SaveEditorOptions();
					isEditorOptionsWindowOpen = false;
				}

				ImGui::SameLine();
				if (ImGui::Button("Cancel", ImVec2(120, 0)))
				{
					isEditorOptionsWindowOpen = false;
				}
				ImGui::End();
			}
		}
	}

	std::filesystem::path GetMercuryEngineRootPath()
	{
		return gEngineRootPath;
	}

	std::filesystem::path GetMercuryEngineShadersPath()
	{
		return gEngineShadersPath;
	}

	std::filesystem::path GetMercuryEditorRootPath()
	{
		return gEditorRootPath;
	}

	std::filesystem::path GetMercuryEditorShadersPath()
	{
		return gEditorShadersPath;
	}
}