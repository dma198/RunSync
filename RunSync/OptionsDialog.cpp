#include "framework.h"

#include "Resource.h"

#include "Hash.h"
#include "MainWindow.h"
#include "OptionsDialog.h"

using namespace RunSync;

wstring OptionsDialog::errMsg = L"";
HINSTANCE OptionsDialog::hInst;
HWND OptionsDialog::mainWindowHandle;
HWND hWndOptionsDialog;

void OptionsDialog::Init(HINSTANCE hInst, HWND mainWnd)
{
    OptionsDialog::hInst = hInst;
    OptionsDialog::mainWindowHandle = mainWnd;
}

INT_PTR OptionsDialog::Show()
{
    INT_PTR res = DialogBox(hInst, MAKEINTRESOURCE(IDD_OPTIONS), MainWindow::getHandle(), OptionsDlgProc);
    return res;
}

bool OptionsDialog::IsActive()
{
    return hWndOptionsDialog>0;
}

void OptionsDialog::Update()
{
    if (hWndOptionsDialog > 0)
        SendMessage(hWndOptionsDialog, WM_RunSync_OPTIONS_UPDATE, 0, 0);
}


// Private members

void OptionsDialog::LoadData(HWND hwnd)
{
    WCHAR buf[MAX_PATH];
    GetDlgItemText(hwnd, IDC_SRCFOLDER, buf, MAX_PATH);
    if (wstring(buf) == L"")
    {
        SetDlgItemText(hwnd, IDC_SRCFOLDER, AppConfig::getSrc().c_str());
        SetDlgItemText(hwnd, IDC_DSTFOLDER, AppConfig::getDst().c_str());
        SetDlgItemText(hwnd, IDC_USER, AppConfig::getSrcUser().c_str());
        SetDlgItemText(hwnd, IDC_PASSWD, L"");
    }

    auto hAppsList = GetDlgItem(hwnd, IDC_APPSLIST);

    RECT r;
    GetWindowRect(hAppsList, &r);

    auto cnt = SendMessage(hAppsList, LVM_GETITEMCOUNT, 0, 0);
    if (cnt == 0)
    {
        LV_COLUMNW cl;
        memset(&cl, 0, sizeof(cl));                      // Zero Members
        cl.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM; // Type of mask
        cl.cx = 0x28;                                    // width between each coloum
        cl.pszText = (LPWSTR)L"Application";             // First Header Text
        cl.cx = (r.right-r.left) / 3;                    // width of column

        SendMessage(hAppsList, LVM_SETEXTENDEDLISTVIEWSTYLE,
            0, LVS_EX_FULLROWSELECT);

        SendMessage(hAppsList, LVM_INSERTCOLUMN, 0, (LPARAM)&cl);
        cl.pszText = (LPWSTR)L"Running Status";                   
        SendMessage(hAppsList, LVM_INSERTCOLUMN, 1, (LPARAM)&cl);
        cl.pszText = (LPWSTR)L"Version Status";
        SendMessage(hAppsList, LVM_INSERTCOLUMN, 2, (LPARAM)&cl);
    }
   
    SendMessage(hAppsList, LVM_DELETEALLITEMS, 0, 0);

    for (auto it : AppManager::getApps())
    {

        LV_ITEMW lvItem;

        lvItem.mask = LVIF_TEXT;    // Text Style
        lvItem.cchTextMax = 256;    // Max size of test
        lvItem.iItem = 0;           // choose item  
        lvItem.iSubItem = 0;        // Put in first coluom
        lvItem.pszText = (LPWSTR)it.AppName.c_str(); // Text to display (can be from a char variable) (Items)
        SendMessage(hAppsList, LVM_INSERTITEM, 0, (LPARAM)&lvItem);
        
        lvItem.iSubItem = 1;
        switch (it.RunStatus)
        {
            case AppRunStatuses::rstNotInstalled : lvItem.pszText = (LPWSTR)L"NOT INSTALLED"; break;
            case AppRunStatuses::rstInstalling : lvItem.pszText = (LPWSTR)L"INSTALLING"; break;
            case AppRunStatuses::rstUpgrading: lvItem.pszText = (LPWSTR)L"UPGRADING"; break;
            case AppRunStatuses::rstRunOpened : lvItem.pszText = (LPWSTR)L"OPENED"; break;
            case AppRunStatuses::rstRunMinimized: lvItem.pszText = (LPWSTR)L"MINIMIZED"; break;
            case AppRunStatuses::rstRunMaximized: lvItem.pszText = (LPWSTR)L"MAXIMIZED"; break;
            default : lvItem.pszText = (LPWSTR)L"STOPPED";
        }
        SendMessage(hAppsList, LVM_SETITEM, 0, (LPARAM)&lvItem);

        lvItem.iSubItem = 2;
        switch (it.VersionStatus)
        {
            case AppVersionStatuses::vstUpToDate: lvItem.pszText = (LPWSTR)L"UP TO DATE"; break;
            case AppVersionStatuses::vstObsolete: lvItem.pszText = (LPWSTR)L"OBSOLETE"; break;
            case AppVersionStatuses::vstUpgradeError: lvItem.pszText = (LPWSTR)L"UPGRADE ERROR"; break;
            default: lvItem.pszText = (LPWSTR)L"UNDEFINED";
        }
        SendMessage(hAppsList, LVM_SETITEM, 0, (LPARAM)&lvItem);

    }

    SetDlgItemText(hwnd, IDC_ERRORMSG, errMsg.c_str());
    auto hErr = GetDlgItem(hwnd, IDC_ERRORMSG);
    GetClientRect(hErr, &r);
    InvalidateRect(hErr, &r, TRUE);
    MapWindowPoints(hErr, hwnd, (POINT*)&r, 2);
    RedrawWindow(hwnd, &r, NULL, RDW_ERASE | RDW_INVALIDATE);
}


INT_PTR OptionsDialog::OptionsDlgProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
    switch (Message)
    {
    case WM_INITDIALOG:
    {
        hWndOptionsDialog = hwnd;
              
        auto rDsk = AppConfig::getDesktopRect();
        RECT rect;
        GetWindowRect(hwnd, &rect);
        SetWindowPos(hwnd, 0,
            rDsk.left + ((rDsk.right - rDsk.left) / 2) - ((rect.right - rect.left) / 2),
            rDsk.top + ((rDsk.bottom - rDsk.top) / 2) - ((rect.bottom - rect.top) / 2),
            rect.right - rect.left, rect.bottom - rect.top, 0);

        auto hAppsList = GetDlgItem(hwnd, IDC_APPSLIST);
        auto hRes = SetWindowTheme(hAppsList, L"Explorer", NULL);

        LoadData(hwnd);
          
        return TRUE;
    }
    break;

    case WM_DESTROY:
        EndDialog(hwnd, 0);
        hWndOptionsDialog = 0;
        break;

    case WM_SHOWWINDOW :
        if(wParam)
            AnimateWindow(hwnd,300, AW_CENTER);
        break;

    case WM_CTLCOLORDLG:
        return (INT_PTR)GetStockObject(WHITE_BRUSH);

    case WM_CTLCOLORSTATIC:
    {
        SetBkMode((HDC)wParam, TRANSPARENT);
        if ((HWND)lParam == GetDlgItem(hwnd, IDC_ERRORMSG))
        {
            SetTextColor((HDC)wParam, RGB(220, 10, 10));
        }
        else
        {
            SetTextColor((HDC)wParam, RGB(0, 0, 0));
        }
        return (INT_PTR)GetStockObject(HOLLOW_BRUSH);
    }
    break;

    case WM_RunSync_OPTIONS_UPDATE:
        LoadData(hwnd);
        break;

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
            case IDOK:
            {
                WCHAR tmpsrc[MAX_PATH];

                GetDlgItemText(hwnd, IDC_SRCFOLDER, tmpsrc, MAX_PATH);
                if (lstrlen(tmpsrc) == 0)
                {
                    MessageBox(hwnd, L"Not defined source folder!", L"Error", MB_ICONEXCLAMATION);
                    return false;
                }
                CharLower(tmpsrc);

                DWORD fa = GetFileAttributes(tmpsrc);
                if (fa == INVALID_FILE_ATTRIBUTES || (fa & FILE_ATTRIBUTE_DIRECTORY) == 0)
                {
                    WCHAR s[1024];
                    wsprintf(s, L"Can not access to source applications folder %s", tmpsrc);
                    MessageBox(hwnd, s, L"Error", MB_ICONEXCLAMATION);
                    return false;
                }

                AppConfig::setSrc(tmpsrc);

                GetDlgItemText(hwnd, IDC_USER, tmpsrc, MAX_PATH);
                AppConfig::setSrcUser(tmpsrc);

                GetDlgItemText(hwnd, IDC_PASSWD, tmpsrc, MAX_PATH);
                if (wstring(tmpsrc) != L"")
                {
                    auto encryptedPasswd = Hash::Encrypt(tmpsrc);
                    AppConfig::setSrcPassword(encryptedPasswd);
                }

                WCHAR tmpdst[MAX_PATH];

                GetDlgItemText(hwnd, IDC_DSTFOLDER, tmpdst, MAX_PATH);
                if (lstrlen(tmpdst) == 0)
                {
                    MessageBox(hwnd, L"Not defined destination folder!", L"Error", MB_ICONEXCLAMATION);
                    return false;
                }
                CharLower(tmpdst);

                if (lstrcmp(tmpsrc, tmpdst) == 0)
                {
                    MessageBox(hwnd, L"Sourse and destination folders can not be the same!", L"Error", MB_ICONEXCLAMATION);
                    return false;
                }


                fa = GetFileAttributes(tmpdst);
                if (fa == INVALID_FILE_ATTRIBUTES || (fa & FILE_ATTRIBUTE_DIRECTORY) == 0)
                {
                    if (SHCreateDirectoryEx(hwnd, tmpdst, NULL) != ERROR_SUCCESS)
                    {
                        WCHAR s[1024];
                        wsprintf(s, L"Can not access to destination applications folder %s", tmpdst);
                        MessageBox(hwnd, s, L"Error", MB_ICONEXCLAMATION);
                        return false;
                    }
                }

                WCHAR tstFile[1024];
                wsprintf(tstFile, L"%s\\%s", tmpdst, L"$tst$");
                HANDLE tstF = CreateFile(tstFile, GENERIC_WRITE, 0,
                    nullptr, CREATE_ALWAYS,
                    FILE_ATTRIBUTE_NORMAL, NULL);
                if (tstF == INVALID_HANDLE_VALUE)
                {
                    WCHAR s[1024];
                    wsprintf(s, L"Can not write files to destination applications folder %s", tmpdst);
                    MessageBox(hwnd, s, L"Error", MB_ICONEXCLAMATION);
                    return false;
                };

                AppConfig::setDst(tmpdst);

                AppConfig::Save();

                CloseHandle(tstF);
                DeleteFile(tstFile);

                EndDialog(hwnd, IDOK);

            }
            break;

            case IDCANCEL:
                EndDialog(hwnd, IDCANCEL);
                break;

            case IDEXIT:
                if (MessageBox(hwnd, L"Do you want to stop version monitoring/synchronization?", L"Exit", MB_ICONEXCLAMATION | MB_YESNO) == IDYES)
                {
                    DestroyWindow(OptionsDialog::mainWindowHandle);
                    ExitProcess(0);
                }
                break;

            case IDABOUT:
                DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hwnd, About);
                break;

            default:
                return (INT_PTR)DefWindowProc(hwnd, Message, wParam, lParam);
        } 
        break;
    default:
        return FALSE;
    }
    return TRUE;
}

INT_PTR CALLBACK OptionsDialog::About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_CTLCOLORDLG:
        return (INT_PTR)GetStockObject(WHITE_BRUSH);
    
    case WM_CTLCOLORSTATIC:
    {
        if ((HWND)lParam != GetDlgItem(hDlg, IDILOGO))
        {
            SetBkMode((HDC)wParam, TRANSPARENT);
            return (INT_PTR)GetStockObject(HOLLOW_BRUSH);
        }
        //return (INT_PTR)GetStockObject(NULL_BRUSH);
        
    }
    break;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}
