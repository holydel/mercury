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

// Global objects for Wayland protocols
static wl_display* gDisplay = nullptr;
static wl_compositor* gCompositor = nullptr;
static wl_surface* gSurface = nullptr;
static xdg_wm_base* gXdgWmBase = nullptr;
static xdg_surface* gXdgSurface = nullptr;
static xdg_toplevel* gXdgToplevel = nullptr;
static wl_buffer* gBuffer = nullptr;
static wl_shm* gShm = nullptr;
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
    if (gShm) wl_shm_destroy(gShm);
    if (gDisplay) wl_display_disconnect(gDisplay);
    
    gDisplay = nullptr;
    gCompositor = nullptr;
    gSurface = nullptr;
    gXdgWmBase = nullptr;
    gXdgSurface = nullptr;
    gXdgToplevel = nullptr;
    gShm = nullptr;
}

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
    
    if (!gShm) {
        std::cerr << "No shared memory available." << std::endl;
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
    
    // Now create the buffer with the configured size
    //gBuffer = CreateBuffer(width, height);
    //if (!gBuffer) {
    //    std::cerr << "Failed to create buffer." << std::endl;
    //    return;
    //}
    
    //wl_surface_attach(gSurface, gBuffer, 0, 0);
    //wl_surface_commit(gSurface);
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
    
    if (gBuffer) {
        wl_buffer_destroy(gBuffer);
        gBuffer = nullptr;
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
    } else if (std::strcmp(interface, "wl_shm") == 0) {
        gShm = static_cast<wl_shm*>(
            wl_registry_bind(registry, name, &wl_shm_interface, 1));
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

// Buffer creation helper
wl_buffer* CreateBuffer(int width, int height)
{
    int stride = width * 4;
    int size = stride * height;
    
    // Create a shared memory file descriptor
    std::string name = "/wayland-shm-" + std::to_string(rand());
    int fd = memfd_create(name.c_str(), MFD_CLOEXEC | MFD_ALLOW_SEALING);
    if (fd < 0) {
        fd = open("/tmp", O_TMPFILE | O_RDWR | O_EXCL, S_IRUSR | S_IWUSR);
        if (fd < 0) {
            std::cerr << "Failed to create shared memory file" << std::endl;
            return nullptr;
        }
    }
    
    // Set size
    if (ftruncate(fd, size) < 0) {
        std::cerr << "Failed to set shared memory size" << std::endl;
        close(fd);
        return nullptr;
    }
    
    // Map memory
    uint32_t* data = static_cast<uint32_t*>(mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0));
    if (data == MAP_FAILED) {
        std::cerr << "Failed to map shared memory" << std::endl;
        close(fd);
        return nullptr;
    }
    
    // Fill with a pattern (red gradient for visibility)
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            data[y * width + x] = (0xFF << 24) | ((x * 255 / width) << 16) | (y * 255 / height << 8);
        }
    }
    
    munmap(data, size);
    
    // Create wl_buffer from the shared memory
    wl_shm_pool* pool = wl_shm_create_pool(gShm, fd, size);
    wl_buffer* buffer = wl_shm_pool_create_buffer(pool, 0, width, height, stride, WL_SHM_FORMAT_XRGB8888);
    wl_shm_pool_destroy(pool);
    close(fd);
    
    return buffer;
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
#endif