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
#include <string>
#include <map>
#include <vector>
#include <fstream>
#include <sstream>

#define KIDMON_VERSION "0.2"

static int fVerbose = 0;

void debugf(const char* fmt, ...) {
    if (!fVerbose) {
        return;
    }
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
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

int getCmdOptionInt(char ** begin, char ** end, const std::string & option, int defValue) {
    char *c = getCmdOption(begin, end, option);
    if (c) {
        try {
            return std::stoi(c);
        }
        catch (...) {
            return defValue;
        }
    }
    else {
        return defValue;
    }
}

bool fileExists(std::string filepath) {
    //std::ifstream stream;
    std::ifstream f(filepath);
    return f.good();
}

int getBasenameIdx(std::string filepath) {
    int idx = filepath.find_last_of("/\\");
    if (idx == std::string::npos) {
        idx = -1;
    }
    return idx + 1;
}

std::string expandFileName(const char *filename) {
#ifdef _WINDOWS_
    char buffer[MAX_PATH];
    ExpandEnvironmentStringsA(filename, buffer, sizeof(buffer));
    std::string res(buffer);
#else
    std::string res(filename);
#endif
    return res;
}

std::string readFile(std::string filename) {
    std::ifstream ifs(filename.c_str(), std::ios::in | std::ios::binary | std::ios::ate);
    std::ifstream::pos_type fileSize = ifs.tellg();
    ifs.seekg(0, std::ios::beg);

    std::vector<char> bytes((unsigned)fileSize);
    ifs.read(&bytes[0], fileSize);

    return std::string(&bytes[0], (unsigned)fileSize);
}

void writeFile(std::string filename, std::string &content) {
    std::ofstream ofs(filename.c_str(), std::ios::out | std::ios::binary | std::ios::trunc);
    ofs << content;
}

template <typename T, std::size_t N>
constexpr std::size_t countof(T const (&)[N]) noexcept {
    return N;
}
