
#include "Core/Configuration.h"
#include "Core/Helpers.h"
#include "Core/Hooks.h"
#include "Core/Input.h"
#include "Core/Rendering.h"
#include "Core/Version.h"
#include "Core/Menu/Base.h"
#include "Core/Menu/Submenus.h"
#include "Core/Menu/Overlay.h"
#include "Utilities/System/SystemCalls.h"
#include "Utilities/System/Timers.h"
#include <sys/prx.h>
#include <sys/ppu_thread.h>
#include <vsh/stdc.h>

SYS_MODULE_INFO(Akari, 0, BUILD_VERSION_MAJOR, BUILD_VERSION_MINOR);
SYS_MODULE_START(module_start);
SYS_MODULE_STOP(module_stop);

sys_ppu_thread_t gMainPpuThreadId = SYS_PPU_THREAD_ID_INVALID;

// Start of the module
CDECL_BEGIN
int module_start(size_t args, const void* argp)
{
    // Create a new thread for the module
    sys_ppu_thread_create(&gMainPpuThreadId, [](uint64_t arg) -> void
    {
        // Wait until the explore_plugin is found
        do Timers::Sleep(1000); while (!paf::View::Find("explore_plugin"));

        stdc::printf(BUILD_STRING "\n");

        // Initialize the global objects
        g_Input = Input();
        g_Render = Render();
        g_Helpers = Helpers();

        g_Config = Config();
        g_Menu = Menu(MainSubmenu);
        g_Overlay = Overlay();

        // Install the hooks
        InstallHooks();

        sys_ppu_thread_exit(0);
    },
    0, 1069, 1536, SYS_PPU_THREAD_CREATE_JOINABLE, "Akari::module_start");

    Syscall::_sys_ppu_thread_exit(0);
    return 0;
}

// Stop of the module
int module_stop(size_t args, const void* argp)
{
    sys_ppu_thread_t stopPpuThreadId;
    int ret = sys_ppu_thread_create(&stopPpuThreadId, [](uint64_t arg) -> void
    {
        // Remove the hooks
        RemoveHooks();

        // Handle shutdown for overlay
        g_Overlay.OnShutdown();

        // Save the configuration and destroy the widgets
        g_Config.Save();
        g_Render.DestroyWidgets();

        sys_ppu_thread_yield();
        Timers::Sleep(1337);

        // Join the main thread if it's valid
        if (gMainPpuThreadId != SYS_PPU_THREAD_ID_INVALID)
        {
            uint64_t exitCode;
            sys_ppu_thread_join(gMainPpuThreadId, &exitCode);
        }

        sys_ppu_thread_exit(0);
    },
    0, 2814, 1024, SYS_PPU_THREAD_CREATE_JOINABLE, "Akari::module_stop");

    // Join the stop thread if it's valid
    if (stopPpuThreadId != SYS_PPU_THREAD_ID_INVALID)
    {
        uint64_t exitCode;
        sys_ppu_thread_join(stopPpuThreadId, &exitCode);
    }

    // Exit the current thread
    Syscall::_sys_ppu_thread_exit(0);
    return 0;
}
CDECL_END