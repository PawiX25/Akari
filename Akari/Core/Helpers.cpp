
#include "Helpers.h"
#include "Configuration.h"
#include "Input.h"
#include "Exports/vsh/vshcommon.h"
#include "Exports/vsh/vshmain.h"
#include "../Utilities/System.h"
#include <vsh/stdc.h>
#include <vsh/interfaces/system_plugin.h>

// Constants for magic numbers
const unsigned int SCREENSHOT_INTERVAL = 500;
const unsigned int GAME_LAUNCH_DETECTION_INTERVAL = 15 * 1000;

// Constants for hardcoded strings
const std::string SCREENSHOTS_DIRECTORY = "Screenshots";
const std::wstring SCREENSHOT_SUCCESS_MESSAGE = L"Screenshot saved as: %s";
const std::wstring SCREENSHOT_UNSUPPORTED_MESSAGE = L"Screenshots aren't supported in game";

// Array for button codes
const Input::Buttons SCREENSHOT_BUTTON_BINDS[] = {
    Input::BUTTON_PAD_UP,
    Input::BUTTON_PAD_DOWN,
    Input::BUTTON_PAD_LEFT,
    Input::BUTTON_PAD_RIGHT
};

Helpers g_Helpers;

Helpers::Helpers()
{
    m_IsHen = Syscall::sys_mapi_is_hen();
}

// This function updates the state of the game and takes a screenshot if the binds are enabled
void Helpers::OnUpdate()
{
    // Find the plugins
    game_ext_plugin = paf::View::Find("game_ext_plugin");
    game_plugin = paf::View::Find("game_plugin");
    system_plugin = paf::View::Find("system_plugin");
    xmb_plugin = paf::View::Find("xmb_plugin");

    // Find the auto-off guide widget
    page_autooff_guide = system_plugin ? system_plugin->FindWidget("page_autooff_guide") : nullptr;

    // Monitor the game state
    MonitorGameState();

    // Take a screenshot if the binds are enabled
    if (g_Config.screenshots.enableBinds && ScreenshotsBinds())
        TakeScreenshot();
}

// This function monitors the state of the game
void Helpers::MonitorGameState()
{
    unsigned int timeNow = Timers::GetTimeNow();

    if (vsh::GetCooperationMode() != vsh::CooperationMode::XMB && !m_StateGameRunning)
    {
        m_StateGameRunning = true;
        m_StateGameJustLaunched = true;
        m_GameLaunchTime = timeNow;
    }
    else if (vsh::GetCooperationMode() == vsh::CooperationMode::XMB)
        m_StateGameRunning = false;

    if (timeNow - m_GameLaunchTime > GAME_LAUNCH_DETECTION_INTERVAL)
        m_StateGameJustLaunched = false;
}

// This function takes a screenshot
void Helpers::TakeScreenshot()
{
    unsigned int timeNow = Timers::GetTimeNow();

    if ((timeNow - m_ScreenshotLastTime) < SCREENSHOT_INTERVAL)
        return;

    m_ScreenshotLastTime = timeNow;

    if (vsh::GetCooperationMode() == vsh::CooperationMode::XMB)
    {
        if (!system_plugin)
            return;

        system_plugin_interface* system_interface = system_plugin->GetInterface<system_plugin_interface*>(1);
        if (!system_interface)
            return;

        std::string screenshotPath = File::GetCurrentDir() + SCREENSHOTS_DIRECTORY;
        stdc::mkdir(screenshotPath.c_str(), 40777);

        time_t rawtime;
        stdc::time(&rawtime);
        tm* timeinfo = stdc::localtime(&rawtime);

        char datestr[100];
        stdc::strftime(datestr, 100, "%Y-%m-%d_%H-%M-%S", timeinfo);

        std::string screenshotName = screenshotPath + "/" + datestr + ".bmp";

        system_interface->saveBMP(screenshotName.c_str());

        wchar_t buffer[130];
        stdc::swprintf(buffer, 0xA0, SCREENSHOT_SUCCESS_MESSAGE.c_str(), screenshotName.c_str());
        vsh::ShowButtonNavigationMessage(buffer);
    }
    else
        vsh::ShowButtonNavigationMessage(SCREENSHOT_UNSUPPORTED_MESSAGE);
}

// This function checks if the screenshot binds are pressed
bool Helpers::ScreenshotsBinds()
{
    if (g_Config.screenshots.binds < 0 || g_Config.screenshots.binds >= sizeof(SCREENSHOT_BUTTON_BINDS) / sizeof(SCREENSHOT_BUTTON_BINDS[0]))
        return false;

    return g_Input.IsButtonBinds(Input::BUTTON_L1, SCREENSHOT_BUTTON_BINDS[g_Config.screenshots.binds]);
}