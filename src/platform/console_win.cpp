/*
    webalizer - a web server log analysis program

    Copyright (c) 2004-2018, Stone Steps Inc. (www.stonesteps.ca)

    See COPYING and Copyright files for additional licensing and copyright information

    console_win.cpp
*/
#include "../pch.h"

#include <windows.h>

static void (*ctrl_c_handler)(void);

static BOOL WINAPI console_ctrl_c_handler(DWORD type)
{
   switch(type) {
      case CTRL_C_EVENT:
      case CTRL_BREAK_EVENT:
      case CTRL_CLOSE_EVENT:
      case CTRL_LOGOFF_EVENT:
      case CTRL_SHUTDOWN_EVENT:
         if(ctrl_c_handler)
            ctrl_c_handler();
         return TRUE;
   }

   return FALSE;
}

void set_ctrl_c_handler(void (*ctrl_c_handler)(void))
{
   // set or replace the handler callback
   ::ctrl_c_handler = ctrl_c_handler;

   // register the handler only when called the first time or after a reset
   if(!::ctrl_c_handler)
      SetConsoleCtrlHandler(console_ctrl_c_handler, TRUE);   
}

void reset_ctrl_c_handler(void)
{
   // remove the handler if we set it before
   if(ctrl_c_handler) {
      SetConsoleCtrlHandler(NULL, FALSE);
      ctrl_c_handler = NULL;
   }
}
