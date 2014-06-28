/*
    webalizer - a web server log analysis program

    Copyright (c) 2004-2014, Stone Steps Inc. (www.stonesteps.ca)

    See COPYING and Copyright files for additional licensing and copyright information

    tstamp.h
*/

#ifndef __TSTAMP_H
#define __TSTAMP_H

#include <time.h>

#include "types.h"

//
// time stamp structure
//
struct tstamp_t {
   u_int    year  : 12;                // 4-digit year
   u_int    month : 4;                 // 1-12
   u_int    day   : 5;                 // 1-31
   u_int          : 11;                // padding
   u_int    hour  : 5;                 // 0-23
   u_int    min   : 6;                 // 0-59
   u_int    sec   : 6;                 // 0-59
   u_int          : 15;                // padding

   public:
      tstamp_t(void) {reset();}
      tstamp_t(int year, int month, int day, int hour, int min, int sec);
      tstamp_t(time_t time) {reset(time);}
      tstamp_t(const char *str) {reset(str);}

      void reset(void) {year = month = day = hour = min = sec = 0;};
      void reset(int year, int month, int day, int hour, int min, int sec);
      void reset(time_t time);
      void reset(const char *str);

      void shift(int secs) {reset(mktime() + secs);}

      int compare(const tstamp_t& tstamp) const;

      bool operator == (const tstamp_t& tstamp) const {return compare(tstamp) == 0;}
      bool operator != (const tstamp_t& tstamp) const {return compare(tstamp) != 0;}
      bool operator > (const tstamp_t& tstamp) const {return compare(tstamp) > 0;}
      bool operator >= (const tstamp_t& tstamp) const {return compare(tstamp) >= 0;}
      bool operator < (const tstamp_t& tstamp) const {return compare(tstamp) < 0;}
      bool operator <= (const tstamp_t& tstamp) const {return compare(tstamp) <= 0;}

      time_t mktime(void) const {return mktime(year, month, day, hour, min, sec);}

      static time_t mktime(int year, int month, int day, int hour, int min, int sec);

      static u_long jdate(int year, int month, int day);

      static inline u_int wday(u_long days) {return (days + 1) % 7;}

      static inline u_int wday(int year, int month, int day) {return wday(jdate(year, month, day));}
};

#endif // __TSTAMP_H
