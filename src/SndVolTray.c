/*
SndTrayVol: https://github.com/Aetopia/SndTrayVol
Author: Aetopia
License: MIT
*/

#include <windows.h>

PROCESS_INFORMATION *SndVolProcess;

static inline void KillProcess(HANDLE hProcess)
{
    TerminateProcess(hProcess, 0);
    CloseHandle(hProcess);
}

void SndVolWndProc(HWINEVENTHOOK hWinEventHook,
                   DWORD event,
                   HWND hWnd,
                   LONG idObject,
                   LONG idChild,
                   DWORD idEventThread,
                   DWORD dwmsEventTime)
{
    if (event != EVENT_SYSTEM_FOREGROUND &&
        idObject != OBJID_WINDOW &&
        idChild != CHILDID_SELF)
        return;
    UnhookWinEvent(hWinEventHook);
    KillProcess(SndVolProcess->hProcess);
    PostQuitMessage(0);
}

void SndVolProcessProc(
    HWINEVENTHOOK hWinEventHook,
    DWORD event,
    HWND hWnd,
    LONG idObject,
    LONG idChild,
    DWORD idEventThread,
    DWORD dwmsEventTime)
{
    if (event != EVENT_OBJECT_SHOW &&
        idObject != OBJID_WINDOW &&
        idChild != CHILDID_SELF)
        return;
    UnhookWinEvent(hWinEventHook);

    RECT wndRect;
    MSG msg;
    APPBARDATA taskbar = {.cbSize = sizeof(APPBARDATA)};
    int taskbarCX,
        taskbarCY,
        wndX,
        wndY,
        wndCX,
        wndCY;

    SHAppBarMessage(ABM_GETTASKBARPOS, &taskbar);
    GetWindowRect(hWnd, &wndRect);
    taskbarCX = taskbar.rc.right - taskbar.rc.left;
    taskbarCY = taskbar.rc.bottom - taskbar.rc.top;
    wndCX = wndRect.right - wndRect.left;
    wndCY = wndRect.bottom - wndRect.top;

    switch (taskbar.uEdge)
    {
    case ABE_LEFT:
        wndX = taskbar.rc.right;
        wndY = taskbarCY - wndCY;
        break;
    case ABE_TOP:
        wndX = taskbarCX - wndCX;
        wndY = taskbar.rc.bottom;
        break;
    case ABE_RIGHT:
        wndX = taskbar.rc.left - wndCX;
        wndY = taskbarCY - wndCY;
        break;
    case ABE_BOTTOM:
        wndX = taskbarCX - wndCX;
        wndY = taskbar.rc.top - wndCY;
        break;
    };
    SetWindowPos(hWnd,
                 0,
                 wndX,
                 wndY,
                 0,
                 0,
                 SWP_NOSIZE);

    SetWinEventHook(EVENT_SYSTEM_FOREGROUND,
                    EVENT_SYSTEM_FOREGROUND,
                    NULL,
                    SndVolWndProc,
                    0, 0,
                    WINEVENT_OUTOFCONTEXT);
    while (GetMessage(&msg, NULL, 0, 0) > 0)
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    };

    PostQuitMessage(0);
}

LRESULT WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    static UINT WM_TASKBARCREATED;
    static PROCESS_INFORMATION pi;
    static NOTIFYICONDATA nid = {.cbSize = sizeof(NOTIFYICONDATA),
                                 .uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP,
                                 .uCallbackMessage = WM_USER + 1,
                                 .szTip = "Volume Mixer"};

    switch (uMsg)
    {
    case WM_CREATE:
        HICON hIcon;
        SndVolProcess = &pi;
        WM_TASKBARCREATED = RegisterWindowMessage("TaskbarCreated");
        ExtractIconEx("C:\\Windows\\System32\\SndVol.exe", 0, NULL, &hIcon, 1);
        nid.hWnd = hWnd;
        nid.hIcon = hIcon;
        Shell_NotifyIcon(NIM_ADD, &nid);
        break;

    case WM_USER + 1:
        switch (lParam)
        {
        case WM_LBUTTONDBLCLK:
        case WM_LBUTTONDOWN:
            STARTUPINFO si = {.cb = sizeof(STARTUPINFO)};
            MSG msg;
            KillProcess(pi.hProcess);
            CreateProcess("C:\\Windows\\System32\\SndVol.exe", NULL, NULL, NULL, FALSE, CREATE_SUSPENDED, NULL, NULL, &si, &pi);
            SetWinEventHook(EVENT_OBJECT_SHOW,
                            EVENT_OBJECT_SHOW,
                            NULL,
                            SndVolProcessProc,
                            pi.dwProcessId,
                            0,
                            WINEVENT_OUTOFCONTEXT);
            ResumeThread(pi.hThread);
            CloseHandle(pi.hThread);
            while (GetMessage(&msg, NULL, 0, 0) > 0)
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            };
            break;

        case WM_RBUTTONDOWN:
            POINT pt;
            HMENU hMenu;
            SwitchToThisWindow(hWnd, TRUE);
            GetCursorPos(&pt);
            hMenu = CreatePopupMenu();
            AppendMenu(hMenu, 0, 1, "Exit");
            TrackPopupMenu(hMenu, 0, pt.x, pt.y, 0, hWnd, NULL);
        };

    case WM_COMMAND:
        if (wParam)
        {
            Shell_NotifyIcon(NIM_DELETE, &nid);
            ExitProcess(0);
        };

    default:
        if (uMsg == WM_TASKBARCREATED)
            Shell_NotifyIcon(NIM_ADD, &nid);
        return DefWindowProc(hWnd, uMsg, wParam, lParam);
    }
    return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                   LPSTR lpCmdLine, int nCmdShow)
{
    WNDCLASS wndClass = {.lpfnWndProc = WndProc,
                         .hInstance = hInstance,
                         .lpszClassName = "SndVolTray"};
    HANDLE mutex = OpenMutex(MUTEX_ALL_ACCESS, FALSE, "SndVolTray");
    MSG msg;

    if (mutex ||
        (!RegisterClass(&wndClass) ||
         !CreateWindow("SndVolTray", 0, 0, 0, 0, 0, 0, NULL, NULL, hInstance, NULL)))
        return 1;
    mutex = CreateMutex(NULL, FALSE, "SndVolTray");

    while (GetMessage(&msg, NULL, 0, 0) > 0)
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return 0;
}