#include "KeyboardHook.h"
#include "OptionsDialog.h"


using namespace RunSync;

HHOOK KeyboardHook::hHook;
bool KeyboardHook::isCtlPressed = false;
bool KeyboardHook::isAltPressed = false;
bool KeyboardHook::isShiftPressed = false;

void KeyboardHook::Init()
{
    hHook = SetWindowsHookEx(WH_KEYBOARD_LL, KeyboardHookProc, nullptr, 0);

    if (hHook == nullptr)
    {
        DWORD err = GetLastError();
    }
}


LRESULT CALLBACK KeyboardHook::KeyboardHookProc(
    _In_ int    code,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
)
{
    if (code < 0 || OptionsDialog::IsActive())return  CallNextHookEx(hHook, code, wParam, lParam);

    LPKBDLLHOOKSTRUCT hs = (LPKBDLLHOOKSTRUCT)lParam;

    if (wParam == WM_SYSKEYDOWN)
        KeyboardHook::isAltPressed = hs->flags & LLKHF_ALTDOWN;

    if (code == HC_ACTION && wParam == WM_KEYDOWN)
    {
        bool pressed = false;
        if (hs->vkCode == 0x52)
        {
            isCtlPressed = ((GetKeyState(VK_CONTROL) & 0x8000) > 0);
            isShiftPressed = ((GetKeyState(VK_SHIFT) & 0x8000) > 0);
            if (isCtlPressed && isShiftPressed /*&& isAltPressed*/)
                pressed = true;
        }
        if (pressed)
        {
            isAltPressed = false;
            OptionsDialog::Show();
        }
        //isAltPressed = false;
    }
    return CallNextHookEx(hHook, code, wParam, lParam);
}
