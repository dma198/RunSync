#include <filesystem>
#include <list>

#include "framework.h"
#include "resource.h"

#include "SystemUtils.h"
#include "AppConfig.h"
#include "AppNameDialog.h"

using namespace RunSync;
using namespace std;

HWND hWndAppNameDialog;
HINSTANCE AppNameDialog::hInst;
HWND AppNameDialog::mainWindowHandle;

INT_PTR AppNameDialog::Show()
{
    auto res = DialogBox(hInst, MAKEINTRESOURCE(IDD_APPNAME), mainWindowHandle, DlgProc);
    return res;
}

void LoadAppNames(list<wstring>& ls,wstring dir)
{
    SystemUtils::LoginToNetworkShared(dir, AppConfig::getSrcUser(), AppConfig::getSrcPassword());
    for (auto& p : filesystem::directory_iterator(dir))
    {
        if (p.is_directory())
        {
            auto exeDirPath = p.path();
            auto exeName = filesystem::path(exeDirPath).filename();
            auto exePath = exeDirPath.append(exeName.string() + ".exe");
            if (filesystem::exists(exePath))
            {
                ls.push_back(exeName);
            }
        }
    }
}

INT_PTR CALLBACK AppNameDialog::DlgProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
    switch (Message)
    {
    case WM_INITDIALOG:
    {
        hWndAppNameDialog = hwnd;
        SetWindowTheme(hwnd, L"Settings", NULL);

        auto rDsk = AppConfig::getDesktopRect();
        RECT rect;
        GetWindowRect(hwnd, &rect);
        SetWindowPos(hwnd, 0,
            rDsk.left + ((rDsk.right - rDsk.left) / 2) - ((rect.right - rect.left) / 2),
            rDsk.top + ((rDsk.bottom - rDsk.top) / 2) - ((rect.bottom - rect.top) / 2),
            rect.right - rect.left, rect.bottom - rect.top, 0);

        list<wstring> appNames;
        try
        {            
            LoadAppNames(appNames, AppConfig::getSrc());
            if(appNames.size()==0)
                LoadAppNames(appNames, AppConfig::getDst());
                
            SendDlgItemMessage(hwnd, IDC_APPNAME, CB_RESETCONTENT, 0, 0);

            for (auto n : appNames)
            {
                SendDlgItemMessage(hwnd, IDC_APPNAME, CB_ADDSTRING, 0, (LPARAM)n.c_str());
            }

            if (appNames.size() > 0)
            {
                SendDlgItemMessage(hwnd, IDC_APPNAME, CB_SETCURSEL, 0, 0);
            }
            
        }
        catch (...) {}

        return TRUE;
    }
    break;

    case WM_CTLCOLORDLG:
        return (INT_PTR)GetStockObject(WHITE_BRUSH);

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
            case IDOK:       
                WCHAR buf[MAX_PATH];
                GetDlgItemText(hwnd, IDC_APPNAME, buf, MAX_PATH);
                if (buf != L"")AppConfig::setAppName(buf);
                EndDialog(hwnd, IDOK);            
                break;

            case IDCANCEL:
                EndDialog(hwnd, IDCANCEL);
                break;

            default:
                return DefWindowProc(hwnd, Message, wParam, lParam);
                break;
        }
        default:
            return FALSE;
    }
    return TRUE;
}
