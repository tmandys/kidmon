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
#include <map>
#include <time.h>

#ifdef _WINDOWS_
#define KIDMON_DATA_DIR "%UserProfile%\\AppData\\LocalLow\\MandySoft\\KidMon\\"
#else
#define KIDMON_DATA_DIR "~/.kidmon/"
#endif
#define KIDMON_HOURLY_RAW_FILENAME "kidmon_hourly_raw.csv"
#define KIDMON_HOURLY_FILENAME "kidmon_hourly.csv"
#define KIDMON_DAILY_FILENAME "kidmon_daily.csv"
#define KIDMON_WEEKLY_FILENAME "kidmon_weekly.csv"
#define KIDMON_MONTHLY_FILENAME "kidmon_monthly.csv"
/*
  Multiplatform base class. Platform dependent stuff goes to child class
  We won't use wchar_t for filename as platform independent functions fopen(), stat(), etc. do not support wide chars.
*/
struct counter {
    unsigned long cnt[2];
    void reset() {
        cnt[0] = 0;
        cnt[1] = 0;
    }
    bool empty() {
        return cnt[0] == 0 && cnt[1] == 0;
    }
};

class CKidMonBase
{
public:
    const char* NAME_UNKNOWN = "_UNKNOWN_";
    const char* FORMAT_STAMP = "%F %R";
    CKidMonBase(const char* dbFilename, unsigned int heartbeat);
    virtual ~CKidMonBase(void);

    void Poll();
    std::string& GetProcessNameStr() { return m_processName; };
    bool GetActivity() { return m_fActivity; };
    std::string& GetDbFilename() { return m_dbFilename; };
    unsigned int GetHeartbeat() { return m_heartbeat; };
    void LogErr(const char* fmt, ...);
    void Flush();
protected:
    virtual unsigned long GetActiveProcessId() = 0;  // return True if the process is the same as in the last call
    virtual std::string GetProcessName(unsigned long pid) = 0;  // non reentrant function
    virtual unsigned long GetActivityTimeout() = 0;  // ms since last human activity
    void Reset();
    time_t CalcStartUTC();

    std::string m_dbFilename;
    virtual void vLogErr(const char* fmt, va_list args);
private:
    const unsigned int COLLECTION_SECS = 60 * 60;
    unsigned long m_pidLast;
    unsigned int m_heartbeat;  // secs, sample data are valid if polling if more often than validity timeout
    time_t m_utcLast;
    time_t m_utcLastActivity;
    time_t m_utcStartPeriod;
    std::map<std::string, struct counter> m_counters;
    std::string m_processName;
    struct counter* m_cntCurrent;
    struct counter m_cntUnknown;  // 0.. active, 1..inactive
    bool m_fActivity;

    void PrintCounters(FILE *f);
    void PrintCounter(FILE *f, char* stamp, std::string processName, struct counter cnt);
};


