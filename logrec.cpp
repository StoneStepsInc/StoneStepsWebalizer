/*
   webalizer - a web server log analysis program

   Copyright (c) 2004-2014, Stone Steps Inc. (www.stonesteps.ca)

   See COPYING and Copyright files for additional licensing and copyright information 
   
   logrec.cpp
*/
#include "pch.h"

#include "logrec.h"
#include <memory.h>

log_struct::log_struct(void)
{
   reset();
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

void log_struct::normalize(u_int log_type)
{
   size_t index;

   //
   // if a proxy log, convert the scheme and domain to lower case
   //
   if(log_type == LOG_SQUID) {
      if(!url.compare_ci("http://", 7)) {
         if((index = url.find('/', 7)) != string_t::npos) 
            url.tolower(0, index);
      }
   }

   //
   // fix empty URL's
   //
   if(url.isempty() || url[0] == '-' && url[1] == 0)
      url = '/';

   //
   // fix empty search arguments
   //
   if(srchargs == "-")
      srchargs.reset();

   //
   // fix empty user agents
   //
   if(agent == "-")
      agent.reset();

   //
   // fix empty users
   //
   if(ident == "-")
      ident.reset();

   //
   // fix empty referrers
   //
   if(refer == "-") 
      refer.reset();
   else if(!refer.compare_ci("http://", 7)) {
      if((index = refer.find('/', 7)) != string_t::npos) 
         refer.tolower(0, index);
   }
   else if(!refer.compare_ci("https://", 8)) {
      if((index = refer.find('/', 8)) != string_t::npos) 
         refer.tolower(0, index);
   }

   //
   // catch blank and lower-case valid host names
   //
   if(hostname.isempty())
      hostname = "0.0.0.0";
   else
      hostname.tolower();
      
   // fix empty HTTP methods
   if(method == "-")
      method.reset();
}
