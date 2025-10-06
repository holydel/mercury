#include "mercury_imgui.h"
#include "imgui_impl.h"
#include <mercury_input.h>
#include <mercury_ui.h>
#include <ll/os.h>
#include <ll/graphics.h>


#ifdef MERCURY_LL_GRAPHICS_D3D12
#include "../ll/graphics/d3d12/d3d12_graphics.h"
static ID3D12DescriptorHeap* gImgui_pd3dSrvDescHeap = nullptr;
#endif

#include "../application.h"
#include <mercury_input.h>

void mercury_imgui::Initialize()
{
	auto app = mercury::Application::GetCurrentApplication();
    auto& config = app->GetConfig();
    auto& imguiCfg = config.imgui;

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

	//io.ConfigFlags |= ImGuiConfigFlags_NoKeyboard;
	//io.ConfigFlags |= ImGuiConfigFlags_NoMouse;
	if (imguiCfg.enableDocking)
		io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;         // IF using Docking Branch	

	ImGui::StyleColorsDark();

	mercury::ll::graphics::gDevice->ImguiInitialize();

#ifdef MERCURY_LL_GRAPHICS_D3D12
	D3D12_DESCRIPTOR_HEAP_DESC desc = {};
	desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	desc.NumDescriptors = 20;
	desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	gD3DDevice->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&gImgui_pd3dSrvDescHeap));

	ImGui_ImplDX12_InitInfo init_info = {};
	init_info.CommandQueue = gD3DCommandQueue;
	init_info.Device = gD3DDevice;	
	init_info.SrvDescriptorHeap = gImgui_pd3dSrvDescHeap;
	init_info.NumFramesInFlight = 3;
	init_info.RTVFormat = gD3DSwapChainFormat;
	init_info.LegacySingleSrvCpuDescriptor = gImgui_pd3dSrvDescHeap->GetCPUDescriptorHandleForHeapStart();
	init_info.LegacySingleSrvGpuDescriptor = gImgui_pd3dSrvDescHeap->GetGPUDescriptorHandleForHeapStart();
	ImGui_ImplDX12_Init(&init_info);

	ImGui_ImplDX12_CreateDeviceObjects();
	//ImGui_ImplDX12_CreateFontsTexture();
#endif

#ifdef MERCURY_LL_GRAPHICS_WEBGPU
	ImGui_ImplWGPU_InitInfo initInfo = {};
	initInfo.Device = gDevice;
	initInfo.RenderTargetFormat = gPrefferedBackbufferFormat;
	initInfo.NumFramesInFlight = 3;
	initInfo.DepthStencilFormat = WGPUTextureFormat::WGPUTextureFormat_Undefined;

	ImGui_ImplWGPU_Init(&initInfo);
#endif

	mercury::ll::os::gOS->ImguiInitialize();

#ifdef MERCURY_LL_OS_LINUX
	//auto win = static_cast<xcb_window_t*>(mercury::platform::getMainWindowHandle());
	//auto connection = static_cast<xcb_connection_t*>(mercury::platform::getAppInstanceHandle());
	//ImGui_ImplX11_Init(connection, win);
#endif
#ifdef MERCURY_LL_OS_ANDROID
	ImGui_ImplAndroid_Init(static_cast<ANativeWindow*>(os::GetNativeWindowHandle()));
	ImGui::GetIO().FontGlobalScale = 2.0f;
#endif
#ifdef MERCURY_LL_OS_MACOS
	void* view = mercury::platform::getAppInstanceHandle(); //view
	ImGui_ImplOSX_Init(view);
#endif
#ifdef MERCURY_LL_OS_EMSCRIPTEN
	ImGui_ImplEmscripten_Init();
#endif
}

void mercury_imgui::Shutdown()
{
	mercury::ll::graphics::gDevice->ImguiShutdown();

#ifdef MERCURY_LL_GRAPHICS_D3D12
		ImGui_ImplDX12_Shutdown();
#endif
#ifdef MERCURY_LL_GRAPHICS_WEBGPU
		ImGui_ImplWGPU_Shutdown();
#endif
#ifdef MERCURY_LL_OS_ANDROID
		ImGui_ImplAndroid_Shutdown();
#endif
#ifdef MERCURY_LL_OS_EMSCRIPTEN
		ImGui_ImplEmscripten_Shutdown();
#endif

	ImGui::DestroyContext();
}

void mercury_imgui::BeginFrame(mercury::ll::graphics::CommandList cmdList)
{
	using namespace mercury::ll;

	graphics::gDevice->ImguiNewFrame();
	os::gOS->ImguiNewFrame();

#ifdef MERCURY_LL_GRAPHICS_D3D12
	ImGui_ImplDX12_NewFrame();
#endif
#ifdef MERCURY_LL_GRAPHICS_WEBGPU
	ImGui_ImplWGPU_NewFrame();
#endif

#ifdef MERCURY_LL_OS_LINUX

	//ImGui_ImplX11_NewFrame();
#endif
#ifdef MERCURY_LL_OS_ANDROID
	ImGui_ImplAndroid_NewFrame();
#endif
#ifdef MERCURY_LL_OS_MACOS
	void* view = mercury::platform::getAppInstanceHandle(); //view
	ImGui_ImplOSX_NewFrame(view);
#endif
#ifdef MERCURY_LL_OS_EMSCRIPTEN
	ImGui_ImplEmscripten_Event();
	ImGui_ImplEmscripten_NewFrame();
#endif
	ImGui::NewFrame();
}

void DrawGamePad(ImDrawList* draw_list, const mercury::input::Gamepad* gamePad) {
	float x = ImGui::GetCursorScreenPos().x;
	float y = ImGui::GetCursorScreenPos().y;

	// Define positions for gamepad elements
	// Shoulder buttons (LB and RB)
	ImVec2 lb_min = ImVec2(x + 20, y + 10);
	ImVec2 lb_max = ImVec2(x + 60, y + 20);
	ImVec2 rb_min = ImVec2(x + 140, y + 10);
	ImVec2 rb_max = ImVec2(x + 180, y + 20);

	// D-pad (Up, Down, Left, Right)
	ImVec2 dpad_up_min = ImVec2(x + 40, y + 40);
	ImVec2 dpad_up_max = ImVec2(x + 60, y + 60);
	ImVec2 dpad_down_min = ImVec2(x + 40, y + 60);
	ImVec2 dpad_down_max = ImVec2(x + 60, y + 80);
	ImVec2 dpad_left_min = ImVec2(x + 20, y + 60);
	ImVec2 dpad_left_max = ImVec2(x + 40, y + 80);
	ImVec2 dpad_right_min = ImVec2(x + 60, y + 60);
	ImVec2 dpad_right_max = ImVec2(x + 80, y + 80);

	// Face buttons (X, Y, A, B)
	float button_radius = 5.0f;
	ImVec2 face_x_center = ImVec2(x + 140, y + 60); // Left
	ImVec2 face_y_center = ImVec2(x + 150, y + 50); // Top
	ImVec2 face_a_center = ImVec2(x + 150, y + 70); // Bottom
	ImVec2 face_b_center = ImVec2(x + 160, y + 60); // Right

	// Analog sticks
	float stick_radius = 20.0f;
	ImVec2 left_stick_base = ImVec2(x + 50, y + 100);
	ImVec2 right_stick_base = ImVec2(x + 150, y + 100);

	// Triggers
	ImVec2 lt_bar_min = ImVec2(x + 20, y + 130);
	ImVec2 lt_bar_max = ImVec2(x + 80, y + 140);
	ImVec2 rt_bar_min = ImVec2(x + 120, y + 130);
	ImVec2 rt_bar_max = ImVec2(x + 180, y + 140);

	// Back and Start buttons
	ImVec2 back_min = ImVec2(x + 80, y + 50);
	ImVec2 back_max = ImVec2(x + 90, y + 60);
	ImVec2 start_min = ImVec2(x + 110, y + 50);
	ImVec2 start_max = ImVec2(x + 120, y + 60);

	// Stick press buttons (LS and RS)
	ImVec2 ls_center = ImVec2(x + 50, y + 120);
	ImVec2 rs_center = ImVec2(x + 150, y + 120);

	// Define colors
	ImU32 color_button_normal = IM_COL32(100, 100, 100, 255);   // Dark gray
	ImU32 color_button_pressed = IM_COL32(0, 255, 0, 255);      // Green
	ImU32 color_stick_base = IM_COL32(50, 50, 50, 255);         // Medium gray
	ImU32 color_stick_position = IM_COL32(255, 0, 0, 255);      // Red
	ImU32 color_trigger_bar = IM_COL32(200, 200, 200, 255);     // Light gray
	ImU32 color_trigger_fill = IM_COL32(0, 0, 255, 255);        // Blue
	ImU32 color_disabled = IM_COL32(50, 50, 50, 255);           // Dark gray for disabled

	// Adjust state based on enabled flag
	mercury::input::Gamepad::State draw_state = gamePad->GetState();
    bool enabled = gamePad->IsConnected();
	if (!enabled) {
		draw_state.leftStick = glm::vec2(0, 0);
		draw_state.rightStick = glm::vec2(0, 0);
		draw_state.leftTrigger = 0.0f;
		draw_state.rightTrigger = 0.0f;
		draw_state.A = draw_state.B = draw_state.X = draw_state.Y = false;
		draw_state.LB = draw_state.RB = draw_state.Back = draw_state.Start = false;
		draw_state.LS = draw_state.RS = draw_state.Up = draw_state.Down = false;
		draw_state.Left = draw_state.Right = false;
	}

	// Draw shoulder buttons
	draw_list->AddRectFilled(lb_min, lb_max, enabled && draw_state.LB ? color_button_pressed : (enabled ? color_button_normal : color_disabled));
	draw_list->AddRectFilled(rb_min, rb_max, enabled && draw_state.RB ? color_button_pressed : (enabled ? color_button_normal : color_disabled));

	// Draw D-pad
	draw_list->AddRectFilled(dpad_up_min, dpad_up_max, enabled && draw_state.Up ? color_button_pressed : (enabled ? color_button_normal : color_disabled));
	draw_list->AddRectFilled(dpad_down_min, dpad_down_max, enabled && draw_state.Down ? color_button_pressed : (enabled ? color_button_normal : color_disabled));
	draw_list->AddRectFilled(dpad_left_min, dpad_left_max, enabled && draw_state.Left ? color_button_pressed : (enabled ? color_button_normal : color_disabled));
	draw_list->AddRectFilled(dpad_right_min, dpad_right_max, enabled && draw_state.Right ? color_button_pressed : (enabled ? color_button_normal : color_disabled));

	// Draw face buttons
	draw_list->AddCircleFilled(face_x_center, button_radius, enabled && draw_state.X ? color_button_pressed : (enabled ? color_button_normal : color_disabled));
	draw_list->AddCircleFilled(face_y_center, button_radius, enabled && draw_state.Y ? color_button_pressed : (enabled ? color_button_normal : color_disabled));
	draw_list->AddCircleFilled(face_a_center, button_radius, enabled && draw_state.A ? color_button_pressed : (enabled ? color_button_normal : color_disabled));
	draw_list->AddCircleFilled(face_b_center, button_radius, enabled && draw_state.B ? color_button_pressed : (enabled ? color_button_normal : color_disabled));

	// Draw analog sticks
	draw_list->AddCircleFilled(left_stick_base, stick_radius, color_stick_base);
	ImVec2 left_stick_pos = ImVec2(left_stick_base.x + draw_state.leftStick.x * (stick_radius - 5), left_stick_base.y - draw_state.leftStick.y * (stick_radius - 5));
	draw_list->AddCircleFilled(left_stick_pos, 5.0f, color_stick_position);

	draw_list->AddCircleFilled(right_stick_base, stick_radius, color_stick_base);
	ImVec2 right_stick_pos = ImVec2(right_stick_base.x + draw_state.rightStick.x * (stick_radius - 5), right_stick_base.y - draw_state.rightStick.y * (stick_radius - 5));
	draw_list->AddCircleFilled(right_stick_pos, 5.0f, color_stick_position);

	// Draw triggers
	draw_list->AddRect(lt_bar_min, lt_bar_max, color_trigger_bar);
	float lt_fill_width = (lt_bar_max.x - lt_bar_min.x) * draw_state.leftTrigger;
	draw_list->AddRectFilled(lt_bar_min, ImVec2(lt_bar_min.x + lt_fill_width, lt_bar_max.y), color_trigger_fill);

	draw_list->AddRect(rt_bar_min, rt_bar_max, color_trigger_bar);
	float rt_fill_width = (rt_bar_max.x - rt_bar_min.x) * draw_state.rightTrigger;
	draw_list->AddRectFilled(rt_bar_min, ImVec2(rt_bar_min.x + rt_fill_width, rt_bar_max.y), color_trigger_fill);

	// Draw Back and Start buttons
	draw_list->AddRectFilled(back_min, back_max, enabled && draw_state.Back ? color_button_pressed : (enabled ? color_button_normal : color_disabled));
	draw_list->AddRectFilled(start_min, start_max, enabled && draw_state.Start ? color_button_pressed : (enabled ? color_button_normal : color_disabled));

	// Draw stick press buttons (LS and RS)
	draw_list->AddCircleFilled(ls_center, button_radius, enabled && draw_state.LS ? color_button_pressed : (enabled ? color_button_normal : color_disabled));
	draw_list->AddCircleFilled(rs_center, button_radius, enabled && draw_state.RS ? color_button_pressed : (enabled ? color_button_normal : color_disabled));
	
	// Draw battery progress bar
	ImVec2 progress_bar_min = ImVec2(x + 50, y + 150);
	ImVec2 progress_bar_max = ImVec2(x + 150, y + 160);
	draw_list->AddRectFilled(progress_bar_min, progress_bar_max, IM_COL32(200, 200, 200, 255)); // Light gray background

	if (enabled) {
        float battery_fraction = gamePad->GetBatteryLevel();
		float fill_width = (progress_bar_max.x - progress_bar_min.x) * battery_fraction;
		ImVec2 fill_max = ImVec2(progress_bar_min.x + fill_width, progress_bar_max.y);
		ImU32 fill_color;
		if (battery_fraction > 0.75f) {
			fill_color = IM_COL32(0, 255, 0, 255); // Green for high battery
		}
		else if (battery_fraction > 0.25f) {
			fill_color = IM_COL32(255, 255, 0, 255); // Yellow for medium battery
		}
		else {
			fill_color = IM_COL32(255, 0, 0, 255); // Red for low battery
		}
		draw_list->AddRectFilled(progress_bar_min, fill_max, fill_color);

		// Add battery level text
		const char* level_str;
		if (battery_fraction >= 1.0f) {
			level_str = "Full";
		}
		else if (battery_fraction >= 0.5f) {
			level_str = "Medium";
		}
		else if (battery_fraction >= 0.25f) {
			level_str = "Low";
		}
		else {
			level_str = "Empty";
		}
		draw_list->AddText(ImVec2(x + 155, y + 150), IM_COL32(255, 255, 255, 255), level_str);
	}
	else {
		draw_list->AddText(ImVec2(x + 155, y + 150), IM_COL32(255, 255, 255, 255), "N/A");
	}
	
	ImGui::Dummy(ImVec2(210, 160)); // Dummy space to avoid overlap with other UI elements
}

void DrawStatisticsWindow()
{
	ImGui::Begin("Statistics", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);

	ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
	ImGui::Separator();
	ImGui::Text("OS: %s", mercury::ll::os::gOS->GetName());
	ImGui::SameLine();
	ImGui::Text("GAPI: %s", mercury::ll::graphics::GetBackendName());
	ImGui::Separator();
	ImGui::Text("Processor: some cpu");
	ImGui::Text("GPU: some gpu");
	ImGui::Text("GPU: some gpu");
	ImGui::Text("RAM: some ram");
	ImGui::Text("Disk: some storage info");
	ImGui::Separator();

	if (ImGui::BeginTabBar("SystemTabs")) {
		if (ImGui::BeginTabItem("CPU")) {
			ImGui::Text("CPU Settings");
			ImGui::Text("Core Count: 8");
			ImGui::Text("Clock Speed: 3.6 GHz");
			///ImGui::SliderFloat("CPU Usage", &cpu_usage, 0.0f, 100.0f, "%.1f%%");
			ImGui::EndTabItem();
		}
		if (ImGui::BeginTabItem("Video")) {
			ImGui::Text("Video Settings");
			ImGui::Text("Resolution: 1920x1080");
			//ImGui::Checkbox("VSync", &vsync_enabled);
			//ImGui::ColorEdit3("Background Color", background_color);
			ImGui::EndTabItem();
		}
		if (ImGui::BeginTabItem("Sound")) {
			ImGui::Text("Sound Settings");
			//ImGui::SliderFloat("Volume", &volume, 0.0f, 100.0f, "%.1f%%");
			///ImGui::Checkbox("Mute", &mute_enabled);
			ImGui::EndTabItem();
		}
		if (ImGui::BeginTabItem("Input")) {
			ImGui::Text("Input Settings");
			ImGui::Text("Connected Devices: Keyboard, Mouse");
			//ImGui::Checkbox("Gamepad Support", &gamepad_enabled);

			for (int i = 0; i < 4; ++i)
			{
				DrawGamePad(ImGui::GetWindowDrawList(), mercury::input::gGamepads[i]);
				ImGui::SameLine();
			}

			ImGui::EndTabItem();
		}
		ImGui::EndTabBar();
	}
	ImGui::End();
}

void mercury_imgui::EndFrame(mercury::ll::graphics::CommandList cmdList)
{
	auto &io = ImGui::GetIO();

	//ImGui::ShowDemoWindow(); // Show demo window! :)
	//DrawStatisticsWindow();

	   // Optional: Draw a marker at the reported position for visual verification
    // You can add this to your ImGui rendering code
    ImGui::GetForegroundDrawList()->AddCircle(io.MousePos, 3, IM_COL32(255, 0, 0, 255));

	ImGui::Render();

	cmdList.RenderImgui();

#ifdef MERCURY_LL_GRAPHICS_D3D12
	auto cmdListD3D12 = static_cast<ID3D12GraphicsCommandList*>(cmdList);

	cmdListD3D12->SetDescriptorHeaps(1, &gImgui_pd3dSrvDescHeap);
	ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), cmdListD3D12);
#endif
#ifdef MERCURY_LL_GRAPHICS_WEBGPU
	auto cmdList = static_cast<WGPURenderPassEncoder>(ctx.impl);
	ImGui_ImplWGPU_RenderDrawData(ImGui::GetDrawData(), cmdList);
#endif
}

void mercury_imgui::Render()
{

}