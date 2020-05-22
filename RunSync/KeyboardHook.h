#pragma once
#include <windows.h>
namespace RunSync
{
    class KeyboardHook
    {

    public:
        static void Init();

    private:

        static HHOOK hHook;

        static bool isCtlPressed;
        static bool isAltPressed;
        static bool isShiftPressed;

        static LRESULT CALLBACK KeyboardHookProc(
            _In_ int    code,
            _In_ WPARAM wParam,
            _In_ LPARAM lParam
        );
    };
}

