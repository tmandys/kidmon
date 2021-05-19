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
#include <psapi.h>
#include <shlwapi.h>
#include "CKidMonWindows.h"

const LPCSTR CKidMonWindows::DEFAULT_DBFILENAME = "%UserProfile%\\AppData\\LocalLow\\MandySoft\\KidMon\\KidMon_hourly_raw.csv";

CKidMonWindows::CKidMonWindows(const char* dbFilename, unsigned int heartbeat) : CKidMonBase(dbFilename, heartbeat) {
    m_dbFilename = ExpandFileName(dbFilename);
    AdjustDirectory(m_dbFilename);
};

BOOL CKidMonWindows::AdjustDirectory(std::string& filename) {
    // create directory if does not exist
    char path[MAX_PATH];
    while (TRUE) {
        strncpy(path, filename.c_str(), sizeof(path));
        if (!PathRemoveFileSpecA(path)) {
            return TRUE;
        }
        if (CreateDirectoryA(path, nullptr)) {
            return TRUE;
        }
        switch (GetLastError()) {
        case ERROR_ALREADY_EXISTS:
            return TRUE;
        case ERROR_PATH_NOT_FOUND:
            break;
        default:
            return FALSE;
        }
        while (GetLastError() == ERROR_PATH_NOT_FOUND) {
            if (!PathRemoveFileSpecA(path)) {
                return FALSE;
            }
            if (CreateDirectoryA(path, nullptr)) {
                break;
            }
            if (GetLastError() != ERROR_ALREADY_EXISTS) {
                return FALSE;
            }
        }
    }
}

unsigned long CKidMonWindows::GetActiveProcessId() {
    HWND hWnd = GetForegroundWindow();
    if (hWnd) {
        // get process id related to window
        DWORD dwProcessId;
        GetWindowThreadProcessId(hWnd, &dwProcessId);
        return dwProcessId;
    }
    else {
        return 0;
    }
}

std::string CKidMonWindows::GetProcessName(unsigned long pid) {
    CHAR cBuffer[MAX_PATH];
    HANDLE hProc = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);
    if (GetProcessImageFileNameA((HMODULE)hProc, cBuffer, MAX_PATH) != 0) {
        // find quickly last path delimiter to get filename
        PCHAR pcFilename = cBuffer;
        /*
        PCHAR p = cBuffer;
        while (*p != '\0') {
            if (*p == '\\') {
                p++;
                pcFilename = p;
            }
            else {
                p++;
            }
        }
        */
        return pcFilename;
    }
    else {
        return "";
    }
}

unsigned long CKidMonWindows::GetActivityTimeout() {
    LASTINPUTINFO info;
    info.cbSize = sizeof(info);
    if (GetLastInputInfo(&info)) {
        return GetTickCount() - info.dwTime;
    }
    else {
        return MAXDWORD;
    }
}

std::string CKidMonWindows::ExpandFileName(const char *filename) {
    char buffer[MAX_PATH];
    ExpandEnvironmentStringsA(filename, buffer, sizeof(buffer));
    std::string res(buffer);
    return res;
}
