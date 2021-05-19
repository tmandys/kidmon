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
#include <Windows.h>
#include "CKidMonBase.h"

class CKidMonWindows : public CKidMonBase {
public:
    static const DWORD DEFAULT_POLLINTERVAL = 5000; // msecs
    static const DWORD DEFAULT_HEARTBEAT = 15000; // msecs
    static const LPCSTR DEFAULT_DBFILENAME;

    CKidMonWindows(const char* dbFilename, unsigned int heartbeat);
    static std::string ExpandFileName(const char *filename);
protected:
    unsigned long GetActiveProcessId();
    std::string GetProcessName(unsigned long pid);
    unsigned long GetActivityTimeout();
    BOOL AdjustDirectory(std::string& filename);

private:
    char m_dbFilenameBuffer[MAX_PATH];
};
