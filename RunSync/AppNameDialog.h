#pragma once
#include "framework.h"

namespace RunSync
{
	class AppNameDialog
	{
	public:
		static INT_PTR Show();
	private:
		static HINSTANCE hInst;
		static HWND mainWindowHandle;
		static INT_PTR CALLBACK DlgProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam);
	};
}

