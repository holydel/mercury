module;
#include <imgui.h>
#include "IconsFontAwesome7.h"

export module EditorMainWindow;

import ShellOS;
import EditorState;
import ImguiState;

import AssetsBrowserWindow;
import SceneHierarchyWindow;
import PropertyWindow;
import EditorOptions;
import ShaderCompiler;

export class EditorMainWindow
{
	void ProcessMainMenu();
	bool showAboutModalWindow = false;
    
    // Track first-time layout initialization
    bool firstTime = true;	
public:
    void ProcessImgui();


};


void EditorMainWindow::ProcessMainMenu()
{
	ImGui::BeginMainMenuBar();
	if (ImGui::BeginMenu("File"))
	{
		if (ImGui::MenuItem("New Project","Ctrl+N"))
		{
			auto path = ShellOS::SelectFolderDialog(ShellOS::GetUserDocumentsDirectory());
			if (!path.empty())
			{
				EditorState::gCurrentProject.CreateNew(path.u8string().c_str());
			}
		}

		if (ImGui::MenuItem("Open Project","CTRL+O"))
		{
			auto path = ShellOS::SelectFolderDialog(ShellOS::GetUserDocumentsDirectory());
			if (!path.empty())
			{
				EditorState::gCurrentProject.LoadFromFolder(path.u8string().c_str());
			}
		}

		if (ImGui::MenuItem("Save Project", "CTRL+S"))
		{
		}

		if(ImGui::MenuItem("Save Project As..."))
		{
		}

		if(ImGui::BeginMenu("Recent projects"))
		{
			if(ImGui::MenuItem("Project 1"))
			{
				EditorState::gCurrentProject.LoadFromFolder(u8"D:\\Projects\\mercury\\sample_project");
			}
			if(ImGui::MenuItem("Project 2"))
			{
				EditorState::gCurrentProject.LoadFromFolder(u8"D:\\Projects\\mercury\\new_sample_project");
			}
			ImGui::EndMenu();
		}

		if (ImGui::MenuItem("Close", "ALT+F4"))
		{
			EditorState::SetRunning(false);
		}

		ImGui::EndMenu();
	}

	if (ImGui::BeginMenu("Edit"))
	{
		if (ImGui::MenuItem(ICON_FA_CARET_LEFT " Undo", "CTRL+Z")) {}
		if (ImGui::MenuItem(ICON_FA_CARET_RIGHT "Redo", "CTRL+Y", false, false)) {}  // Disabled item
		ImGui::Separator();
		if (ImGui::MenuItem(ICON_FA_SCISSORS " Cut", "CTRL+X")) {}
		if (ImGui::MenuItem(ICON_FA_COPY " Copy", "CTRL+C")) {}
		if (ImGui::MenuItem(ICON_FA_PASTE " Paste", "CTRL+V")) {}
		ImGui::EndMenu();
	}

	if (ImGui::BeginMenu("Project"))
	{
		if (ImGui::MenuItem(ICON_FA_GEARS " Build Project", "F7"))
		{
		}
		ImGui::EndMenu();
	}

	if (ImGui::BeginMenu("Tools"))
	{
		if (ImGui::MenuItem(ICON_FA_GEAR " Options")) {
			EditorOptions::ShowEditorOptionsWindow();
		}

		ImGui::Separator();

		if (ImGui::MenuItem(ICON_FA_SHAPES "Recompile engine embedded shaders")) {

			ShaderCompiler::RebuildShaderDesc desc;

			desc.shadersFolder = EditorOptions::GetMercuryEngineShadersPath();
			desc.outputHeaderPath = EditorOptions::GetMercuryEngineRootPath() / "include" / "mercury_embedded_shaders.h";
			desc.outputSourceSPIRVPath = EditorOptions::GetMercuryEngineRootPath() / "src/ll/graphics/vulkan/embedded_shaders_spirv.cpp";
			desc.outputSourceDXILPath = EditorOptions::GetMercuryEngineRootPath() / "src/ll/graphics/d3d12/embedded_shaders_dxil.cpp";
			desc.outputSourceMetalPath = EditorOptions::GetMercuryEngineRootPath() / "src/ll/graphics/metal/embedded_shaders_metal.cpp";
			desc.outputSourceWGSLPath = EditorOptions::GetMercuryEngineRootPath() / "src/ll/graphics/webgpu/embedded_shaders_wgsl.cpp";

			ShaderCompiler::RebuildEmbeddedShaders(desc);
		}

		if (ImGui::MenuItem(ICON_FA_SHAPES "Recompile editor embedded shaders")) {
			
			ShaderCompiler::RebuildShaderDesc desc;

			desc.shadersFolder = EditorOptions::GetMercuryEditorShadersPath();
			desc.outputHeaderPath = EditorOptions::GetMercuryEditorRootPath() / "include" / "editor_embedded_shaders.h";
			desc.outputSourceSPIRVPath = EditorOptions::GetMercuryEditorRootPath() / "src/editor_embedded_shaders_spirv.cpp";
			desc.outputSourceDXILPath = EditorOptions::GetMercuryEditorRootPath() / "src/editor_embedded_shaders_dxil.cpp";
			desc.outputSourceMetalPath = EditorOptions::GetMercuryEditorRootPath() / "src/editor_embedded_shaders_metal.cpp";
			desc.outputSourceWGSLPath = EditorOptions::GetMercuryEditorRootPath() / "src/editor_embedded_shaders_wgsl.cpp";

			ShaderCompiler::RebuildEmbdeddedShaders(desc);
		}

		ImGui::EndMenu();
	}

	if (ImGui::BeginMenu("Help"))
	{
		if (ImGui::MenuItem("About")) {
			showAboutModalWindow = true;
		}
		ImGui::EndMenu();
	}
	ImGui::EndMainMenuBar();
}

void EditorMainWindow::ProcessImgui()
{
	ImGui::PushFont(ImguiState::GetMainFont());
	ProcessMainMenu();

    // Create the docking environment
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);
    ImGui::SetNextWindowViewport(viewport->ID);
    
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
    window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
    window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoBackground;
    
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::Begin("DockSpace", nullptr, window_flags);
    ImGui::PopStyleVar();
    
    // Submit the DockSpace
    ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
    ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_PassthruCentralNode);
    
    // Set up default docking layout the first time
    //if (firstTime) {
    //    firstTime = false;
    //    ImGui::DockBuilderRemoveNode(dockspace_id);
    //    ImGui::DockBuilderAddNode(dockspace_id, ImGuiDockNodeFlags_DockSpace);
    //    ImGui::DockBuilderSetNodeSize(dockspace_id, viewport->Size);
    //    
    //    // Create layout: scene hierarchy on left, properties on right, assets at bottom
    //    ImGuiID dock_main_id = dockspace_id;
    //    ImGuiID dock_id_left = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Left, 0.2f, nullptr, &dock_main_id);
    //    ImGuiID dock_id_right = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Right, 0.25f, nullptr, &dock_main_id);
    //    ImGuiID dock_id_bottom = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Down, 0.25f, nullptr, &dock_main_id);
    //    
    //    ImGui::DockBuilderDockWindow("Scene Hierarchy", dock_id_left);
    //    ImGui::DockBuilderDockWindow("Properties", dock_id_right);
    //    ImGui::DockBuilderDockWindow("Assets Browser", dock_id_bottom);
    //    
    //    ImGui::DockBuilderFinish(dockspace_id);
    //}
    
    ImGui::End(); // End dockspace

	if (showAboutModalWindow)
	{
		if (ImGui::Begin("About Mercury Editor", &showAboutModalWindow, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoDocking))
		{
			ImGui::Text("Mercury Engine Editor");
			ImGui::Text("Version 0.1.0");
			ImGui::Text("© 2025 Mercury Engine");
			ImGui::Separator();
			if (ImGui::Button("OK", ImVec2(120, 0))) { showAboutModalWindow = false; ImGui::CloseCurrentPopup(); }
			ImGui::End();
		}
	}

	EditorOptions::ProcessImguiWindow();
	
	AssetsBrowserWindow::Get().ProcessImgui();
	SceneHierarchyWindow::Get().ProcessImgui();
	PropertyWindow::Get().ProcessImgui();

	ImGui::PopFont();
}