#include "linux_display_server.h"

#ifdef MERCURY_LL_OS_LINUX
#include <wayland-client.h>
#include <wayland-client-protocol.h>
#include <iostream>
#include <cstring>
#include <cstdlib>
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>

// We need to include the XDG shell header
#include "xdg-shell-client-protocol.h"

#include "backends/imgui_impl_wayland.h"

// Global objects for Wayland protocols
static wl_display* gDisplay = nullptr;
static wl_compositor* gCompositor = nullptr;
static wl_surface* gSurface = nullptr;
static xdg_wm_base* gXdgWmBase = nullptr;
static xdg_surface* gXdgSurface = nullptr;
static xdg_toplevel* gXdgToplevel = nullptr;
static wl_seat* gSeat = nullptr;
static wl_pointer* gPointer = nullptr;
static wl_keyboard* gKeyboard = nullptr;
static bool gIsWindowReady = false;
static int width = 800;
static int height = 600;
static bool running = true;
static bool gConfigured = false;

    // Registry listener
    static void HandleRegistryGlobal(void* data, wl_registry* registry, uint32_t name, 
                                   const char* interface, uint32_t version);
    static void HandleRegistryGlobalRemove(void* data, wl_registry* registry, uint32_t name);
    
    // XDG surface listener
    static void HandleXdgSurfaceConfigure(void* data, xdg_surface* xdg_surface, uint32_t serial);
    
    // XDG toplevel listener
    static void HandleXdgToplevelConfigure(void* data, xdg_toplevel* xdg_toplevel, 
                                         int32_t width, int32_t height, wl_array* states);
    static void HandleXdgToplevelClose(void* data, xdg_toplevel* xdg_toplevel);
    
    // XDG WM base listener
    static void HandleXdgWmBasePing(void* data, xdg_wm_base* xdg_wm_base, uint32_t serial);
    
    // Buffer creation helper
    static wl_buffer* CreateBuffer(int width, int height);
    

class WaylandDisplayServer : public LinuxDisplayServer
{
public:
    virtual ~WaylandDisplayServer();
    bool Initialize() override;
    void CreateWindow(const mercury::ll::os::OS::NativeWindowDescriptor &desc) override;
    void DestroyWindow() override;
    bool ProcessEvents() override;
    void* GetWindowNativeHandle() override
    {
        return gSurface;
    }
    virtual bool IsNativeWindowReady() override
    {
        return gIsWindowReady;
    }
    virtual void GetActualWindowSize(unsigned int& widthOut, unsigned int& heightOut) override
    {
        widthOut = width;
        heightOut = height;
    }
    virtual VkSurfaceKHR CreateSurface(void* vk_instance, void* allocations_callback) override;
    virtual bool CheckIfPresentSupportedOnQueue(void* vk_physical_device, mercury::u32 queueIndex) override;

    virtual void ImguiInitialize() override;
    virtual void ImguiNewFrame() override;
    virtual void ImguiShutdown() override;
private:
};

// Define static listeners
const wl_registry_listener sRegistryListener = {
    HandleRegistryGlobal,
    HandleRegistryGlobalRemove
};

const xdg_wm_base_listener sXdgWmBaseListener = {
    HandleXdgWmBasePing
};

const xdg_surface_listener sXdgSurfaceListener = {
    HandleXdgSurfaceConfigure
};

const xdg_toplevel_listener sXdgToplevelListener = {
    HandleXdgToplevelConfigure,
    HandleXdgToplevelClose
};

WaylandDisplayServer::~WaylandDisplayServer()
{
    DestroyWindow();
    
    if (gXdgToplevel) xdg_toplevel_destroy(gXdgToplevel);
    if (gXdgSurface) xdg_surface_destroy(gXdgSurface);
    if (gXdgWmBase) xdg_wm_base_destroy(gXdgWmBase);
    if (gCompositor) wl_compositor_destroy(gCompositor);
    if (gDisplay) wl_display_disconnect(gDisplay);
    
    gDisplay = nullptr;
    gCompositor = nullptr;
    gSurface = nullptr;
    gXdgWmBase = nullptr;
    gXdgSurface = nullptr;
    gXdgToplevel = nullptr;
}

static void HandlePointerEnter(void* data, wl_pointer* pointer, uint32_t serial,
                              wl_surface* surface, wl_fixed_t x, wl_fixed_t y) {
    // Notify ImGui about mouse enter
}

static void HandlePointerLeave(void* data, wl_pointer* pointer, uint32_t serial,
                              wl_surface* surface) {
    // Notify ImGui about mouse leave
}

static void HandlePointerMotion(void* data, wl_pointer* pointer, uint32_t time,
                               wl_fixed_t x, wl_fixed_t y) {
    // Convert fixed-point to integer coordinates
    float mouseX = wl_fixed_to_double(x);
    float mouseY = wl_fixed_to_double(y);

    printf("mouse pos: %f %f\n",(float)mouseX,(float)mouseY);
    // Forward to ImGui IO
    ImGuiIO& io = ImGui::GetIO();
    io.AddMousePosEvent(mouseX, mouseY);
}

static void HandlePointerButton(void* data, wl_pointer* pointer, uint32_t serial,
                               uint32_t time, uint32_t button, uint32_t state) {
    // Map Wayland button to ImGui button
    // ImGuiMouseButton imguiButton;
    // if (button == BTN_LEFT) imguiButton = ImGuiMouseButton_Left;
    // else if (button == BTN_RIGHT) imguiButton = ImGuiMouseButton_Right;
    // else if (button == BTN_MIDDLE) imguiButton = ImGuiMouseButton_Middle;
    // else return;

    // bool pressed = (state == WL_POINTER_BUTTON_STATE_PRESSED);
    // io.AddMouseButtonEvent(imguiButton, pressed);
}

static void HandlePointerAxis(void* data, wl_pointer* pointer, uint32_t time,
                             uint32_t axis, wl_fixed_t value) {
    // Handle mouse wheel
    // if (axis == WL_POINTER_AXIS_VERTICAL_SCROLL) {
    //     float scrollY = wl_fixed_to_double(value);
    //     io.AddMouseWheelEvent(0, scrollY);
    // }
}

const wl_pointer_listener sPointerListener = {
    HandlePointerEnter,
    HandlePointerLeave,
    HandlePointerMotion,
    HandlePointerButton,
    HandlePointerAxis
};

static void HandleKeyboardKeymap(void* data, wl_keyboard* keyboard, uint32_t format,
                                int fd, uint32_t size) {
    // Handle keymap (e.g., XKB keymap)
}

static void HandleKeyboardEnter(void* data, wl_keyboard* keyboard, uint32_t serial,
                               wl_surface* surface, wl_array* keys) {
    // Keyboard focus entered the surface
}

static void HandleKeyboardLeave(void* data, wl_keyboard* keyboard, uint32_t serial,
                               wl_surface* surface) {
    // Keyboard focus left the surface
}

static void HandleKeyboardKey(void* data, wl_keyboard* keyboard, uint32_t serial,
                             uint32_t time, uint32_t key, uint32_t state) {
    bool pressed = (state == WL_KEYBOARD_KEY_STATE_PRESSED);
    // Map Wayland key to ImGui key (may require keymap translation)
   // io.AddKeyEvent(ImGuiKey_Space, pressed); // Example; need proper mapping
}

static void HandleKeyboardModifiers(void* data, wl_keyboard* keyboard, uint32_t serial,
                                   uint32_t mods_depressed, uint32_t mods_latched,
                                   uint32_t mods_locked, uint32_t group) {
    // Handle modifiers (Shift, Ctrl, etc.)
}

const wl_keyboard_listener sKeyboardListener = {
    HandleKeyboardKeymap,
    HandleKeyboardEnter,
    HandleKeyboardLeave,
    HandleKeyboardKey,
    HandleKeyboardModifiers
};

static void HandleSeatCapabilities(void* data, wl_seat* seat, uint32_t capabilities) {
    // Check for pointer capability
    if (capabilities & WL_SEAT_CAPABILITY_POINTER) {
        gPointer = wl_seat_get_pointer(seat);
        wl_pointer_add_listener(gPointer, &sPointerListener, nullptr);
    }
    // Check for keyboard capability
    if (capabilities & WL_SEAT_CAPABILITY_KEYBOARD) {
       gKeyboard = wl_seat_get_keyboard(seat);
       wl_keyboard_add_listener(gKeyboard, &sKeyboardListener, nullptr);
   }
}

static void HandleSeatName(void* data, wl_seat* seat, const char* name) {
    // Optional: Handle seat name
}

const wl_seat_listener sSeatListener = {
    HandleSeatCapabilities,
    HandleSeatName
};

LinuxDisplayServer* LinuxDisplayServer::CreateWayland()
{
    return new WaylandDisplayServer();
}

bool WaylandDisplayServer::Initialize()
{
    // Connect to the Wayland display
    gDisplay = wl_display_connect(nullptr);
    if (!gDisplay) {
        std::cerr << "Failed to connect to Wayland display." << std::endl;
        return false;
    }

    // Get registry and bind objects
    wl_registry* registry = wl_display_get_registry(gDisplay);
    wl_registry_add_listener(registry, &sRegistryListener, nullptr);
    
    // First roundtrip to get the registry globals
    wl_display_roundtrip(gDisplay);
    
    // Check if we have the required globals
    if (!gCompositor) {
        std::cerr << "No compositor available." << std::endl;
        return false;
    }
    
    if (!gXdgWmBase) {
        std::cerr << "No XDG shell available." << std::endl;
        return false;
    }
    
    // Add listener to XDG WM base
    xdg_wm_base_add_listener(gXdgWmBase, &sXdgWmBaseListener, nullptr);

    return true;
}

void WaylandDisplayServer::CreateWindow(const mercury::ll::os::OS::NativeWindowDescriptor &desc)
{
    width = desc.width;
    height = desc.height;
    
    // Create surface
    gSurface = wl_compositor_create_surface(gCompositor);
    if (!gSurface) {
        std::cerr << "Failed to create surface." << std::endl;
        return;
    }

    // Create XDG surface
    gXdgSurface = xdg_wm_base_get_xdg_surface(gXdgWmBase, gSurface);
    if (!gXdgSurface) {
        std::cerr << "Failed to create xdg surface." << std::endl;
        return;
    }
    
    xdg_surface_add_listener(gXdgSurface, &sXdgSurfaceListener, nullptr);

    // Create XDG toplevel
    gXdgToplevel = xdg_surface_get_toplevel(gXdgSurface);
    if (!gXdgToplevel) {
        std::cerr << "Failed to create xdg toplevel." << std::endl;
        return;
    }
    
    xdg_toplevel_add_listener(gXdgToplevel, &sXdgToplevelListener, nullptr);
    xdg_toplevel_set_title(gXdgToplevel, (const char*)desc.title.c_str());
    
    // Set window size (this is a hint, the compositor may ignore it)
    xdg_toplevel_set_min_size(gXdgToplevel, desc.width, desc.height);
    xdg_toplevel_set_max_size(gXdgToplevel, desc.width, desc.height);
    
    // Commit the surface to make the window visible
    wl_surface_commit(gSurface);
    
    std::cout << "Window created with size " << desc.width << "x" << desc.height << std::endl;

    // Wait for the initial configure event
    while (!gConfigured && running) {
        wl_display_dispatch(gDisplay);
    }
    
    wl_seat_add_listener(gSeat, &sSeatListener, nullptr);
}

void WaylandDisplayServer::DestroyWindow()
{
    if (gXdgToplevel) {
        xdg_toplevel_destroy(gXdgToplevel);
        gXdgToplevel = nullptr;
    }
    
    if (gXdgSurface) {
        xdg_surface_destroy(gXdgSurface);
        gXdgSurface = nullptr;
    }
    
    if (gSurface) {
        wl_surface_destroy(gSurface);
        gSurface = nullptr;
    }
    
    gConfigured = false;
    gIsWindowReady = false;
}

// Registry global handler
void HandleRegistryGlobal(void* data, wl_registry* registry, uint32_t name, 
                                              const char* interface, uint32_t version)
{
    if (std::strcmp(interface, "wl_compositor") == 0) {
        gCompositor = static_cast<wl_compositor*>(
            wl_registry_bind(registry, name, &wl_compositor_interface, 3));
    } else if (std::strcmp(interface, "xdg_wm_base") == 0) {
        gXdgWmBase = static_cast<xdg_wm_base*>(
            wl_registry_bind(registry, name, &xdg_wm_base_interface, 1));
    } else if (std::strcmp(interface, "wl_seat") == 0) {
        gSeat = static_cast<wl_seat*>(
            wl_registry_bind(registry, name, &wl_seat_interface, 7)); // Use version 7
    }
    
}

// Registry global remove handler
void HandleRegistryGlobalRemove(void* data, wl_registry* registry, uint32_t name)
{
    printf("HandleRegistryGlobalRemove\n");
    // We don't handle this for simplicity
}

// XDG WM base ping handler
void HandleXdgWmBasePing(void* data, xdg_wm_base* xdg_wm_base, uint32_t serial)
{
    xdg_wm_base_pong(xdg_wm_base, serial);
}

// XDG surface configure handler
void HandleXdgSurfaceConfigure(void* data, xdg_surface* xdg_surface, uint32_t serial)
{
            printf("HandleXdgSurfaceConfigure\n");
    xdg_surface_ack_configure(xdg_surface, serial);
    gConfigured = true;
}

// XDG toplevel configure handler
void HandleXdgToplevelConfigure(void* data, xdg_toplevel* xdg_toplevel, 
                                                    int32_t new_width, int32_t new_height, wl_array* states)
{
                printf("HandleXdgToplevelConfigure\n");
    // Handle window configuration changes
    if (new_width > 0 && new_height > 0) {
        width = new_width;
        height = new_height;
        std::cout << "Window configured to size " << width << "x" << height << std::endl;

        gIsWindowReady = true;
    }


}

// XDG toplevel close handler
void HandleXdgToplevelClose(void* data, xdg_toplevel* xdg_toplevel)
{
                    printf("HandleXdgToplevelClose\n");
    // The compositor requested the window to close
    running = false;
    std::cout << "Window close requested by compositor." << std::endl;
}

bool WaylandDisplayServer::ProcessEvents()
{
    if (gDisplay) {
        // Dispatch events without blocking
        wl_display_dispatch_pending(gDisplay);
        wl_display_flush(gDisplay);
        return running;
    }
    return false;
}

VkSurfaceKHR WaylandDisplayServer::CreateSurface(void* vk_instance, void* allocations_callback)
{
    #ifdef MERCURY_LL_GRAPHICS_VULKAN
    VkWaylandSurfaceCreateInfoKHR createInfo{VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR};
	createInfo.surface = gSurface;
	createInfo.display = gDisplay;

    VkSurfaceKHR result = VK_NULL_HANDLE;
    VK_CALL(vkCreateWaylandSurfaceKHR((VkInstance)vk_instance, &createInfo, (VkAllocationCallbacks*)allocations_callback,&result));

    return result;
    #else
    return nullptr;
    #endif
}

bool WaylandDisplayServer::CheckIfPresentSupportedOnQueue(void* vk_physical_device, mercury::u32 queueIndex)
{
    #ifdef MERCURY_LL_GRAPHICS_VULKAN
    if(!vkGetPhysicalDeviceWaylandPresentationSupportKHR)
        return false;

    return vkGetPhysicalDeviceWaylandPresentationSupportKHR((VkPhysicalDevice)vk_physical_device, queueIndex, gDisplay);
    #else
    return false;
    #endif
}

    void WaylandDisplayServer::ImguiInitialize()
    {
        ImGui_ImplWayland_Init(gDisplay, gSurface);
    }

    void WaylandDisplayServer::ImguiNewFrame()
    {
        ImGui_ImplWayland_NewFrame();
    }

    void WaylandDisplayServer::ImguiShutdown()
    {
        ImGui_ImplWayland_Shutdown();
    }
#endif