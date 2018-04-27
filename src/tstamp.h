/*
    webalizer - a web server log analysis program

    Copyright (c) 2004-2017, Stone Steps Inc. (www.stonesteps.ca)

    See COPYING and Copyright files for additional licensing and copyright information

    tstamp.h
*/

#ifndef TSTAMP_H
#define TSTAMP_H

#include <ctime>

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

   public:
      // time stamp component identifiers
      struct tm_parts {
         static const u_int YEAR      = 64;
         static const u_int MONTH     = 32;
         static const u_int DAY       = 16;
         static const u_int HOUR      = 8;
         static const u_int MINUTE    = 4;
         static const u_int SECOND    = 2;
         static const u_int MLSEC     = 1;   // not used
         static const u_int DATE      = YEAR | MONTH | DAY; 
         static const u_int TIME      = HOUR | MINUTE | SECOND; 
         static const u_int TIMESTAMP = DATE | TIME; 
      };

   private:
      enum uninitialized_t {
         uninitialized
      };

   private:
      tstamp_t(uninitialized_t) {}

      u_int reset_time(int64_t time);
      void reset_date(u_int jdn);
      bool parse_tstamp(const char *str);

   public:
      tstamp_t(void) {reset();}
      tstamp_t(u_int year, u_int month, u_int day, u_int hour, u_int min, u_int sec, int offset);
      tstamp_t(u_int year, u_int month, u_int day, u_int hour, u_int min, u_int sec);
      explicit tstamp_t(time_t time) {reset(time);}
      tstamp_t(time_t time, int offset) {reset(time, offset);}

      void reset(void) {year = month = day = hour = min = sec = 0; offset = 0; utc = false; null = true;}
      void reset(u_int year, u_int month, u_int day, u_int hour, u_int min, u_int sec, int offset);
      void reset(u_int year, u_int month, u_int day, u_int hour, u_int min, u_int sec);
      void reset(time_t time);
      void reset(time_t time, int offset);

      bool parse(const char *str);
      bool parse(const char *str, int offset);

      void shift(int secs);

      int64_t elapsed(const tstamp_t& tstamp) const;

      void toutc(void);
      void tolocal(int offset);

      inline bool isutc(void) const {return utc;}
      inline bool islocal(void) const {return !utc;}

      int64_t compare(const tstamp_t& tstamp, int mode = tm_parts::TIMESTAMP) const;

      bool operator == (const tstamp_t& tstamp) const {return compare(tstamp) == 0;}
      bool operator != (const tstamp_t& tstamp) const {return compare(tstamp) != 0;}
      bool operator > (const tstamp_t& tstamp) const {return compare(tstamp) > 0;}
      bool operator >= (const tstamp_t& tstamp) const {return compare(tstamp) >= 0;}
      bool operator < (const tstamp_t& tstamp) const {return compare(tstamp) < 0;}
      bool operator <= (const tstamp_t& tstamp) const {return compare(tstamp) <= 0;}

      string_t format(void) const;

      time_t mktime(void) const;

      u_int last_month_day(void) const {return last_month_day(year, month);}

      u_int jday(void) const {return jday(year, month, day);}

      u_int wday(void) const {return wday(year, month, day);}

      static time_t mktime(u_int year, u_int month, u_int day, u_int hour, u_int min, u_int sec);

      static time_t mktime(u_int year, u_int month, u_int day, u_int hour, u_int min, u_int sec, int offset);

      static u_int jday(u_int year, u_int month, u_int day);

      // returns zero-based US week day number (0 - Sun, 1 - Mon, etc)
      static inline u_int wday(uint64_t days) {return (days + 1) % 7;}

      // returns one-based ISO week day number (1 - Mon, 2 - Tue, etc)
      static inline u_int wday_iso(uint64_t days) {return (days % 7) + 1;}

      static inline u_int wday(u_int year, u_int month, u_int day) {return wday(jday(year, month, day));}

      static inline u_int wday_iso(u_int year, u_int month, u_int day) {return wday_iso(jday(year, month, day));}

      static u_int last_month_day(u_int year, u_int month);
};

#endif // TSTAMP_H
