#include <windows.h>
#include <stdio.h>

struct
{
    PROCESS_INFORMATION *pi;
    HWND hWnd;
} SndVolProcess;

void SndVolWndProc(HWINEVENTHOOK hWinEventHook,
                   DWORD event,
                   HWND hWnd,
                   LONG idObject,
                   LONG idChild,
                   DWORD idEventThread,
                   DWORD dwmsEventTime)
{
    if (event != EVENT_SYSTEM_FOREGROUND)
        return;

    DWORD pid;
    GetWindowThreadProcessId(hWnd, &pid);
    if (pid == SndVolProcess.pi->dwProcessId)
    {
        SwitchToThisWindow(SndVolProcess.hWnd, TRUE);
        return;
    };
    UnhookWinEvent(hWinEventHook);

    ShowWindow(SndVolProcess.hWnd, SW_HIDE);
    TerminateProcess(SndVolProcess.pi->hProcess, 0);

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

    SndVolProcess.hWnd = hWnd;
    MSG msg;
    RECT wndRect;
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
    }

    PostQuitMessage(0);
}

LRESULT WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    static UINT WM_TASKBARCREATED;
    static PROCESS_INFORMATION pi;
    static STARTUPINFO si = {.cb = sizeof(STARTUPINFO)};
    static MSG msg;
    static NOTIFYICONDATA nid = {.cbSize = sizeof(NOTIFYICONDATA),
                                 .uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP,
                                 .uCallbackMessage = WM_USER + 1,
                                 .szTip = "Volume Mixer"};

    switch (uMsg)
    {
    case WM_CREATE:
        HICON hIcon;
        SndVolProcess.pi = &pi;
        WM_TASKBARCREATED = RegisterWindowMessage("TaskbarCreated");
        ExtractIconEx("C:\\Windows\\System32\\SndVol.exe", 0, NULL, &hIcon, 1);
        nid.hWnd = hWnd;
        nid.hIcon = hIcon;
        Shell_NotifyIcon(NIM_ADD, &nid);
        break;

    case WM_USER + 1:
        switch (lParam)
        {
        case WM_LBUTTONDOWN:
            CreateProcess("C:\\Windows\\System32\\SndVol.exe", NULL, NULL, NULL, FALSE, CREATE_SUSPENDED, NULL, NULL, &si, &pi);
            SetWinEventHook(EVENT_OBJECT_SHOW,
                            EVENT_OBJECT_SHOW,
                            NULL,
                            SndVolProcessProc,
                            pi.dwProcessId,
                            0,
                            WINEVENT_OUTOFCONTEXT);
            ResumeThread(pi.hThread);
            while (GetMessage(&msg, NULL, 0, 0) > 0)
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
            CloseHandle(pi.hThread);
            CloseHandle(pi.hProcess);
            break;

        case WM_RBUTTONDBLCLK:
            Shell_NotifyIcon(NIM_DELETE, &nid);
            if (MessageBox(hWnd,
                           "Exit?",
                           "SndVolTray",
                           MB_YESNO | MB_ICONQUESTION | MB_SYSTEMMODAL) == IDYES)
                ExitProcess(0);
            Shell_NotifyIcon(NIM_ADD, &nid);
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