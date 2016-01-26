/*
    webalizer - a web server log analysis program

    Copyright (c) 2004-2016, Stone Steps Inc. (www.stonesteps.ca)

    See COPYING and Copyright files for additional licensing and copyright information

    mutex_win.cpp
*/
#include "../pch.h"

#include "../mutex.h"

#include <windows.h>

struct mutex_handle_t {
   CRITICAL_SECTION  crit_sect;
};

mutex_t mutex_create(void)
{
	mutex_t mutex = new mutex_handle_t;
	InitializeCriticalSection(&mutex->crit_sect);
	return mutex;
}

void mutex_destroy(mutex_t mutex)
{
	if(mutex) {
		DeleteCriticalSection(&mutex->crit_sect);
		delete mutex;
	}
}

void mutex_lock(mutex_t mutex)
{
   if(mutex)
	   EnterCriticalSection(&mutex->crit_sect);
}

void mutex_unlock(mutex_t mutex)
{
   if(mutex)
	   LeaveCriticalSection(&mutex->crit_sect);
}
