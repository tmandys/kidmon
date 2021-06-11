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

#include "CKidMonBase.h"
#include <stdarg.h>

CKidMonBase::CKidMonBase(const char* dbFilename, unsigned int heartbeat) : 
    m_dbFilename(dbFilename), 
    m_pidLast(0), 
    m_heartbeat(heartbeat), 
    m_utcLast(0),
    m_utcLastActivity(0)
{
    Reset();
}

CKidMonBase::~CKidMonBase() {
    //PrintCounters(stderr);
    Flush();
}

void CKidMonBase::Poll() {
    time_t utcTime;
    time(&utcTime);
    double diffColl = difftime(utcTime, m_utcStartPeriod);
    if (diffColl < 0) {
        // time went back before collection start time, i.e. reset collected data for current interval
        Reset();
    }
    // has the human made a input (click, move, ..) ?
    m_fActivity = GetActivityTimeout() <= m_heartbeat;
    unsigned long pid = GetActiveProcessId();
    if (pid != m_pidLast) {
        m_processName = GetProcessName(pid);
        if (!m_processName.empty()) {
            std::map<std::string, struct counter>::iterator iter = m_counters.find(m_processName);
            if (iter == m_counters.end()) {
                struct counter cnt;
                cnt.reset();
                m_counters[m_processName] = cnt;
                iter = m_counters.find(m_processName);
            }
            m_cntCurrent = &iter->second;
        }
        else {
            // ignored process
            m_cntCurrent = nullptr;
        }
        m_pidLast = pid;
    }
    if (m_utcLast) {
        // is not the first polling
        double diff = difftime(utcTime, m_utcLast);
        bool valid = diff <= m_heartbeat;
        // check if we can close period
        if (diffColl >= COLLECTION_SECS) {
            // end of period
            if (m_cntCurrent) {
                // add time to end of period
                diff = difftime(m_utcStartPeriod, m_utcLast) + COLLECTION_SECS;
                if (valid) {
                    // we will add time for period since last sample
                    m_cntCurrent->cnt[m_fActivity] += long(1000 * diff);
                }
                else {
                    m_cntUnknown.cnt[m_fActivity] += long(1000 * diff);
                }
            }
            Flush();
            Reset();
            // recalc diff since new period
            diff = difftime(utcTime, m_utcStartPeriod);
            if (diff > 0 && !m_processName.empty()) {
                // recreate current counter
                struct counter cnt;
                cnt.reset();
                m_counters[m_processName] = cnt;
                std::map<std::string, struct counter>::iterator iter = m_counters.find(m_processName);
                m_cntCurrent = &iter->second;
            }
        }
        if (m_cntCurrent && diff > 0) {
            // add time to end of period
            if (valid) {
                // we will add time for period since last sample
                m_cntCurrent->cnt[m_fActivity] += long(1000 * diff);
            }
            else {
                m_cntUnknown.cnt[m_fActivity] += long(1000 * diff);
            }
        }
    }
    m_utcLast = utcTime;
}

time_t CKidMonBase::CalcStartUTC() {
    // calculate UTC time for current hour in local time, note every TZ has only hours shift
    time_t utc;
    time(&utc);
    struct tm *local = localtime(&utc);
    local->tm_min = 0;
    local->tm_sec = 0;
    return mktime(local);
}

void CKidMonBase::Reset() {
    m_utcStartPeriod = CalcStartUTC();
    m_counters.clear();
    m_cntCurrent = nullptr;
    m_pidLast = 0;
    m_cntUnknown.reset();
}

void CKidMonBase::Flush() {
    if (m_counters.empty() && m_cntUnknown.empty()) {
        return;
    }
    FILE *f = fopen(m_dbFilename.c_str(), "a");
    if (f != nullptr) {
        PrintCounters(f);
        fclose(f);
    }
    else {
        LogErr("Cannot open '%s', errno: %d\n", m_dbFilename.c_str(), errno);
    }
}

void CKidMonBase::PrintCounters(FILE *f) {
    char stamp[20];
    struct tm *local = localtime(&m_utcStartPeriod);
    strftime(stamp, sizeof(stamp), FORMAT_STAMP, local);
    PrintCounter(f, stamp, NAME_UNKNOWN, m_cntUnknown);
    for (std::map<std::string, struct counter>::iterator iter = m_counters.begin(); iter != m_counters.end(); iter++) {
        PrintCounter(f, stamp, iter->first, iter->second);
    }
}

void CKidMonBase::PrintCounter(FILE *f, char* stamp, std::string processName, struct counter cnt) {
    if (!cnt.empty()) {
        fprintf(f, "%s;\"%s\";%d;%d\n", stamp, processName.c_str(), cnt.cnt[0], cnt.cnt[1]);
    }
}

void CKidMonBase::LogErr(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vLogErr(fmt, args);
    va_end(args);
}

void CKidMonBase::vLogErr(const char* fmt, va_list args) {
    vfprintf(stderr, fmt, args);
}
