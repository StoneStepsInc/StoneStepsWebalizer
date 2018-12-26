/*
    webalizer - a web server log analysis program

    Copyright (c) 2004-2019, Stone Steps Inc. (www.stonesteps.ca)

    See COPYING and Copyright files for additional licensing and copyright information

    thread_win.cpp
*/
#include "../pch.h"

#include "../thread.h"
#include <process.h>
#include <windows.h>

void msleep(unsigned long timeout)
{
   Sleep(timeout);
}

uint64_t msecs(void)
{
   return (uint64_t) GetTickCount();
}

unsigned long thread_id(void)
{
   return (unsigned long) GetCurrentThreadId();
}
