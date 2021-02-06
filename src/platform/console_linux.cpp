/*
    webalizer - a web server log analysis program

    Copyright (c) 2004-2021, Stone Steps Inc. (www.stonesteps.ca)

    See COPYING and Copyright files for additional licensing and copyright information

    console_linux.cpp
*/
#include "../pch.h"

#include <signal.h>
#include <unistd.h>

static void (*ctrl_c_handler)(void);

static void console_ctrl_c_handler(int sig)
{
   if(sig == SIGINT) {
      if(ctrl_c_handler)
         ctrl_c_handler();
   }
}

void set_ctrl_c_handler(void (*ctrl_c_handler)(void))
{
   // register the handler only when called the first time or after a reset
   if(!::ctrl_c_handler) {
      ::ctrl_c_handler = ctrl_c_handler;

      struct sigaction sa = {};
      sa.sa_handler = console_ctrl_c_handler;
      sigaction(SIGINT, &sa, NULL);
   }
}

void reset_ctrl_c_handler(void)
{
   // reset the handler to the default value if it was set before
   if(ctrl_c_handler) {
      struct sigaction sa = {};
      sa.sa_handler = SIG_DFL;
      sigaction(SIGINT, &sa, NULL);

      ctrl_c_handler = NULL;
   }
}
