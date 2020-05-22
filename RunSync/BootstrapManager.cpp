#include "SystemUtils.h"
#include "BootstrapManager.h"

using namespace RunSync;


bool BootstrapManager::SendCmdToRunningInstance(const wstring& cmd)
{
    auto pid = SystemUtils::GetRunningInstancePID();
    if (pid > 0)
    {        
        HWND h = SystemUtils::GetMainWindow(pid);
        if (h > 0)
        {
            COPYDATASTRUCT data;

            data.dwData = 0;
            data.cbData = (DWORD)((cmd.length() + 1) * sizeof(WCHAR));
            data.lpData = (PVOID)cmd.c_str();

            SendMessage(h, WM_COPYDATA,
                (WPARAM)(HWND)0,
                (LPARAM)(LPVOID)&data);
        }
        
    }

    return pid > 0;
}



