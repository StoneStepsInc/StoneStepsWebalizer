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
#include "tstring.h"

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
     int    offset: 11;                // UTC offset, -12h..14h in minutes (-720..840)
    bool    utc   : 1;                 // UTC or local?
    bool    null  : 1;                 // not initialized?
   u_int          : 2;                 // padding

   private:
      u_int reset_time(int64_t time);
      void reset_date(u_int jdn);
      bool parse_tstamp(const char *str);

   public:
      tstamp_t(void) {reset();}
      tstamp_t(u_int year, u_int month, u_int day, u_int hour, u_int min, u_int sec, int offset);
      tstamp_t(u_int year, u_int month, u_int day, u_int hour, u_int min, u_int sec);
      tstamp_t(time_t time) {reset(time);}

      void reset(void) {year = month = day = hour = min = sec = 0; offset = 0; utc = false; null = true;}
      void reset(u_int year, u_int month, u_int day, u_int hour, u_int min, u_int sec, int offset);
      void reset(u_int year, u_int month, u_int day, u_int hour, u_int min, u_int sec);
      void reset(time_t time);
      void reset(time_t time, int offset);

      bool parse(const char *str);
      bool parse(const char *str, int offset);

      void shift(int secs);

      void toutc(void);
      void tolocal(int offset);

      int64_t compare(const tstamp_t& tstamp) const;

      bool operator == (const tstamp_t& tstamp) const {return compare(tstamp) == 0;}
      bool operator != (const tstamp_t& tstamp) const {return compare(tstamp) != 0;}
      bool operator > (const tstamp_t& tstamp) const {return compare(tstamp) > 0;}
      bool operator >= (const tstamp_t& tstamp) const {return compare(tstamp) >= 0;}
      bool operator < (const tstamp_t& tstamp) const {return compare(tstamp) < 0;}
      bool operator <= (const tstamp_t& tstamp) const {return compare(tstamp) <= 0;}

      string_t format(void) const;

      time_t mktime(void) const;

      static time_t mktime(int year, int month, int day, int hour, int min, int sec);

      static time_t mktime(int year, int month, int day, int hour, int min, int sec, int offset);

      static u_int jday(u_int year, u_int month, u_int day);

      static inline u_int wday(u_long days) {return (days + 1) % 7;}

      static inline u_int wday(int year, int month, int day) {return wday(jday(year, month, day));}
};

#endif // __TSTAMP_H
