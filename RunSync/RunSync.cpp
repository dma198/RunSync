// DMA198
// Application Entry Point
//

#include "framework.h"
#include "AppConfig.h"
#include "KeyboardHook.h"
#include "BootstrapManager.h"
#include "OptionsDialog.h"
#include "AppNameDialog.h"
#include "MainWindow.h"
#include "AppManager.h"

#include "RunSync.h"

using namespace RunSync;


HINSTANCE hInst;  // Application instance

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // Load configuration 
    wstring cmd(lpCmdLine);
    AppConfig::Init(cmd);

    if (AppConfig::getDst() == L"")
    {
        OptionsDialog::Show();
    }


    if (AppConfig::getDst() == L"")
    {
        ExitProcess(0);
        return 0;
    }

    if (AppConfig::getAppName() == L"")
    {
        if (AppNameDialog::Show() != IDOK)
        {
            ExitProcess(0);
            return 0;
        }
    }

    if (AppConfig::getDst() == L"") // Not provided required options
    {
        ExitProcess(0); 
        return 0;
    }


    // Trying to send command to running RunSync instance (if found)
    if (BootstrapManager::SendCmdToRunningInstance(AppConfig::getAppName()))
    {
        ExitProcess(0);
        return 0; 
    }

    // Add application info into list of apps to be managed 
    // AppManager::addApp(AppConfig::getAppName());

    // Initialize Main Window
    if (!MainWindow::Init(hInstance))
    {
        return FALSE;
    }

    // Set global keyboard hook 
    KeyboardHook::Init();

    AppManager::Init();

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_RUNSYNC));
    MSG msg;

    // Main application message loop:
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    // Grace shutdown
    MainWindow::Dispose();

    return (int) msg.wParam;
}


