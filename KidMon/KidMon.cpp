/*
Copyright(c) 2021 by MandySoft

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files(the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions :

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/

#include <windows.h>
#include <tlhelp32.h>
#include <tchar.h>
#include <time.h>
#include <psapi.h>
#include <Lmcons.h>
#include <WinUser.h>
#include <string>
#include <iostream>
#include <algorithm>

#include "CKidMonWindows.h"
#include "KidMonUtils.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

using namespace std;

void printError(TCHAR* msg)
{
    DWORD eNum;
    TCHAR sysMsg[256];
    TCHAR* p;

    eNum = GetLastError();
    FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL, eNum,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
        sysMsg, 256, NULL);

    // Trim the end of the line and terminate it with a null
    p = sysMsg;
    while ((*p > 31) || (*p == 9))
        ++p;
    do { *p-- = 0; } while ((p >= sysMsg) &&
        ((*p == '.') || (*p < 33)));

    // Display the message
    fprintf(stderr, TEXT("\n  WARNING: %s failed with error %d (%s)\n"), msg, eNum, sysMsg);
}

// there is only wide char variant in API
LPSTR* CommandLineToArgvA(LPSTR lpCmdLine, INT *pNumArgs)
{
    int retval;
    retval = MultiByteToWideChar(CP_ACP, MB_ERR_INVALID_CHARS, lpCmdLine, -1, NULL, 0);
    if (!SUCCEEDED(retval))
        return NULL;

    LPWSTR lpWideCharStr = (LPWSTR)malloc(retval * sizeof(WCHAR));
    if (lpWideCharStr == NULL)
        return NULL;

    retval = MultiByteToWideChar(CP_ACP, MB_ERR_INVALID_CHARS, lpCmdLine, -1, lpWideCharStr, retval);
    if (!SUCCEEDED(retval))
    {
        free(lpWideCharStr);
        return NULL;
    }

    int numArgs;
    LPWSTR* args;
    args = CommandLineToArgvW(lpWideCharStr, &numArgs);
    free(lpWideCharStr);
    if (args == NULL)
        return NULL;

    int storage = numArgs * sizeof(LPSTR);
    for (int i = 0; i < numArgs; ++i)
    {
        BOOL lpUsedDefaultChar = FALSE;
        retval = WideCharToMultiByte(CP_ACP, 0, args[i], -1, NULL, 0, NULL, &lpUsedDefaultChar);
        if (!SUCCEEDED(retval))
        {
            LocalFree(args);
            return NULL;
        }

        storage += retval;
    }

    LPSTR* result = (LPSTR*)LocalAlloc(LMEM_FIXED, storage);
    if (result == NULL)
    {
        LocalFree(args);
        return NULL;
    }

    int bufLen = storage - numArgs * sizeof(LPSTR);
    LPSTR buffer = ((LPSTR)result) + numArgs * sizeof(LPSTR);
    for (int i = 0; i < numArgs; ++i)
    {
        //assert(bufLen > 0);
        BOOL lpUsedDefaultChar = FALSE;
        retval = WideCharToMultiByte(CP_ACP, 0, args[i], -1, buffer, bufLen, NULL, &lpUsedDefaultChar);
        if (!SUCCEEDED(retval))
        {
            LocalFree(result);
            LocalFree(args);
            return NULL;
        }

        result[i] = buffer;
        buffer += retval;
        bufLen -= retval;
    }

    LocalFree(args);

    *pNumArgs = numArgs;
    return result;
}

// https://stackoverflow.com/questions/191842/how-do-i-get-console-output-in-c-with-a-windows-program

bool RedirectConsoleIO()
{
    bool result = true;
    FILE* fp;

    // Redirect STDIN if the console has an input handle
    if (GetStdHandle(STD_INPUT_HANDLE) != INVALID_HANDLE_VALUE)
        if (freopen_s(&fp, "CONIN$", "r", stdin) != 0)
            result = false;
        else
            setvbuf(stdin, NULL, _IONBF, 0);

    // Redirect STDOUT if the console has an output handle
    if (GetStdHandle(STD_OUTPUT_HANDLE) != INVALID_HANDLE_VALUE)
        if (freopen_s(&fp, "CONOUT$", "w", stdout) != 0)
            result = false;
        else
            setvbuf(stdout, NULL, _IONBF, 0);

    // Redirect STDERR if the console has an error handle
    if (GetStdHandle(STD_ERROR_HANDLE) != INVALID_HANDLE_VALUE)
        if (freopen_s(&fp, "CONOUT$", "w", stderr) != 0)
            result = false;
        else
            setvbuf(stderr, NULL, _IONBF, 0);

    // Make C++ standard streams point to console as well.
    ios::sync_with_stdio(true);

    // Clear the error state for each of the C++ standard streams.
    std::wcout.clear();
    std::cout.clear();
    std::wcerr.clear();
    std::cerr.clear();
    std::wcin.clear();
    std::cin.clear();

    return result;
}

bool ReleaseConsole()
{
    bool result = true;
    FILE* fp;

    // Just to be safe, redirect standard IO to NUL before releasing.

    // Redirect STDIN to NUL
    if (freopen_s(&fp, "NUL:", "r", stdin) != 0)
        result = false;
    else
        setvbuf(stdin, NULL, _IONBF, 0);

    // Redirect STDOUT to NUL
    if (freopen_s(&fp, "NUL:", "w", stdout) != 0)
        result = false;
    else
        setvbuf(stdout, NULL, _IONBF, 0);

    // Redirect STDERR to NUL
    if (freopen_s(&fp, "NUL:", "w", stderr) != 0)
        result = false;
    else
        setvbuf(stderr, NULL, _IONBF, 0);

    // Detach from console
    if (!FreeConsole())
        result = false;

    return result;
}

void AdjustConsoleBuffer(int16_t minLength)
{
    // Set the screen buffer to be big enough to scroll some text
    CONSOLE_SCREEN_BUFFER_INFO conInfo;
    GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &conInfo);
    if (conInfo.dwSize.Y < minLength)
        conInfo.dwSize.Y = minLength;
    SetConsoleScreenBufferSize(GetStdHandle(STD_OUTPUT_HANDLE), conInfo.dwSize);
}

bool CreateNewConsole(int16_t minLength)
{
    bool result = false;

    // Release any current console and redirect IO to NUL
    ReleaseConsole();

    // Attempt to create new console
    if (AllocConsole())
    {
        AdjustConsoleBuffer(minLength);
        result = RedirectConsoleIO();
    }

    return result;
}

bool AttachParentConsole(int16_t minLength)
{
    bool result = false;

    // Release any current console and redirect IO to NUL
    ReleaseConsole();

    // Attempt to attach to parent process's console
    if (AttachConsole(ATTACH_PARENT_PROCESS))
    {
        AdjustConsoleBuffer(minLength);
        result = RedirectConsoleIO();
    }

    return result;
}

#define CONSOLE_LEN 1024

static BOOL fStopping = FALSE;
static HANDLE hStoppedEvent;
HWND hHiddenWindow;

BOOL WINAPI HandlerRoutine(_In_ DWORD dwCtrlType) {
    /*
    https://docs.microsoft.com/en-us/windows/console/handlerroutine
    Note: If a console application loads the gdi32.dll or user32.dll library, the HandlerRoutine function that you specify
    when you call SetConsoleCtrlHandler does not get called for the CTRL_LOGOFF_EVENT and CTRL_SHUTDOWN_EVENT events.

    Handler is called in separate thread.
    */
    fStopping = TRUE;
    SetEvent(hStoppedEvent);
    PostMessageA(hHiddenWindow, WM_QUIT, 0, 0);  // to terminate GetMessage() immediately
    printf(TEXT("\r\nEvent (Ctrl+C, ...) detected. Stopping...\n"));
    return TRUE;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    // called in main thread
    debugf(TEXT("WndProc(%u, %d, %d)\n"), uMsg, wParam, lParam);
    switch (uMsg) {
    case WM_QUERYENDSESSION:
        return TRUE;
    case WM_ENDSESSION:
        fStopping = TRUE;
        SetEvent(hStoppedEvent);  // make sense only for console
        break;
    }
    return DefWindowProcA(hWnd, uMsg, wParam, lParam);
}

/*
    We cannot use plain main() as it always creates console window. WinMain() without GUI window is completely hidden unless console is created.
*/
int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int cmdShow) {
    int nRetCode = 0;
    const char *dbFilename = CKidMonWindows::DEFAULT_DBFILENAME;
    unsigned int heartbeat = CKidMonWindows::DEFAULT_HEARTBEAT;
    unsigned int pollInterval = CKidMonWindows::DEFAULT_POLLINTERVAL;

    LPSTR *szArgList;
    int argCount;
    szArgList = CommandLineToArgvA(GetCommandLineA(), &argCount);
    std::string exeFilename(szArgList[0]);
    int exeBasenameIdx = getBasenameIdx(exeFilename);

    bool fConsole = true;
    if (szArgList != NULL) {
        char *c;
        if (cmdOptionExists(szArgList, szArgList + argCount, "-h")) {
            if (AttachParentConsole(CONSOLE_LEN) || CreateNewConsole(CONSOLE_LEN))
            {
                printf(TEXT("KidMon v" KIDMON_VERSION ", Copyright (c) 2021 by MandySoft\n"));
                printf(TEXT("\n"));
                printf(TEXT("usage: %s [-h] [-c] [-v] [-i <polling_interval>] [-b <heartbeat>] [-f <db_filename>]\n"), exeFilename.substr(exeBasenameIdx).c_str());
                printf(TEXT("\n"));
                printf(TEXT("Usage:\n"));
                printf(TEXT("  -h  print help\n"));
                printf(TEXT("  -c  run without console\n"));
                printf(TEXT("  -v  print debug messages\n"));
                printf(TEXT("  <db_filename>   CSV file where data are hourly flushed, default: %s\n"), dbFilename);
                printf(TEXT("  <polling_interval>   in ms, default: %u\n"), pollInterval);
                printf(TEXT("  <heartbeat>   in ms in which data must be polled to be considered as valid, default: %u\n"), heartbeat);
                nRetCode = -1;
                goto cleanup_console;
            }
            else {
                nRetCode = 1;
                goto cleanup_arglist;
            }
        }
        if (cmdOptionExists(szArgList, szArgList + argCount, "-c")) {
            fConsole = false;
        }
        if (cmdOptionExists(szArgList, szArgList + argCount, "-v")) {
            fVerbose++;
        }
        c = getCmdOption(szArgList, szArgList + argCount, "-f");
        if (c) {
            dbFilename = c;
        }
        c = getCmdOption(szArgList, szArgList + argCount, "-b");
        if (c) {
            heartbeat = stoul(c);
        }
        c = getCmdOption(szArgList, szArgList + argCount, "-i");
        if (c) {
            pollInterval = stoul(c);
        }
    }
    if (fConsole) {
        if (!AttachParentConsole(CONSOLE_LEN) && !CreateNewConsole(CONSOLE_LEN))
        {
            nRetCode = 1;
            goto cleanup_arglist;
        }
    }
    /*
    To receive events when a user signs out or the device shuts down in these circumstances,
    create a hidden window in your console application, and then handle the WM_QUERYENDSESSION and
    WM_ENDSESSION window messages that the hidden window receives.
    We can create a hidden window by calling the CreateWindowEx method with the dwExStyle parameter set to 0.
    */
    LPCTSTR lpcszClassName = "KidMonHiddenWindowClass";
    if (!hPrevInstance) {
        WNDCLASSEXA wx;
        ZeroMemory(&wx, sizeof(wx));
        wx.cbSize = sizeof(wx);
        wx.lpfnWndProc = WndProc;
        wx.hInstance = hInstance;
        wx.lpszClassName = lpcszClassName;
        if (!RegisterClassExA(&wx)) {
            printError(TEXT("RegisterClassEx"));
            nRetCode = 1;
            goto cleanup_arglist;
        }
    }
    /*
    We cannot use message-only window to receive broadcast messages as WM_QUERYENDSESSION, WM_ENDSESSION etc.
    */
    hHiddenWindow = CreateWindowEx(0, lpcszClassName, NULL, 0, 0, 0, 0, 0, NULL /*HWND_MESSAGE*/, NULL, hInstance, NULL);
    debugf(TEXT("hHiddenWindow: %p\n"), hHiddenWindow);
    if (!hHiddenWindow) {
        printError(TEXT("CreateWindowEx"));
        nRetCode = 1;
        goto cleanup_arglist;
    }
    // allow only one instance per user
    TCHAR  czUserName[UNLEN + 1];
    DWORD dwUserLen = UNLEN;
    if (GetUserNameA(czUserName, &dwUserLen) == 0) {
        printError(TEXT("GetUserName"));
        nRetCode = 1;
        goto cleanup_console;
    }

    TCHAR czEventName[MAX_PATH];
    snprintf(czEventName, sizeof(czEventName), "LocalKidMonInstance_%s", czUserName);  // Local or Global prefix namespace ?
    debugf(TEXT("Event: %s\n"), czEventName);
    // we create event to reuse it for stopping (instead of simpler mutex)
    hStoppedEvent = CreateEventA(NULL, TRUE, FALSE, czEventName);
    if (hStoppedEvent == NULL) {
        printError(TEXT("CreateEvent"));
        nRetCode = 1;
        goto cleanup_console;
    }
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        fprintf(stderr, TEXT("Program instance already exists for user: %s\n"), czUserName);
        nRetCode = 1;
        goto cleanup_event;
    }
    if (fConsole) {
        SetConsoleCtrlHandler(HandlerRoutine, TRUE);
    }

    debugf(TEXT("GetCommandLineA: %s\n"), GetCommandLineA());
    for (int i = 0; i < argCount; i++) {
        debugf(TEXT("%d: %s\n"), i, szArgList[i]);
    }
    debugf(TEXT("ProcessId: %d, ThreadId: %d\n"), GetCurrentProcessId(), GetCurrentThreadId());
    {
        CKidMonWindows mon(dbFilename, heartbeat);
        printf(TEXT("DB: %s, Heartbeat: %u ms, Polling: %u ms\r\n"), mon.GetDbFilename().c_str(), mon.GetHeartbeat(), pollInterval);
        int n = 0;
        int lastLen = 0;
        DWORD dwStamp = GetTickCount();
        while (!fStopping)
        {
            const char *fn = mon.GetProcessNameStr().c_str();
            const char *p = fn;
            while (*p != '\0') {
                if (*p == '\\') {
                    p++;
                    fn = p;
                }
                else {
                    p++;
                }
            }
            int len = printf(TEXT("Polling: %d %d [%d, %s]"), n, GetTickCount() - dwStamp, mon.GetActivity(), fn);
            while (lastLen > len) {
                // clear trailling chars
                lastLen--;
                printf(" ");
            }
            lastLen = len;
            printf("\r");
            mon.Poll();

            // why fStopping has not TRUE value when has been set in handler ? Multithreads but it should be global variable ???
            fStopping |= WaitForSingleObject(hStoppedEvent, 0) == WAIT_OBJECT_0;
            UINT_PTR timerId = SetTimer(NULL, NULL, pollInterval, NULL);
            if (timerId) {
                MSG msg;
                fStopping |= !GetMessage(&msg, NULL, 0, 0);
                debugf(TEXT("Msg: (%d, %d, %d)\n"), msg.message, msg.wParam, timerId);
                KillTimer(NULL, timerId);
                fStopping |= WaitForSingleObject(hStoppedEvent, 0) == WAIT_OBJECT_0;
                if (!fStopping) {
                    if (msg.message == WM_TIMER && msg.hwnd == NULL && msg.wParam == timerId) {
                        // do polling
                    }
                    else {
                        // TranslateMessage(&msg);
                        DispatchMessage(&msg);
                    }
                }
            }
            else {
                // timer creation failed, use simpler solution
                WaitForSingleObject(hStoppedEvent, pollInterval);
            }
            n++;
        }
        // here object desctructor is executed and data flushed
    }
    printf(TEXT("\r\nDone\n"));
cleanup_event:
    CloseHandle(hStoppedEvent);
cleanup_console:
    if (fConsole) {
        ReleaseConsole();
    }
cleanup_arglist:
    LocalFree(szArgList);
    return nRetCode;
}
