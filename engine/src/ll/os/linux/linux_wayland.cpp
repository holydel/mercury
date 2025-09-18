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
#include "xdg-decoration.h"
#include "backends/imgui_impl_wayland.h"
#include <linux/input-event-codes.h>
#include <xkbcommon/xkbcommon.h>
#include <xkbcommon/xkbcommon-keysyms.h>
#include <mercury_application.h>

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
static int gWidth = 800;
static int gHeight = 600;
static bool running = true;
static bool gConfigured = false;
static struct xkb_context* xkb_context = nullptr;
static struct xkb_keymap* xkb_keymap = nullptr;
static struct xkb_state* xkb_state = nullptr;
static xkb_mod_mask_t shift_mask;
static xkb_mod_mask_t ctrl_mask;
static xkb_mod_mask_t alt_mask;
static xkb_mod_mask_t super_mask;
// Add these with your other global Wayland protocol objects
static zxdg_decoration_manager_v1* gDecorationManager = nullptr;
static zxdg_toplevel_decoration_v1* gToplevelDecoration = nullptr;

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
        widthOut = gWidth;
        heightOut = gHeight;
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

float gMouseX = 0;
float gMouseY = 0;

static void HandlePointerMotion(void* data, wl_pointer* pointer, uint32_t time,
                               wl_fixed_t x, wl_fixed_t y) {
    // Convert fixed-point to integer coordinates
    float mouseX = wl_fixed_to_double(x);
    float mouseY = wl_fixed_to_double(y);

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

    int btnIndex = button - BTN_MOUSE;
    static ImGuiMouseButton mappedButtons[] = {ImGuiMouseButton_Left,ImGuiMouseButton_Right,ImGuiMouseButton_Middle,ImGuiMouseButton_Middle+1,ImGuiMouseButton_Middle+2};

    auto &io = ImGui::GetIO();
    bool pressed = (state == WL_POINTER_BUTTON_STATE_PRESSED);
    io.AddMouseButtonEvent(mappedButtons[btnIndex], pressed);

}

static void HandlePointerAxis(void* data, wl_pointer* pointer, uint32_t time,
                             uint32_t axis, wl_fixed_t value) {
    // Handle mouse wheel
    // if (axis == WL_POINTER_AXIS_VERTICAL_SCROLL) {
    //     float scrollY = wl_fixed_to_double(value);
    //     io.AddMouseWheelEvent(0, scrollY);
    // }
}

// Add these stub functions for missing pointer events
static void HandlePointerFrame(void* data, wl_pointer* pointer) {
    // Marks the end of a pointer event sequence
}

static void HandlePointerAxisSource(void* data, wl_pointer* pointer, uint32_t axis_source) {
    // Indicates the source of axis events (e.g., wheel, finger, continuous)
}

static void HandlePointerAxisStop(void* data, wl_pointer* pointer, uint32_t time, uint32_t axis) {
    // Signals the end of an axis motion
}

static void HandlePointerAxisDiscrete(void* data, wl_pointer* pointer, uint32_t axis, int32_t discrete) {
   
    auto &io = ImGui::GetIO();
    io.AddMouseWheelEvent(0, discrete * -1.0f);
}

// Update your pointer listener to include ALL required events
const wl_pointer_listener sPointerListener = {
    HandlePointerEnter,
    HandlePointerLeave,
    HandlePointerMotion,
    HandlePointerButton,
    HandlePointerAxis,
    HandlePointerFrame,           // Opcode 5 - REQUIRED
    HandlePointerAxisSource,      // Since version 5
    HandlePointerAxisStop,        // Since version 5
    HandlePointerAxisDiscrete     // Since version 5 (deprecated in v8)
};

static ImGuiKey XkbKeysymToImGuiKey(xkb_keysym_t keysym) {
    switch (keysym) {
        case XKB_KEY_Tab: return ImGuiKey_Tab;
        case XKB_KEY_Left: return ImGuiKey_LeftArrow;
        case XKB_KEY_Right: return ImGuiKey_RightArrow;
        case XKB_KEY_Up: return ImGuiKey_UpArrow;
        case XKB_KEY_Down: return ImGuiKey_DownArrow;
        case XKB_KEY_Page_Up: return ImGuiKey_PageUp;
        case XKB_KEY_Page_Down: return ImGuiKey_PageDown;
        case XKB_KEY_Home: return ImGuiKey_Home;
        case XKB_KEY_End: return ImGuiKey_End;
        case XKB_KEY_Insert: return ImGuiKey_Insert;
        case XKB_KEY_Delete: return ImGuiKey_Delete;
        case XKB_KEY_BackSpace: return ImGuiKey_Backspace;
        case XKB_KEY_space: return ImGuiKey_Space;
        case XKB_KEY_Return: return ImGuiKey_Enter;
        case XKB_KEY_Escape: return ImGuiKey_Escape;
        case XKB_KEY_apostrophe: return ImGuiKey_Apostrophe;
        case XKB_KEY_comma: return ImGuiKey_Comma;
        case XKB_KEY_minus: return ImGuiKey_Minus;
        case XKB_KEY_period: return ImGuiKey_Period;
        case XKB_KEY_slash: return ImGuiKey_Slash;
        case XKB_KEY_semicolon: return ImGuiKey_Semicolon;
        case XKB_KEY_equal: return ImGuiKey_Equal;
        case XKB_KEY_bracketleft: return ImGuiKey_LeftBracket;
        case XKB_KEY_backslash: return ImGuiKey_Backslash;
        case XKB_KEY_bracketright: return ImGuiKey_RightBracket;
        case XKB_KEY_grave: return ImGuiKey_GraveAccent;
        case XKB_KEY_Caps_Lock: return ImGuiKey_CapsLock;
        case XKB_KEY_Scroll_Lock: return ImGuiKey_ScrollLock;
        case XKB_KEY_Num_Lock: return ImGuiKey_NumLock;
        case XKB_KEY_Print: return ImGuiKey_PrintScreen;
        case XKB_KEY_Pause: return ImGuiKey_Pause;
        case XKB_KEY_KP_Enter: return ImGuiKey_KeypadEnter;
        case XKB_KEY_KP_Equal: return ImGuiKey_KeypadEqual;
        case XKB_KEY_KP_Multiply: return ImGuiKey_KeypadMultiply;
        case XKB_KEY_KP_Add: return ImGuiKey_KeypadAdd;
        case XKB_KEY_KP_Subtract: return ImGuiKey_KeypadSubtract;
        case XKB_KEY_KP_Decimal: return ImGuiKey_KeypadDecimal;
        case XKB_KEY_KP_Divide: return ImGuiKey_KeypadDivide;
        case XKB_KEY_KP_0: return ImGuiKey_Keypad0;
        case XKB_KEY_KP_1: return ImGuiKey_Keypad1;
        case XKB_KEY_KP_2: return ImGuiKey_Keypad2;
        case XKB_KEY_KP_3: return ImGuiKey_Keypad3;
        case XKB_KEY_KP_4: return ImGuiKey_Keypad4;
        case XKB_KEY_KP_5: return ImGuiKey_Keypad5;
        case XKB_KEY_KP_6: return ImGuiKey_Keypad6;
        case XKB_KEY_KP_7: return ImGuiKey_Keypad7;
        case XKB_KEY_KP_8: return ImGuiKey_Keypad8;
        case XKB_KEY_KP_9: return ImGuiKey_Keypad9;
        case XKB_KEY_0: return ImGuiKey_0;
        case XKB_KEY_1: return ImGuiKey_1;
        case XKB_KEY_2: return ImGuiKey_2;
        case XKB_KEY_3: return ImGuiKey_3;
        case XKB_KEY_4: return ImGuiKey_4;
        case XKB_KEY_5: return ImGuiKey_5;
        case XKB_KEY_6: return ImGuiKey_6;
        case XKB_KEY_7: return ImGuiKey_7;
        case XKB_KEY_8: return ImGuiKey_8;
        case XKB_KEY_9: return ImGuiKey_9;
        case XKB_KEY_a: return ImGuiKey_A;
        case XKB_KEY_b: return ImGuiKey_B;
        case XKB_KEY_c: return ImGuiKey_C;
        case XKB_KEY_d: return ImGuiKey_D;
        case XKB_KEY_e: return ImGuiKey_E;
        case XKB_KEY_f: return ImGuiKey_F;
        case XKB_KEY_g: return ImGuiKey_G;
        case XKB_KEY_h: return ImGuiKey_H;
        case XKB_KEY_i: return ImGuiKey_I;
        case XKB_KEY_j: return ImGuiKey_J;
        case XKB_KEY_k: return ImGuiKey_K;
        case XKB_KEY_l: return ImGuiKey_L;
        case XKB_KEY_m: return ImGuiKey_M;
        case XKB_KEY_n: return ImGuiKey_N;
        case XKB_KEY_o: return ImGuiKey_O;
        case XKB_KEY_p: return ImGuiKey_P;
        case XKB_KEY_q: return ImGuiKey_Q;
        case XKB_KEY_r: return ImGuiKey_R;
        case XKB_KEY_s: return ImGuiKey_S;
        case XKB_KEY_t: return ImGuiKey_T;
        case XKB_KEY_u: return ImGuiKey_U;
        case XKB_KEY_v: return ImGuiKey_V;
        case XKB_KEY_w: return ImGuiKey_W;
        case XKB_KEY_x: return ImGuiKey_X;
        case XKB_KEY_y: return ImGuiKey_Y;
        case XKB_KEY_z: return ImGuiKey_Z;
        case XKB_KEY_F1: return ImGuiKey_F1;
        case XKB_KEY_F2: return ImGuiKey_F2;
        case XKB_KEY_F3: return ImGuiKey_F3;
        case XKB_KEY_F4: return ImGuiKey_F4;
        case XKB_KEY_F5: return ImGuiKey_F5;
        case XKB_KEY_F6: return ImGuiKey_F6;
        case XKB_KEY_F7: return ImGuiKey_F7;
        case XKB_KEY_F8: return ImGuiKey_F8;
        case XKB_KEY_F9: return ImGuiKey_F9;
        case XKB_KEY_F10: return ImGuiKey_F10;
        case XKB_KEY_F11: return ImGuiKey_F11;
        case XKB_KEY_F12: return ImGuiKey_F12;
        default: return ImGuiKey_None;
    }
}

static void HandleKeyboardKeymap(void* data, wl_keyboard* keyboard, uint32_t format,
                                int fd, uint32_t size) {
    if (format != WL_KEYBOARD_KEYMAP_FORMAT_XKB_V1) {
        close(fd);
        return;
    }

    char* map_str = static_cast<char*>(mmap(nullptr, size, PROT_READ, MAP_PRIVATE, fd, 0));
    if (map_str == MAP_FAILED) {
        close(fd);
        return;
    }

    xkb_context = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
    xkb_keymap = xkb_keymap_new_from_string(xkb_context, map_str, 
                                           XKB_KEYMAP_FORMAT_TEXT_V1,
                                           XKB_KEYMAP_COMPILE_NO_FLAGS);
    munmap(map_str, size);
    close(fd);

    xkb_state = xkb_state_new(xkb_keymap);
    
    // Get modifier masks
    shift_mask = 1 << xkb_keymap_mod_get_index(xkb_keymap, XKB_MOD_NAME_SHIFT);
    ctrl_mask = 1 << xkb_keymap_mod_get_index(xkb_keymap, XKB_MOD_NAME_CTRL);
    alt_mask = 1 << xkb_keymap_mod_get_index(xkb_keymap, XKB_MOD_NAME_ALT);
    super_mask = 1 << xkb_keymap_mod_get_index(xkb_keymap, XKB_MOD_NAME_LOGO);
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
    ImGuiIO& io = ImGui::GetIO();
    bool pressed = (state == WL_KEYBOARD_KEY_STATE_PRESSED);
    
    // Convert Linux keycode to XKB keycode
    uint32_t keycode = key + 8;
    
    // Update XKB state
    xkb_state_update_key(xkb_state, keycode, 
                        pressed ? XKB_KEY_DOWN : XKB_KEY_UP);
    
    // Get keysym and ImGui key
    xkb_keysym_t keysym = xkb_state_key_get_one_sym(xkb_state, keycode);
    ImGuiKey imgui_key = XkbKeysymToImGuiKey(keysym);
    
    if (imgui_key != ImGuiKey_None) {
        io.AddKeyEvent(imgui_key, pressed);
    }
    
    // Handle text input (only on press events)
    if (pressed) {
        char buf[128];
        int size = xkb_state_key_get_utf8(xkb_state, keycode, buf, sizeof(buf));
        if (size > 0) {
            io.AddInputCharactersUTF8(buf);
        }
    }
}

static void HandleKeyboardModifiers(void* data, wl_keyboard* keyboard, uint32_t serial,
                                   uint32_t mods_depressed, uint32_t mods_latched,
                                   uint32_t mods_locked, uint32_t group) {
    xkb_state_update_mask(xkb_state, mods_depressed, mods_latched, 
                         mods_locked, 0, 0, group);
    
    ImGuiIO& io = ImGui::GetIO();
    io.AddKeyEvent(ImGuiKey_ModShift,   xkb_state_mod_index_is_active(xkb_state, 
                        xkb_keymap_mod_get_index(xkb_keymap, XKB_MOD_NAME_SHIFT), 
                        XKB_STATE_MODS_EFFECTIVE) > 0);
    io.AddKeyEvent(ImGuiKey_ModCtrl,    xkb_state_mod_index_is_active(xkb_state, 
                        xkb_keymap_mod_get_index(xkb_keymap, XKB_MOD_NAME_CTRL), 
                        XKB_STATE_MODS_EFFECTIVE) > 0);
    io.AddKeyEvent(ImGuiKey_ModAlt,     xkb_state_mod_index_is_active(xkb_state, 
                        xkb_keymap_mod_get_index(xkb_keymap, XKB_MOD_NAME_ALT), 
                        XKB_STATE_MODS_EFFECTIVE) > 0);
    io.AddKeyEvent(ImGuiKey_ModSuper,   xkb_state_mod_index_is_active(xkb_state, 
                        xkb_keymap_mod_get_index(xkb_keymap, XKB_MOD_NAME_LOGO), 
                        XKB_STATE_MODS_EFFECTIVE) > 0);
}

static void HandleKeyboardRepeatInfo(void* data, wl_keyboard* keyboard, 
                                   int32_t rate, int32_t delay) {
    // Handle key repeat information (required for protocol version 4+)
}

const wl_keyboard_listener sKeyboardListener = {
    HandleKeyboardKeymap,
    HandleKeyboardEnter,
    HandleKeyboardLeave,
    HandleKeyboardKey,
    HandleKeyboardModifiers,
    HandleKeyboardRepeatInfo
};

static void HandleSeatCapabilities(void* data, wl_seat* seat, uint32_t capabilities) {
    // Check for pointer capability
    if (capabilities & WL_SEAT_CAPABILITY_POINTER) {
        if (!gPointer) {
            gPointer = wl_seat_get_pointer(seat);
            wl_pointer_add_listener(gPointer, &sPointerListener, nullptr);
        }
    } else {
        if (gPointer) {
            wl_pointer_destroy(gPointer);
            gPointer = nullptr;
        }
    }
    // Similarly for keyboard
    if (capabilities & WL_SEAT_CAPABILITY_KEYBOARD) {
        if (!gKeyboard) {
            gKeyboard = wl_seat_get_keyboard(seat);
            wl_keyboard_add_listener(gKeyboard, &sKeyboardListener, nullptr);
        }
    } else {
        if (gKeyboard) {
            wl_keyboard_destroy(gKeyboard);
            gKeyboard = nullptr;
        }
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

    wl_display_roundtrip(gDisplay); 
    return true;
}

static void SetInputRegionToWholeWindow() {
    wl_compositor* compositor = gCompositor; // Your global compositor
    wl_surface* surface = gSurface; // Your global surface
    
    // Create a region that covers the entire window
    wl_region* region = wl_compositor_create_region(compositor);
    wl_region_add(region, 0, 0, gWidth, gHeight);
    
    // Set this region as the input region for your surface
    wl_surface_set_input_region(surface, region);
    
    // Destroy the region as it's been copied by the compositor
    wl_region_destroy(region);
}


static void ConfigureToplevelDecoration()
{
    if (!gDecorationManager || !gXdgToplevel) {
        std::cout << "Decoration manager or toplevel not available, using client-side decorations" << std::endl;
        return;
    }
    
    // Create a decoration object for the toplevel
    gToplevelDecoration = zxdg_decoration_manager_v1_get_toplevel_decoration(
        gDecorationManager, gXdgToplevel);
    
    if (!gToplevelDecoration) {
        std::cerr << "Failed to create toplevel decoration" << std::endl;
        return;
    }
    
    // Request server-side decorations
    zxdg_toplevel_decoration_v1_set_mode(gToplevelDecoration, 
        ZXDG_TOPLEVEL_DECORATION_V1_MODE_SERVER_SIDE);
    
}

static void HandleDecorationConfigure(void* data, 
                                    zxdg_toplevel_decoration_v1* decoration, 
                                    uint32_t mode)
{
    switch (mode) {
        case ZXDG_TOPLEVEL_DECORATION_V1_MODE_CLIENT_SIDE:
            std::cout << "Compositor enforced client-side decorations" << std::endl;
            break;
        case ZXDG_TOPLEVEL_DECORATION_V1_MODE_SERVER_SIDE:
            std::cout << "Using server-side decorations" << std::endl;
            break;
        default:
            std::cout << "Unknown decoration mode: " << mode << std::endl;
    }
}

static const zxdg_toplevel_decoration_v1_listener sDecorationListener = {
    HandleDecorationConfigure
};


void WaylandDisplayServer::CreateWindow(const mercury::ll::os::OS::NativeWindowDescriptor &desc)
{
    gWidth = desc.width;
    gHeight = desc.height;
    
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
    
    SetInputRegionToWholeWindow();

    xdg_surface_add_listener(gXdgSurface, &sXdgSurfaceListener, nullptr);

    // Create XDG toplevel
    gXdgToplevel = xdg_surface_get_toplevel(gXdgSurface);
    if (!gXdgToplevel) {
        std::cerr << "Failed to create xdg toplevel." << std::endl;
        return;
    }
    
    xdg_toplevel_add_listener(gXdgToplevel, &sXdgToplevelListener, nullptr);
    xdg_toplevel_set_title(gXdgToplevel, (const char*)desc.title.c_str());
 
    const auto& cfg = mercury::Application::GetCurrentApplication()->GetConfig();

    xdg_toplevel_set_app_id(gXdgToplevel, (const char*)cfg.appID);
    // Set window size (this is a hint, the compositor may ignore it)
    xdg_toplevel_set_min_size(gXdgToplevel, 16, 16);
    xdg_toplevel_set_max_size(gXdgToplevel, 4096, 4096);
    //xdg_toplevel_set_fullscreen(gXdgToplevel, nullptr);
    //xdg_toplevel_set_minimized(gXdgToplevel);
    //xdg_toplevel_show_window_menu(gXdgToplevel,gSeat,0,100,200);
    xdg_toplevel_set_maximized(gXdgToplevel);
    //xdg_toplevel_resize
    // Commit the surface to make the window visible
    wl_surface_commit(gSurface);
    
    std::cout << "Window created with size " << desc.width << "x" << desc.height << std::endl;

    // Wait for the initial configure event
    while (!gConfigured && running) {
        wl_display_dispatch(gDisplay);
    }
    
    //wl_seat_add_listener(gSeat, &sSeatListener, nullptr);
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
    printf("has wayland interface: %s\n",interface);
    
    if (std::strcmp(interface, "wl_compositor") == 0) {
        gCompositor = static_cast<wl_compositor*>(
            wl_registry_bind(registry, name, &wl_compositor_interface, 3));
    } else if (std::strcmp(interface, "xdg_wm_base") == 0) {
        gXdgWmBase = static_cast<xdg_wm_base*>(
            wl_registry_bind(registry, name, &xdg_wm_base_interface, 1));
    } else if (std::strcmp(interface, "wl_seat") == 0) {
        gSeat = static_cast<wl_seat*>(
            wl_registry_bind(registry, name, &wl_seat_interface, 7));
        wl_seat_add_listener(gSeat, &sSeatListener, nullptr);
    } else if (std::strcmp(interface, "zxdg_decoration_manager_v1") == 0) {
        // Bind the decoration manager
        gDecorationManager = static_cast<zxdg_decoration_manager_v1*>(
            wl_registry_bind(registry, name, &zxdg_decoration_manager_v1_interface, 1));
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
        gWidth = new_width;
        gHeight = new_height;
        std::cout << "Window configured to size " << gWidth << "x" << gHeight << std::endl;

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
        auto& io = ImGui::GetIO();
        io.DisplaySize.x = (float)gWidth;
        io.DisplaySize.y = (float)gHeight;

        ImGui_ImplWayland_NewFrame();
    }

    void WaylandDisplayServer::ImguiShutdown()
    {
        ImGui_ImplWayland_Shutdown();
    }
#endif