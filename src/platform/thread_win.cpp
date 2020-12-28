/*
    webalizer - a web server log analysis program

    Copyright (c) 2004-2021, Stone Steps Inc. (www.stonesteps.ca)

    See COPYING and Copyright files for additional licensing and copyright information

    thread_win.cpp
*/
#include "../pch.h"

#include "../thread.h"
#include <windows.h>

unsigned long thread_id(void)
{
   return (unsigned long) GetCurrentThreadId();
}
