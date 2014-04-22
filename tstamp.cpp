/*
    webalizer - a web server log analysis program

    Copyright (c) 2004-2013, Stone Steps Inc. (www.stonesteps.ca)

    See COPYING and Copyright files for additional licensing and copyright information

    tstamp.cpp
*/
#include "pch.h"

#include "tstamp.h"
#include "util.h"
#include <ctype.h>

tstamp_t::tstamp_t(int y, int mo, int d, int h, int mi, int s)
{
   reset(y, mo, d, h, mi, s);
}

void tstamp_t::reset(int y, int mo, int d, int h, int mi, int s)
{
   year = y;
   month = mo;
   day = d;
   hour = h;
   min = mi;
   sec = s;
}

void tstamp_t::reset(time_t time)
{
   u_long a, b, c, d, e, m, t;

   time -= (t = time % 60ul);
   sec = t;

   time -= (t = time % 3600ul);
   min = t / 60ul;

   time -= (t = time % 86400ul);
   hour = t / 3600ul;

   time = time / 86400ul + 2440588ul;

   a = time + 32044;
   b = (4*a+3)/146097;
   c = a - (b*146097)/4;
   d = (4*c+3)/1461;
   e = c - (1461*d)/4;
   m = (5*e+2)/153;

   year = b*100 + d - 4800 + m/10;
   month = m + 3 - 12*(m/10);
   day = e - (153*m+2)/5 + 1;
}

void tstamp_t::reset(const char *str)
{
   u_int num;
   const char *cp1, *cp2;
   u_int ti = 0, di = 0, dp[3], tp[3];
   bool time = false;

   reset();

   cp1 = str;

   // 2007-11-17 12:30:01
   // 2007-11-17
   //      11-17 12:30
   //            12:30:01
   while(*cp1) {
      // get the longest number and update the arrays
      num = str2ul(cp1, &cp2);
      
      // $$$ change reset to return bool !!!
      if(!cp2)
         return;

      if(*cp2 == ':') {
         // part of time
         tp[ti++] = num;
         time = true;
         cp1 = cp2+1;
      }
      else if(*cp2 == '/' || *cp2 == '-') {
         // part of date
         dp[di++] = num;
         time = false;
         cp1 = cp2+1;
      }
      else if(*cp2 == ' ') {
         // if it's a trailing space after time, we are done
         if(time) {
            tp[ti++] = num;
            break;
         }
         
         // otherwise, finish the date
         time = true;
         dp[di++] = num;
         cp1 = cp2+1;
      }
      else {
         // a single number is ambiguous - return
         if(!di && !ti) return;

         // complete the current part and break out
         if(time)
            tp[ti++] = num;
         else
            dp[di++] = num;
         break;
      }
   }

   if(di) {
      // use the date parts to populate the date (day, month, year)
      day = dp[--di];
      if(di) month = dp[--di];
      if(di) year = dp[--di];
   }

   if(ti) {
      // use the time parts to populate the time (hour, minute, second)
      hour = tp[0]; ti--;
      if(ti) {min = tp[1]; ti--;}
      if(ti) {sec = tp[2]; ti--;}
   }
}

int tstamp_t::compare(const tstamp_t& tstamp) const
{
   int diff;

   // compare part by part for performance reasons
   if((diff = year - tstamp.year) != 0)
      return diff;

   if((diff = month - tstamp.month) != 0)
      return diff;

   if((diff = day - tstamp.day) != 0)
      return diff;

   if((diff = hour - tstamp.hour) != 0)
      return diff;

   if((diff = min - tstamp.min) != 0)
      return diff;

   if((diff = sec - tstamp.sec) != 0)
      return diff;

   return 0;
}

//
// Returns the number of seconds since midnight 1/1/1970 (2440588)
//
time_t tstamp_t::mktime(int year, int month, int day, int hour, int min, int sec)
{
   return ((jdate(year, month, day)-2440588ul)*86400ul) + hour*3600ul + min*60ul + sec;
}

//
// Returns the number of days from noon UTC, January 1st, 4713 B.C.E. (Julian day number)
//
u_long tstamp_t::jdate(int year, int month, int day)
{
   u_long a, y, m;
   a = (14-month)/12;
   y = year+4800-a;
   m = month + 12*a - 3;
   return day + (153*m+2)/5 + y*365 + y/4 - y/100 + y/400 - 32045ul;
}
