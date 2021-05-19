/* Using the thread pool is simple and feels natural in C++.
*
* class CSampleService
* {
    *public:
        *
            *     void AsyncRun()
            * {
            *CThreadPool::QueueUserWorkItem(&Service::Run, this);
            *
        }
        *
            *     void Run()
            * {
            *         // Some lengthy operation
                *
        }
        *
};
*
* Kenny Kerr spends most of his time designing and building distributed
* applications for the Microsoft Windows platform.He also has a particular
* passion for C++ and security programming.Reach Kenny at
* http://weblogs.asp.net/kennykerr/ or visit his Web site: 
*http ://www.kennyandkarin.com/Kenny/.
*/

#pragma once

#include <memory>

class CThreadPool
{
public:

    template <typename T>
    static void QueueUserWorkItem(void (T::*function)(void),
        T *object, ULONG flags = WT_EXECUTELONGFUNCTION)
    {
        typedef std::pair<void (T::*)(), T *> CallbackType;
        std::auto_ptr<CallbackType> p(new CallbackType(function, object));

        if (::QueueUserWorkItem(ThreadProc<T>, p.get(), flags))
        {
            // The ThreadProc now has the responsibility of deleting the pair.
            p.release();
        }
        else
        {
            throw GetLastError();
        }
    }

private:

    template <typename T>
    static DWORD WINAPI ThreadProc(PVOID context)
    {
        typedef std::pair<void (T::*)(), T *> CallbackType;

        std::auto_ptr<CallbackType> p(static_cast<CallbackType *>(context));

        (p->second->*p->first)();
        return 0;
    }
};