/*
    webalizer - a web server log analysis program

    Copyright (c) 2004-2013, Stone Steps Inc. (www.stonesteps.ca)

    See COPYING and Copyright files for additional licensing and copyright information

    thread_win.cpp
*/
#include "pch.h"

#include <process.h>
#include "thread.h"

thread_t thread_create(start_routine_t start_routine, void *arg)
{
	unsigned threadid;

	return (thread_t) _beginthreadex(NULL, 0, start_routine, arg, 0, &threadid);
}

void thread_destroy(thread_t thread)
{
	DWORD exitcode;

   if(thread != (thread_t) -1) {
	   if(!GetExitCodeThread(thread, &exitcode) && exitcode == STILL_ACTIVE)
		   TerminateThread(thread, -1);

	   CloseHandle(thread);
   }
}

void msleep(unsigned long timeout)
{
   Sleep(timeout);
}

u_long msecs(void)
{
   return GetTickCount();
}

unsigned long thread_id(void)
{
	return (unsigned long) GetCurrentThreadId();
}
