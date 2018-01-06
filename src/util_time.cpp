/*
   webalizer - a web server log analysis program

   Copyright (c) 2004-2018, Stone Steps Inc. (www.stonesteps.ca)
   Copyright (C) 1997-2001  Bradford L. Barrett (brad@mrunix.net)

   See COPYING and Copyright files for additional licensing and copyright information 
   
   util_time.cpp
*/
#include "pch.h"

#include "util_time.h"
#include "unicode.h"

#include <ctime>

uint64_t usec2msec(uint64_t usec)
{
   return (usec / 1000) + ((usec % 1000 >= 500) ? 1 : 0);
}

/*********************************************/
/* CUR_TIME - return date/time as a string   */
/*********************************************/

string_t cur_time(bool local_time)
{
   size_t slen;
   wchar_t wctstamp[256];
   char    timestamp[256];
   time_t  now;

   /* get system time */
   now = time(NULL);
   
   // format time stamp for the current locale
   if (local_time)
      slen = wcsftime(wctstamp, sizeof(wctstamp)/sizeof(wchar_t), L"%d-%b-%Y %H:%M %Z", localtime(&now));
   else
      slen = wcsftime(wctstamp, sizeof(wctstamp)/sizeof(wchar_t), L"%d-%b-%Y %H:%M UTC", gmtime(&now));
   
   // convert the wide-character string to UTF-8
   slen = ucs2utf8(wctstamp, slen, timestamp, sizeof(timestamp));

   return string_t(timestamp, slen);
}

uint64_t elapsed(uint64_t stime, uint64_t etime)
{
   return stime < etime ? etime-stime : (~stime+1)+etime;
}
