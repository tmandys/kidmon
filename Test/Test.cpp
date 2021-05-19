// #include "stdafx.h"
#include <windows.h>
#include <tlhelp32.h>
#include <tchar.h>
#include <time.h>
#include <psapi.h>
#include <Lmcons.h>
#include <string>
#include <iostream>
#include <algorithm>


#ifdef _DEBUG
#define new DEBUG_NEW
#endif

BOOL GetProcessList();
BOOL ListProcessModules(DWORD dwPID);
BOOL ListProcessThreads(DWORD dwOwnerPID);
void printError(TCHAR* msg);

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
    _tprintf(TEXT("\n  WARNING: %s failed with error %d (%s)\n"), msg, eNum, sysMsg);
}

BOOL GetProcessList()
{
    HANDLE hProcessSnap;
    HANDLE hProcess;
    PROCESSENTRY32 pe32;
    DWORD dwPriorityClass;

    // Take a snapshot of all processes in the system.
    hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hProcessSnap == INVALID_HANDLE_VALUE)
    {
        printError(TEXT("CreateToolhelp32Snapshot (of processes)"));
        return(FALSE);
    }

    // Set the size of the structure before using it.
    pe32.dwSize = sizeof(PROCESSENTRY32);

    // Retrieve information about the first process,
    // and exit if unsuccessful
    if (!Process32First(hProcessSnap, &pe32))
    {
        printError(TEXT("Process32First")); // show cause of failure
        CloseHandle(hProcessSnap);          // clean the snapshot object
        return(FALSE);
    }

    // Now walk the snapshot of processes, and
    // display information about each process in turn
    do
    {
        _tprintf(TEXT("\n\n====================================================="));
        _tprintf(TEXT("\nPROCESS NAME:  %s"), pe32.szExeFile);
        _tprintf(TEXT("\n-------------------------------------------------------"));

        // Retrieve the priority class.
        dwPriorityClass = 0;
        hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pe32.th32ProcessID);
        if (hProcess == NULL)
            printError(TEXT("OpenProcess"));
        else
        {
            dwPriorityClass = GetPriorityClass(hProcess);
            if (!dwPriorityClass)
                printError(TEXT("GetPriorityClass"));
            CloseHandle(hProcess);
        }

        _tprintf(TEXT("\n  Process ID        = 0x%08X"), pe32.th32ProcessID);
        _tprintf(TEXT("\n  Thread count      = %d"), pe32.cntThreads);
        _tprintf(TEXT("\n  Parent process ID = 0x%08X"), pe32.th32ParentProcessID);
        _tprintf(TEXT("\n  Priority base     = %d"), pe32.pcPriClassBase);
        if (dwPriorityClass)
            _tprintf(TEXT("\n  Priority class    = %d"), dwPriorityClass);

        // List the modules and threads associated with this process
        //ListProcessModules(pe32.th32ProcessID);		
        //ListProcessThreads(pe32.th32ProcessID);

    } while (Process32Next(hProcessSnap, &pe32));

    CloseHandle(hProcessSnap);
    return(TRUE);
}


BOOL ListProcessModules(DWORD dwPID)
{
    HANDLE hModuleSnap = INVALID_HANDLE_VALUE;
    MODULEENTRY32 me32;

    // Take a snapshot of all modules in the specified process.
    hModuleSnap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, dwPID);
    if (hModuleSnap == INVALID_HANDLE_VALUE)
    {
        printError(TEXT("CreateToolhelp32Snapshot (of modules)"));
        return(FALSE);
    }

    // Set the size of the structure before using it.
    me32.dwSize = sizeof(MODULEENTRY32);

    // Retrieve information about the first module,
    // and exit if unsuccessful
    if (!Module32First(hModuleSnap, &me32))
    {
        printError(TEXT("Module32First"));  // show cause of failure
        CloseHandle(hModuleSnap);           // clean the snapshot object
        return(FALSE);
    }

    // Now walk the module list of the process,
    // and display information about each module
    do
    {
        _tprintf(TEXT("\n\n     MODULE NAME:     %s"), me32.szModule);
        _tprintf(TEXT("\n     Executable     = %s"), me32.szExePath);
        _tprintf(TEXT("\n     Process ID     = 0x%08X"), me32.th32ProcessID);
        _tprintf(TEXT("\n     Ref count (g)  = 0x%04X"), me32.GlblcntUsage);
        _tprintf(TEXT("\n     Ref count (p)  = 0x%04X"), me32.ProccntUsage);
        _tprintf(TEXT("\n     Base address   = 0x%08X"), (DWORD)me32.modBaseAddr);
        _tprintf(TEXT("\n     Base size      = %d"), me32.modBaseSize);

    } while (Module32Next(hModuleSnap, &me32));

    CloseHandle(hModuleSnap);
    return(TRUE);
}

BOOL ListProcessThreads(DWORD dwOwnerPID)
{
    HANDLE hThreadSnap = INVALID_HANDLE_VALUE;
    THREADENTRY32 te32;

    // Take a snapshot of all running threads  
    hThreadSnap = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
    if (hThreadSnap == INVALID_HANDLE_VALUE)
        return(FALSE);

    // Fill in the size of the structure before using it. 
    te32.dwSize = sizeof(THREADENTRY32);

    // Retrieve information about the first thread,
    // and exit if unsuccessful
    if (!Thread32First(hThreadSnap, &te32))
    {
        printError(TEXT("Thread32First")); // show cause of failure
        CloseHandle(hThreadSnap);          // clean the snapshot object
        return(FALSE);
    }

    // Now walk the thread list of the system,
    // and display information about each thread
    // associated with the specified process
    do
    {
        if (te32.th32OwnerProcessID == dwOwnerPID)
        {
            _tprintf(TEXT("\n\n     THREAD ID      = 0x%08X"), te32.th32ThreadID);
            _tprintf(TEXT("\n     Base priority  = %d"), te32.tpBasePri);
            _tprintf(TEXT("\n     Delta priority = %d"), te32.tpDeltaPri);
            _tprintf(TEXT("\n"));
        }
    } while (Thread32Next(hThreadSnap, &te32));

    CloseHandle(hThreadSnap);
    return(TRUE);
}

void printWindow(HWND hWnd, char* txt) {
    WCHAR buffer[MAX_PATH];
    GetWindowTextW(hWnd, buffer, sizeof(buffer) / sizeof(buffer[0]));
    DWORD processId;
    GetWindowThreadProcessId(hWnd, &processId);
    wprintf(L"%S: %d: pid: %d\n%s\n", txt, (long int)hWnd, processId, buffer);
    HANDLE hProc = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, processId);
    if (GetModuleFileNameW((HMODULE)hProc, buffer, MAX_PATH) == 0) {
        printError(TEXT("GetModuleFileNameW"));
    }
    else {
        wprintf(L"fn: %s\n", buffer);
    }
    if (GetModuleBaseNameW((HMODULE)hProc, NULL, buffer, MAX_PATH) == 0) {
        printError(TEXT("GetModuleBaseNameW"));
    }
    else {
        wprintf(L"bn: %s\n", buffer);
    }
    if (GetProcessImageFileNameW((HMODULE)hProc, buffer, MAX_PATH) == 0) {
        printError(TEXT("GetProcessImageFileNameW"));
    }
    else {
        // find quickly last path delimiter to get filename
        PWCHAR p = buffer;
        PWCHAR fn = p;
        while (*p != '\0') {
            if (*p == '\\') {
                p++;
                fn = p;
            }
            else {
                p++;
            }
        }
        wprintf(L"ifn: %s: %s\n", buffer, fn);
    }
    CloseHandle(hProc);
    wprintf(L"--------------\n");
    /* does not work for foreign windows
    UINT n = GetWindowModuleFileNameW(hWnd, buffer, sizeof(buffer) / sizeof(buffer[0]));
    if (n > 0) {
    wprintf(L"%d: fn: %s\n", (long int)hWnd, buffer);
    } */
}

// list windows
static BOOL CALLBACK enumWindowCallback(HWND hWnd, LPARAM lparam) {
    int length = GetWindowTextLength(hWnd);
    // List visible windows with a non-empty title
    if (IsWindowVisible(hWnd) && length != 0) {
        printWindow(hWnd, "Enum");
    }
    return TRUE;
}

void listWindows() {


    std::cout << "Enmumerating windows..." << std::endl;
    EnumWindows(enumWindowCallback, NULL);
    std::cout << "Done..." << std::endl;

    printWindow(GetForegroundWindow(), "Foreground");
    printWindow(GetActiveWindow(), "Active");
    //std::cin.ignore();
}

// idle detection
void detectIdle() {
    LASTINPUTINFO info;
    info.cbSize = sizeof(info);
    if (GetLastInputInfo(&info)) {
        wprintf(L"Idle time: %d\n", (GetTickCount() - info.dwTime));
    }
}

void testTime() {
    time_t start, finish;
    long product;

    time(&start);
    for (int i = 0; i<10000; i++)
    {
        for (int j = 0; j<100000; j++)
        {
            product = i*j;
        }
    }
    time(&finish);
    cout << "Time required = " << difftime(finish, start) << " seconds";
    cout << "Time required = " << difftime(start, finish) << " seconds";
}

char* getCmdOption(char ** begin, char ** end, const std::string & option)
{
    char ** itr = std::find(begin, end, option);
    if (itr != end && ++itr != end)
    {
        return *itr;
    }
    return 0;
}

bool cmdOptionExists(char** begin, char** end, const std::string& option)
{
    return std::find(begin, end, option) != end;
}

int main(int argc, char * argv[])
{
    int nRetCode = 0;
    HMODULE hModule = ::GetModuleHandle(nullptr);

    if (hModule != nullptr)
    {
        // initialize MFC and print and error on failure
        /*        if (!AfxWinInit(hModule, nullptr, ::GetCommandLine(), 0))
        {
        // TODO: change error code to suit your needs
        wprintf(L"Fatal Error: MFC initialization failed\n");
        nRetCode = 1;
        }
        else*/
        {
            // TODO: code your application's behavior here.
            //GetProcessList();
            listWindows();
            detectIdle();
            testTime();
        }
    }
    else
    {
        // TODO: change error code to suit your needs
        wprintf(L"Fatal Error: GetModuleHandle failed\n");
        nRetCode = 1;
    }

    return nRetCode;
}


