#pragma once
#include "AppConfig.h"
#include "AppManager.h"

#define WM_RUNSYNC_OPTIONS_UPDATE (WM_USER + 0x0001)

using namespace std;

namespace RunSync
{
	class OptionsDialog
	{
	public:
		static void Init(HINSTANCE hInst,HWND mainWnd);
		static INT_PTR Show();
		static bool IsActive();
		static void Update();
		static void setErrorMsg(wstring msg) { errMsg = msg; Update(); };
	private:
		static wstring errMsg;
		static HINSTANCE hInst;
		static HWND mainWindowHandle;
		static bool isActive;
		static void LoadData(HWND hwnd);
		static INT_PTR CALLBACK OptionsDlgProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam);
		static INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
	};
}

