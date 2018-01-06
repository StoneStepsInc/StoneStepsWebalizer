/*
    webalizer - a web server log analysis program

    Copyright (c) 2004-2018, Stone Steps Inc. (www.stonesteps.ca)

    See COPYING and Copyright files for additional licensing and copyright information

    tstamp.h
*/

#ifndef TSTAMP_H
#define TSTAMP_H

#include <ctime>

#include "types.h"
#include "tstring.h"

/// @struct tstamp_t
///
/// @brief  A time stamp structure that maintains and manipulates either local or UTC time
///
/// Date and time components may be stored in local time or UTC and are converted to the internal 
/// serial time for all date/time math operations.
///
/// The internal serial time (intime_t) is represented in seconds since midnight January 1st, 1970 
/// and may have the time zone offset included in the value. This data type allows us to operate on 
/// local time and UTC values uniformly. The former, effectively, is treated as a UTC value shifted 
/// by the time zone offset, which also makes calculating Julian day numbers for the internal serial 
/// time more straightforward because a Julian day number is placed at noon UTC for the resulting 
/// day.
///
/// The internal serial time cannot be used outside of tstamp_t because it is impossible to figure 
/// out the value of the time zone offset included in the internal serial time value. Caller of any 
/// function that makes use of the internal serial time must track whether it's local time or UTC, 
/// as well as the time zone offset value.
///
/// In order to convert a local time stamp X to time_t via the internal serial time:
///
///   * Time components of X are converted to seconds since midnight (B)
///   * A Julian day number is computed for date components of X as if it is in UTC
///   * The Julian day for January 1st, 1970 (2440588) is subtracted from the Julian day number 
///     for X, which yields the number of days between X and noon January 1st, 1970 UTC (C), 
///     which is the same as the number of days since midnight of January 1st 1970 UTC (D)
///   * The value of D is multiplied by the number of seconds in a day (86400) and yields the 
///     number of seconds to the midnight of the day of X, local time. Leap seconds are ignored.
///   * D and B are added, which yields an internal serial time of X
///   * The time zone offset (A) is subtracted from the previous result, which yields a UTC 
///     value corresponding to X and its time zone offset. 
/// ```
///          2440588
///     ...------------->|<--------------------------C---------------------------->|
///         UTC          |                                             |<-B->|     |
///        00:00         |                   00:00                     |     |     |            time
///    ...---+===========+===========+---...---+===========+=======U===+-----X-----N----------+---->
///     1970-01-01     12:00              2017-08-10     12:00     | 00:00   |   12:00
///          |                                                     |<---A--->|
///          |<----------------------------D-------------------------->|
/// ```
/// The math for UTC time stamps is even simpler because the internal seria time in this case
/// is in UTC and contains no time zone offset. 
///

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

      // the internal serial time in seconds (see class description)
      typedef int64_t intime_t;

      // A Julian day number is a number of days since noon January 1st, 4713 BC, UTC
      typedef u_int jdn_t;

   private:
      tstamp_t(uninitialized_t) {}

      u_int reset_time(intime_t time);
      void reset_date(jdn_t jdn);
      bool parse_tstamp(const char *str);

      static intime_t mkintime(u_int year, u_int month, u_int day, u_int hour, u_int min, u_int sec);

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

      int64_t compare(const tstamp_t& tstamp, int mode = tm_parts::TIMESTAMP) const;

      bool operator == (const tstamp_t& tstamp) const {return compare(tstamp) == 0;}
      bool operator != (const tstamp_t& tstamp) const {return compare(tstamp) != 0;}
      bool operator > (const tstamp_t& tstamp) const {return compare(tstamp) > 0;}
      bool operator >= (const tstamp_t& tstamp) const {return compare(tstamp) >= 0;}
      bool operator < (const tstamp_t& tstamp) const {return compare(tstamp) < 0;}
      bool operator <= (const tstamp_t& tstamp) const {return compare(tstamp) <= 0;}

      string_t format(void) const;

      time_t mktime(void) const;

      tstamp_t first_month_day(void) const;

      tstamp_t last_month_day(void) const;

      jdn_t jday(void) const {return jday(year, month, day);}

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
