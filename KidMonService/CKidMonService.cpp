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

#pragma region Includes
#include "CKidMonService.h"
#include "CKidMonWindows.h"

#include "ThreadPool.h"
#include <psapi.h>
#include <time.h>
#pragma endregion

CKidMonService::CKidMonService(PSTR pszServiceName,
    BOOL fCanStop,
    BOOL fCanShutdown,
    BOOL fCanPauseContinue)
    : CServiceBase(pszServiceName, fCanStop, fCanShutdown, fCanPauseContinue)
{
    m_fStopping = FALSE;
    m_fPaused = FALSE;

    // Create a manual-reset event that is not signaled at first to indicate 
    // the stopped signal of the service.
    m_hStoppedEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (m_hStoppedEvent == NULL)
    {
        throw GetLastError();
    }
}


CKidMonService::~CKidMonService(void)
{
    if (m_hStoppedEvent)
    {
        CloseHandle(m_hStoppedEvent);
        m_hStoppedEvent = NULL;
    }
}


//
//   FUNCTION: CKidMonService::OnStart(DWORD, LPSTR *)
//
//   PURPOSE: The function is executed when a Start command is sent to the 
//   service by the SCM or when the operating system starts (for a service 
//   that starts automatically). It specifies actions to take when the 
//   service starts. In this code sample, OnStart logs a service-start 
//   message to the Application log, and queues the main service function for 
//   execution in a thread pool worker thread.
//
//   PARAMETERS:
//   * dwArgc   - number of command line arguments
//   * lpszArgv - array of command line arguments
//
//   NOTE: A service application is designed to be long running. Therefore, 
//   it usually polls or monitors something in the system. The monitoring is 
//   set up in the OnStart method. However, OnStart does not actually do the 
//   monitoring. The OnStart method must return to the operating system after 
//   the service's operation has begun. It must not loop forever or block. To 
//   set up a simple monitoring mechanism, one general solution is to create 
//   a timer in OnStart. The timer would then raise events in your code 
//   periodically, at which time your service could do its monitoring. The 
//   other solution is to spawn a new thread to perform the main service 
//   functions, which is demonstrated in this code sample.
//
void CKidMonService::OnStart(DWORD dwArgc, LPSTR *lpszArgv)
{
    // Log a service start message to the Application log.
    WriteEventLogEntry(EVENTLOG_INFORMATION_TYPE, "KidMon Service in OnStart");

    // Queue the main service function for execution in a worker thread.
    CThreadPool::QueueUserWorkItem(&CKidMonService::ServiceWorkerThread, this);
}


//
//   FUNCTION: CKidMonService::ServiceWorkerThread(void)
//
//   PURPOSE: The method performs the main function of the service. It runs 
//   on a thread pool worker thread.
//
void CKidMonService::ServiceWorkerThread(void)
{
    char *redirectFilename = "C:\\temp\\stderr.log";
    FILE* stderrRedirect = freopen(redirectFilename, "w", stderr);
    if (!stderrRedirect) {
        WriteEventLogEntry(EVENTLOG_ERROR_TYPE, "Cannot redirect stderr to '%S', errno: %d", redirectFilename, errno);
    }
    std::string iniFile = CKidMonWindows::ExpandFileName(KidMon::INI_DEFAULT_FILENAME);
    ReadConfiguration(iniFile.c_str());
    {
        CKidMonWindowsEventLog mon(m_cDbFilename, m_dwHeartbeat, *this);
        WriteEventLogEntry(EVENTLOG_INFORMATION_TYPE, "KidMon Service Worker thread [%S, heartbeat: %u, pollinterval: %u]", mon.GetDbFilename().c_str(), mon.GetHeartbeat(), m_dwPollInterval);

        while (!m_fStopping)
        {
            //WriteEventLogEntry(EVENTLOG_INFORMATION_TYPE, "Poll");
            if (!m_fPaused) {
                mon.LogErr("calling poll\n");
                mon.Poll();
            }
            if (!m_fStopping) {
                ::Sleep(m_dwPollInterval);
            }
        }
        WriteEventLogEntry(EVENTLOG_INFORMATION_TYPE, "Poll finished");
        mon.Flush();
    }

    // Signal the stopped event.
    SetEvent(m_hStoppedEvent);
    if (stderrRedirect) {
        fclose(stderrRedirect);
    }
}


//
//   FUNCTION: CKidMonService::OnStop(void)
//
//   PURPOSE: The function is executed when a Stop command is sent to the 
//   service by SCM. It specifies actions to take when a service stops 
//   running. In this code sample, OnStop logs a service-stop message to the 
//   Application log, and waits for the finish of the main service function.
//
//   COMMENTS:
//   Be sure to periodically call ReportServiceStatus() with 
//   SERVICE_STOP_PENDING if the procedure is going to take long time. 
//
void CKidMonService::OnStop()
{
    // Log a service stop message to the Application log.
    WriteEventLogEntry(EVENTLOG_INFORMATION_TYPE, "KidMon Service in OnStop");

    // Indicate that the service is stopping and wait for the finish of the 
    // main service function (ServiceWorkerThread).
    m_fStopping = TRUE;
    if (WaitForSingleObject(m_hStoppedEvent, INFINITE) != WAIT_OBJECT_0)
    {
        throw GetLastError();
    }
}

void CKidMonService::OnPause()
{
    // Log a service stop message to the Application log.
    WriteEventLogEntry(EVENTLOG_INFORMATION_TYPE, "KidMon Service in OnPause");
    m_fPaused = true;
}

void CKidMonService::OnContinue()
{
    // Log a service stop message to the Application log.
    WriteEventLogEntry(EVENTLOG_INFORMATION_TYPE, "KidMon Service in OnContinue");
    m_fPaused = false;
}

void CKidMonService::ReadConfiguration(LPCSTR lpIniFilename) {
    using namespace KidMon;
    m_dwPollInterval = GetPrivateProfileIntA(INI_SECTION_GENERAL, INI_KEY_POLLINTERVAL, CKidMonWindows::DEFAULT_POLLINTERVAL, lpIniFilename);
    GetPrivateProfileStringA(INI_SECTION_GENERAL, INI_KEY_DBFILENAME, CKidMonWindows::DEFAULT_DBFILENAME, m_cDbFilename, sizeof(m_cDbFilename), lpIniFilename);
    m_dwHeartbeat = GetPrivateProfileIntA(INI_SECTION_GENERAL, INI_KEY_HEARTBEAT, CKidMonWindows::DEFAULT_HEARTBEAT, lpIniFilename);
}

void CKidMonWindowsEventLog::vLogErr(const char* fmt, va_list args) {
    m_svc.vWriteEventLogEntry(EVENTLOG_ERROR_TYPE, fmt, args);
}

