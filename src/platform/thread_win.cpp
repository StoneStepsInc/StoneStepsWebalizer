/*
    webalizer - a web server log analysis program

    Copyright (c) 2004-2015, Stone Steps Inc. (www.stonesteps.ca)

    See COPYING and Copyright files for additional licensing and copyright information

    thread_win.cpp
*/
#include "../pch.h"

#include "../thread.h"
#include <process.h>
#include <windows.h>

struct thread_handle_t {
   HANDLE thread_handle;
   thread_handle_t(HANDLE thread_handle) : thread_handle(thread_handle) {}
};

thread_t thread_create(start_routine_t start_routine, void *arg)
{
	unsigned int threadid;
   uintptr_t thread_handle;
   
   thread_handle = _beginthreadex(NULL, 0, start_routine, arg, 0, &threadid);

   if(thread_handle == 0 || thread_handle == (uintptr_t) -1)
      return NULL;

   return new thread_handle_t((HANDLE) thread_handle);
}

void thread_destroy(thread_t thread)
{
	DWORD exitcode;

   if(thread) {
	   if(!GetExitCodeThread(thread->thread_handle, &exitcode) && exitcode == STILL_ACTIVE)
		   TerminateThread(thread->thread_handle, -1);

	   CloseHandle(thread->thread_handle);

      delete thread;
   }
}

void msleep(unsigned long timeout)
{
   Sleep(timeout);
}

uint64_t msecs(void)
{
   return GetTickCount();
}

unsigned long thread_id(void)
{
	return (unsigned long) GetCurrentThreadId();
}
