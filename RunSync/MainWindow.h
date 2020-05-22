#pragma once
#include <string>
#include "framework.h"

using namespace std;

namespace RunSync
{
	class MainWindow
	{
	public:
		static bool Init(HINSTANCE hInstance);
		static void Dispose();
		static void Show(wstring txt);
		static void Hide();
		static HWND getHandle();
	private:
		static ATOM RegisterClass(HINSTANCE hInstance);
		static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
		static void Draw(HDC hdc);

	};
}

