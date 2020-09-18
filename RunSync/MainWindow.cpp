#include "resource.h"

#include "AppConfig.h"
#include "OptionsDialog.h"
#include "MainWindow.h"

using namespace Gdiplus;
using namespace RunSync;

constexpr int MAX_LOADSTRING=100;
constexpr int BOTTOM_PADDING = 5;

Color clInstallationBk = Color::DarkGreen;
Color clUpgradingBk = Color::DarkBlue;
Color clDoneBk = Color::DarkGreen;

wstring text;
static RECT rApp;

Color clTextNormal = Color::LightGray;
Color clTextActive = Color::White;
Color* clText = &clTextNormal;

FontFamily* fontFamily;
Font* fontNormal;
Font* fontActive;
Font* font;

RECT rDsk;
HWND hDsk;

BOOL mouseTrack = true;
BOOL mouseDrag = false;
POINT dragStartPos;

bool blink = false;
Color  clrBinkTxt1(255, 200, 200, 200);
Color  clrBinkTxt2(255, 255, 255, 255);
Color* clrTxt = &clrBinkTxt1;

ULONG_PTR  gdiplusToken;

AppConfig* cfg;
OptionsDialog* dlgOptions;

extern HINSTANCE hInst;

HWND hWnd;
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name

UINT_PTR tmBlink;
UINT_PTR tmShow;

static Operations curOperation = Operations::opUndefined;
static wstring curApp = L"";

bool MainWindow::Init(HINSTANCE hInstance)
{
    // Initialize GDI+.
    GdiplusStartupInput gdiplusStartupInput;
    GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);


    // Initialize global strings
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_RUNSYNC, szWindowClass, MAX_LOADSTRING);
    RegisterClass(hInstance);
    
    hInst = hInstance; // Store instance handle in our global variable
    hWnd = CreateWindowExW(WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_TOOLWINDOW,
        szWindowClass, szTitle,
        WS_POPUP,
        0, 0, 0, 0,
        nullptr, nullptr, hInst, nullptr);

    if (!hWnd)
    {
        return FALSE;
    }


    auto n = cfg->getAppName();


    hDsk = GetDesktopWindow();
    GetWindowRect(hDsk, &rDsk);

    REAL fsize = ((REAL)(rDsk.bottom - rDsk.top)) / 36;
    if (fsize < 9)fsize = 9;

    fontFamily = new FontFamily((L"Segoe UI"));
    fontNormal = new Font(fontFamily, fsize, FontStyleRegular, UnitPixel);
    fontActive = new Font(fontFamily, fsize, FontStyleBold, UnitPixel);
    font = fontNormal;

    NONCLIENTMETRICSW mtx;
    mtx.cbSize = sizeof(NONCLIENTMETRICS);
    if (!SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, sizeof(NONCLIENTMETRICS), &mtx, 0))
    {
        MessageBox(nullptr, L"Can not get system metrics", L"Error", MB_ICONERROR);
    };

    OptionsDialog::Init(hInst, hWnd);

    mtx.lfMenuFont.lfHeight = (rDsk.bottom - rDsk.top) / 28;
    if (mtx.lfMenuFont.lfHeight < 9)mtx.lfMenuFont.lfHeight = 9;


    SetLayeredWindowAttributes(hWnd, 0, (255 * 80) / 100, LWA_ALPHA);


    UpdateWindow(hWnd);

    curOperation = Operations::opNone;

    tmBlink = SetTimer(hWnd, 1, 500, nullptr);
    tmShow = SetTimer(hWnd, 2, 500, nullptr);

    AppManager::AppStart(AppConfig::getAppName());

    return true;
}

void MainWindow::Dispose()
{
    GdiplusShutdown(gdiplusToken);
}

void MainWindow::Show(wstring txt)
{
    text = txt;

    HDC hdc = GetWindowDC(hDsk);
    Graphics graphics(hdc);
    
    RectF r;
    graphics.MeasureString(text.c_str(), (INT)text.length(), font, PointF(0,0), &r);

    rApp.left = (long)(rDsk.right - r.Width);
    rApp.top = 0;
    rApp.right = (long)r.Width;
    rApp.bottom = (LONG)font->GetHeight(&graphics) + BOTTOM_PADDING;

    SetWindowPos(hWnd, 0, rApp.left, rApp.top, (int)r.Width, (int)r.Height, 0);    

    AnimateWindow(hWnd, 300, AW_SLIDE |  AW_VER_POSITIVE);

    
}

void MainWindow::Hide()
{
    blink = false;
    AnimateWindow(hWnd, 300, AW_SLIDE | AW_VER_NEGATIVE | AW_HIDE);
}

HWND MainWindow::getHandle()
{
    return hWnd;
}

void MainWindow::Draw(HDC hdc)
{
    Graphics graphics(hdc);

    Gdiplus::RectF r(0, 0, (REAL)rApp.right, (REAL)rApp.bottom);
    /*

    LinearGradientBrush  brush(r, Color(255, 80, 150, 80),
        Color(255, 100, 200, 100),
        90);
        */

    SolidBrush brush(Color::Black);
    switch (curOperation)
    {
        case Operations::opInstall: brush.SetColor(clInstallationBk); break;
        case Operations::opUpgrade: brush.SetColor(clUpgradingBk); break;
        default : brush.SetColor(clDoneBk); break;
    }
    graphics.FillRectangle(&brush, r);


    SolidBrush tbrush(*clrTxt);
    graphics.DrawString(text.c_str(), (INT)text.length(),
        font,
        Gdiplus::PointF(0, 0),
        &tbrush);

}

LRESULT CALLBACK MainWindow::WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{

    switch (message)
    {

    case WM_PRINTCLIENT:
    {

        HDC hdc = (HDC)wParam;
        Draw(hdc);
    }
    break;

    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);
        Draw(hdc);
        EndPaint(hWnd, &ps);
    }
    break;

    case WM_SIZE:
    {
        if (hWnd == hDsk)
        {
            SetWindowPos(hWnd, 0, 0, 0, 0, 0, 0);
        }
    }
    break;

    //case WM_SHOWWINDOW:
    //    AnimateWindow(hWnd, 1000, AW_SLIDE | AW_HOR_NEGATIVE);
    //    break;

    case WM_LBUTTONDOWN:
    {
        mouseDrag = true;
        dragStartPos.x = GET_X_LPARAM(lParam);
        dragStartPos.y = GET_Y_LPARAM(lParam);
        SetCapture(hWnd);
    }
    break;

    case WM_LBUTTONUP:
    {
        mouseDrag = false;
        ReleaseCapture();
    }
    break;


    case WM_MOUSEMOVE:
    {
        if (mouseDrag)
        {
            POINT pos;
            pos.x = GET_X_LPARAM(lParam);
            pos.y = GET_Y_LPARAM(lParam);

            RECT r;
            GetWindowRect(hWnd, &r);
            auto width = r.right - r.left;
            auto height = r.bottom - r.top;
            
            ClientToScreen(hWnd, &pos);

            pos.x -= dragStartPos.x;
            pos.y -= dragStartPos.y;

            //Do not let window leave desktop space 
            if (pos.x + width > rDsk.right)
                pos.x = rDsk.right - width;
            if (pos.y + height > rDsk.bottom)
                pos.y = rDsk.bottom - height;            
            if (pos.x <= 0)pos.x = 0;
            if (pos.y <= 0)pos.y = 0;

            MoveWindow(hWnd, pos.x, pos.y, width , height, FALSE);

        }
        /*
        if (mouseTrack)
        {
            TRACKMOUSEEVENT me;
            me.cbSize = sizeof(TRACKMOUSEEVENT);
            me.dwFlags = TME_HOVER | TME_LEAVE;
            me.dwHoverTime = 10;
            me.hwndTrack = hWnd;
            TrackMouseEvent(&me);
        }*/
    }
    break;

    case WM_MOUSEHOVER: {
        mouseTrack = false;
        clText = &clTextActive;
        InvalidateRect(hWnd, 0, TRUE);
    }
    break;

    case WM_MOUSELEAVE: {
        clText = &clTextNormal;
        InvalidateRect(hWnd, 0, TRUE);
        mouseTrack = true;
    }
    break;


    case WM_CONTEXTMENU: {
        if (hWnd == hDsk)
        {
            SetWindowPos(hWnd, 0, 0, 0, 0, 0, 0);
        }
    }
    break;

    case WM_TIMER: {
        if(wParam == tmBlink)
        {
            if (blink)
            {
                if (clrTxt == &clrBinkTxt1)
                    clrTxt = &clrBinkTxt2;
                else
                    clrTxt = &clrBinkTxt1;
                InvalidateRect(hWnd, 0, TRUE);
            }
            else
            {
                if (clrTxt != &clrBinkTxt1)
                {
                    clrTxt = &clrBinkTxt1;
                    InvalidateRect(hWnd, 0, TRUE);
                    
                }
            }
        }
        else if(wParam == tmShow)
        {
            auto cur =  AppManager::GetCurrentAppOperation();
            if (curOperation != cur.second || curApp != cur.first)
            {
                curApp = cur.first;
                curOperation = cur.second;
                if ((curOperation != Operations::opInstall) && (curOperation != Operations::opUpgrade))
                {
                    Hide();
                }
                else
                {
                    if (curOperation == Operations::opInstall)
                    {
                        blink = true;
                        Show(L"Installing " + cur.first + L" ...");
                    }
                    else if (curOperation == Operations::opUpgrade)
                    {
                        blink = true;
                        Show(L"Upgrading " + cur.first + L" ...");
                    }
                }
            }           
        }

    }
    break;


    case WM_DESTROY:
        PostQuitMessage(0);
        break;


    case WM_COPYDATA:
    {
        auto data = (PCOPYDATASTRUCT)lParam;
        WCHAR* lbuf;
        lbuf = new WCHAR[(size_t)(lstrlen((LPWSTR)data->lpData) + sizeof(WCHAR))];
        lstrcpy(lbuf, (LPWSTR)data->lpData);

        AppManager::AppStart(lbuf);

        delete[] lbuf;
    }
    break;


    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

ATOM MainWindow::RegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_RUNSYNC));
    wcex.hCursor = LoadCursor(nullptr, IDC_HAND);
    wcex.hbrBackground = CreateSolidBrush(RGB(255, 255, 255));//  (HBRUSH)(COLOR_INFOBK + 1);
    wcex.lpszMenuName = nullptr;//MAKEINTRESOURCEW(IDR_MENU1);
    wcex.lpszClassName = szWindowClass;
    wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}
