#include "ActivationService.h"

#include <windows.h>

bool ActivationService::activate(WId handle)
{
    HWND hwnd = reinterpret_cast<HWND>(handle);
    if (hwnd == nullptr || !IsWindow(hwnd))
    {
        return false;
    }

    if (IsIconic(hwnd))
    {
        ShowWindow(hwnd, SW_RESTORE);
    }

    if (SetForegroundWindow(hwnd))
    {
        SetWindowPos(hwnd, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
        return true;
    }

    const HWND foreground = GetForegroundWindow();
    const DWORD foregroundThread =
        foreground != nullptr ? GetWindowThreadProcessId(foreground, nullptr) : 0;
    const DWORD targetThread = GetWindowThreadProcessId(hwnd, nullptr);

    if (foregroundThread != 0 && targetThread != 0 && foregroundThread != targetThread)
    {
        AttachThreadInput(foregroundThread, targetThread, TRUE);
        SetForegroundWindow(hwnd);
        AttachThreadInput(foregroundThread, targetThread, FALSE);
    }

    if (GetForegroundWindow() == hwnd)
    {
        SetWindowPos(hwnd, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
        return true;
    }

    return false;
}
