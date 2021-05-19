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

#pragma once

#include "ServiceBase.h"
#include "CKidMonWindows.h"

namespace KidMon {
    // https://docs.microsoft.com/en-us/windows/win32/shell/knownfolderid

    constexpr LPCSTR INI_DEFAULT_FILENAME = "%ProgramData%\\MandySoft\\KidMon\\KidMon.ini";
    constexpr LPCSTR INI_SECTION_GENERAL = "General";
    constexpr LPCSTR INI_KEY_POLLINTERVAL = "PollInterval";
    constexpr LPCSTR INI_KEY_HEARTBEAT = "Heartbeat";
    constexpr LPCSTR INI_KEY_DBFILENAME = "DbFilename";
}

class CKidMonWindowsEventLog;

class CKidMonService : public CServiceBase
{
public:

    CKidMonService(PSTR pszServiceName,
        BOOL fCanStop = TRUE,
        BOOL fCanShutdown = TRUE,
        BOOL fCanPauseContinue = TRUE);
    virtual ~CKidMonService(void);

protected:

    virtual void OnStart(DWORD dwArgc, PSTR *pszArgv);
    virtual void OnStop();
    virtual void OnPause();
    virtual void OnContinue();

    void ServiceWorkerThread(void);
    void ReadConfiguration(LPCSTR lpIniFilename);
    friend CKidMonWindowsEventLog;
private:

    BOOL m_fStopping;
    BOOL m_fPaused;
    HANDLE m_hStoppedEvent;
    DWORD m_dwPollInterval; // ms
    DWORD m_dwHeartbeat; // ms
    CHAR m_cDbFilename[MAX_PATH];
};

class CKidMonWindowsEventLog : public CKidMonWindows {
public:
    CKidMonWindowsEventLog(const char* dbFilename, unsigned int heartbeat, CKidMonService& svc) : CKidMonWindows(dbFilename, heartbeat), m_svc(svc) { };
    virtual void vLogErr(const char* fmt, va_list args);
protected:
private:
    CKidMonService &m_svc;
};
