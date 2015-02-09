/*
    webalizer - a web server log analysis program

    Copyright (c) 2004-2015, Stone Steps Inc. (www.stonesteps.ca)

    See COPYING and Copyright files for additional licensing and copyright information

    mutex_win.cpp
*/
#include "pch.h"

#include "mutex.h"

mutex_t mutex_create(void)
{
	CRITICAL_SECTION *mutex = new CRITICAL_SECTION;
	InitializeCriticalSection(mutex);
	return mutex;
}

void mutex_destroy(mutex_t mutex)
{
	if(mutex) {
		DeleteCriticalSection(mutex);
		delete mutex;
	}
}

void mutex_lock(mutex_t mutex)
{
   if(mutex)
	   EnterCriticalSection(mutex);
}

void mutex_unlock(mutex_t mutex)
{
   if(mutex)
	   LeaveCriticalSection(mutex);
}
