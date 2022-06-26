/*
   webalizer - a web server log analysis program

   Copyright (c) 2004-2022, Stone Steps Inc. (www.stonesteps.ca)

   See COPYING and Copyright files for additional licensing and copyright information 
   
   logrec.cpp
*/
#include "pch.h"

#include "logrec.h"

log_struct::log_struct(void) : resp_code(0), xfer_size(0), proc_time(0), port(0)
{
}

void log_struct::reset(void)
{
   hostname.reset();
   method.reset();
   url.reset();
   refer.reset();
   agent.reset();
   srchargs.reset();
   ident.reset();
   xsrchstr.reset();

   tstamp.reset();

   resp_code = 0;
   xfer_size = 0;
   proc_time = 0;
   port = 0;
}
