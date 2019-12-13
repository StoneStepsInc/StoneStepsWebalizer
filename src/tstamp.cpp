/*
    webalizer - a web server log analysis program

    Copyright (c) 2004-2019, Stone Steps Inc. (www.stonesteps.ca)

    See COPYING and Copyright files for additional licensing and copyright information

    tstamp.cpp
*/
#include "pch.h"

#include "tstamp.h"
#include "util_string.h"

#include <cctype>
#include <memory>
#include <new>

tstamp_t::tstamp_t(void) :
      year(0), month(0), day(0),
      hour(0), min(0), sec(0),
      offset(0),
      utc(false),
      null(true)
{
}

tstamp_t::tstamp_t(u_int y, u_int mo, u_int d, u_int h, u_int mi, u_int s, int of) :
      year(y), month(mo), day(d),
      hour(h), min(mi), sec(s),
      offset(of),
      utc(false),
      null(false)
{
}

tstamp_t::tstamp_t(u_int y, u_int mo, u_int d, u_int h, u_int mi, u_int s) :
      year(y), month(mo), day(d),
      hour(h), min(mi), sec(s),
      offset(0),
      utc(true),
      null(false)
{
}

void tstamp_t::reset(u_int y, u_int mo, u_int d, u_int h, u_int mi, u_int s, int of)
{
   year = y;
   month = mo;
   day = d;
   hour = h;
   min = mi;
   sec = s;
   offset = of;
   utc = false;
   null = false;
}

void tstamp_t::reset(u_int y, u_int mo, u_int d, u_int h, u_int mi, u_int s)
{
   year = y;
   month = mo;
   day = d;
   hour = h;
   min = mi;
   sec = s;
   offset = 0;
   utc = true;
   null = false;
}

//
// Extracts the number of seconds correspondng to the last day within the specified time 
// value, converts this number to the time components and uses the remaining whole number 
// of days expressed in seconds to compute the Julian day number of that day, regardless 
// of the time zone offset. Consequently, the returned value is a Julian day number either 
// in UTC or local time.
//
u_int tstamp_t::reset_time(intime_t time)
{
   int64_t t;

   time -= (t = time % 60ll);
   sec = t;

   time -= (t = time % 3600ll);
   min = t / 60ll;

   time -= (t = time % 86400ll);
   hour = t / 3600ll;

   //
   // The remaining time value at this point is the number of days since midnight 
   // 1970-01-01 UTC. Adding the Julian day number for noon 1970-01-01 UTC (2440588), 
   // produces a Julian day number either in UTC or in local time.
   //
   return (u_int) (time / 86400ll + 2440588ll);
}

void tstamp_t::reset_date(jdn_t jdn)
{
   uint64_t a, b, c, d, e, m;

   a = jdn + 32044ull;
   b = (4ull*a + 3ull)/146097ull;
   c = a - (b*146097ull)/4ull;
   d = (4ull*c + 3ull)/1461ull;
   e = c - (1461ull*d)/4ull;
   m = (5ull*e + 2ull)/153ull;

   year = (u_int) (b*100ull + d - 4800ull + m/10ull);
   month = (u_int) (m + 3ull - 12ull*(m/10ull));
   day = (u_int) (e - (153ull*m+2ull)/5ull + 1ull);
}

void tstamp_t::reset(time_t time)
{
   reset_date(reset_time(time));

   offset = 0;
   utc = true;
   null = false;
}

void tstamp_t::reset(time_t time, int of)
{
   reset_date(reset_time(time + of * 60ll));

   offset = of;
   utc = false;
   null = false;
}

//
// Parses a time stamp string and returns true if the input is a valid time stamp. 
// If the method returns false, then data members are left unchanged. This method
// only assigns date/time components and does not change the time stamp state.
//
//    2007-11-17 12:30:01
//    2007-11-17 2:00
//    2007-11-17
//
bool tstamp_t::parse_tstamp(const char *str)
{
   static const struct {
      bool status;            // parse status 
      const char *delim;      // valid delimiters
   } desc[] = {{false, "-/"}, {false, "-/"}, {true, " T"}, {false, ":"}, {true, ":"}, {true, nullptr}};

   const char *cp1, *cp2;
   u_int i = 0;
   u_int p[6];

   cp1 = str;

   while (cp1 && *cp1 && i < sizeof(desc)/sizeof(desc[0])) {
      p[i] = (u_int) str2ul(cp1, &cp2);
      
      // check if we got a number
      if(!cp2)
         return false;

      // break out if there's no more input
      if(!*cp2)
         break;

      // otherwise check for excess of input or if we got a bad delimiter
      if(!desc[i].delim || !strchr(desc[i].delim, *cp2))
         return false;

      // move past the last delimiter and select the next element in the date parts array
      cp1 = cp2 + 1;
      i++;
   }

   // check if we stopped at a point that produces a valid time stamp
   if(!desc[i].status)
      return false;

   // we got a valid timestamp and i at this point may only be 2, 4 and 5
   if(i >= 2) {
      year = p[0];
      month = p[1];
      day = p[2];
   }

   if(i >= 4) {
      hour = p[3];
      min = p[4];
   }
   else
      hour = min = sec = 0;

   if(i == 5)
      sec = p[5];
   else
      sec = 0;

   // reset the offset because we never expect it in the input
   offset = 0;

   return true;
}

bool tstamp_t::parse(const char *str)
{
   reset();

   if(!parse_tstamp(str))
      return false;

   utc = true;
   null = false;

   return true;
}

bool tstamp_t::parse(const char *str, int of)
{
   reset();

   if(!parse_tstamp(str))
      return false;

   offset = of;
   utc = false;
   null = false;

   return true;
}

void tstamp_t::shift(int secs) 
{
   utc ?
      reset(mktime() + secs) :
      reset(mktime() + secs, offset);
}

int64_t tstamp_t::elapsed(const tstamp_t& tstamp) const
{
   return mktime() - tstamp.mktime();
}

void tstamp_t::toutc(void) 
{
   if(!null && !utc) 
      reset(mktime());
}

void tstamp_t::tolocal(int of) 
{
   if(!null && (utc || of != offset)) 
      reset(mktime(), of);
}

int64_t tstamp_t::compare(const tstamp_t& other, int mode) const
{
   static auto temp_deleter = [] (tstamp_t *temp) {temp->tstamp_t::~tstamp_t();};
   char tmpbuf[sizeof(tstamp_t)];
   std::unique_ptr<tstamp_t, decltype(temp_deleter)> temp(nullptr, temp_deleter);

   // need to convert the time zone of the other time stamp if the UTC flags or offsets mismatch
   if(utc != other.utc || (!utc && offset != other.offset)) {
      // create a temporary time stamp with the same utc/offset attributes
      temp.reset(new (tmpbuf) tstamp_t(other));
      utc ? temp->toutc() : temp->tolocal(offset);
   }

   const tstamp_t& tstamp = temp ? *temp : other;

   //
   // Compare the selected components of both time stamps. Make sure to cast each 
   // component to int, so we avoid negative results converted to u_int and then 
   // to large int64_t values (i.e. no sign extension in this case).
   //
   if(mode & tm_parts::YEAR) {
      if(year != tstamp.year)
         return (int) year - (int) tstamp.year;
   }

   if(mode & tstamp_t::tm_parts::MONTH) {
      if(month != tstamp.month)
         return (int) month - (int) tstamp.month;
   }

   if(mode & tstamp_t::tm_parts::DAY) {
      if(day != tstamp.day)
         return (int) day - (int) tstamp.day;
   }

   if(mode & tstamp_t::tm_parts::HOUR) {
      if(hour != tstamp.hour)
         return (int) hour - (int) tstamp.hour;
   }

   if(mode & tstamp_t::tm_parts::MINUTE) {
      if(min != tstamp.min)
         return (int) min - (int) tstamp.min;
   }

   if(mode & tstamp_t::tm_parts::SECOND) {
      if(sec != tstamp.sec)
         return (int) sec - (int) tstamp.sec;
   }

   return 0;
}

string_t tstamp_t::format(void) const
{
   // output only non-zero seconds
   if(sec) return utc ? 
      string_t::_format("%04d-%02d-%02d %02d:%02d:%02dZ", year, month, day, hour, min, sec) :
      string_t::_format("%04d-%02d-%02d %02d:%02d:%02d%+05d", year, month, day, hour, min, sec, ((offset / 60) * 100 + offset % 60));
   else return utc ? 
      string_t::_format("%04d-%02d-%02d %02d:%02dZ", year, month, day, hour, min) :
      string_t::_format("%04d-%02d-%02d %02d:%02d%+05d", year, month, day, hour, min, ((offset / 60) * 100 + offset % 60));
}

time_t tstamp_t::mktime(void) const 
{
   return utc ? 
      mktime(year, month, day, hour, min, sec) : 
      mktime(year, month, day, hour, min, sec, offset);
}

time_t tstamp_t::mktime(u_int year, u_int month, u_int day, u_int hour, u_int min, u_int sec)
{
   return (time_t) mkintime(year, month, day, hour, min, sec);
}

time_t tstamp_t::mktime(u_int year, u_int month, u_int day, u_int hour, u_int min, u_int sec, int offset)
{
   //
   // Convert local time to the internal serial time and then subtract the 
   // offset (i.e. negative offsets will be added) to produce actual UTC time.
   //
   return (time_t) (mkintime(year, month, day, hour, min, sec) - (offset * 60ll));
}

//
// Returns the number of seconds since midnight 1/1/1970, internal serial time
//
tstamp_t::intime_t tstamp_t::mkintime(u_int year, u_int month, u_int day, u_int hour, u_int min, u_int sec)
{
   //
   // Julian day number for noon 1970-01-01 (UTC) is 2440588. Subtracting this value
   // from the value returned from jday produces midnight UTC time for the specified 
   // date, which is treated as an internal serial time because at this point it is 
   // not known whether these time components are in local time or UTC.
   //
   return ((intime_t) (jday(year, month, day) - 2440588) * 86400ll) + hour*3600ll + min*60ll + sec;
}

//
// Returns a Julian day number - number of days from noon January 1st, 4713 BC (UTC)
//
tstamp_t::jdn_t tstamp_t::jday(u_int year, u_int month, u_int day)
{
   u_int a, y, m;
   a = (14 - month)/12;
   y = year+4800-a;
   m = month + 12*a - 3;
   return day + (153*m+2)/5 + y*365 + y/4 - y/100 + y/400 - 32045;
}

u_int tstamp_t::last_month_day(u_int year, u_int month)
{
   //
   // Find out how many days there are between the beginning of the 28th 
   // day of the requested month and the beginning of the 1st day of the 
   // next month and add the difference to 27 to account for the full 
   // day of the 28th included into the difference.
   //
   return 27 + ((month < 12) ? jday(year, month+1, 1) : jday(year+1, 1, 1)) - jday(year, month, 28);
}

tstamp_t tstamp_t::first_month_day(void) const
{
   tstamp_t tstamp(*this);

   // changing a day in the time stamp does not affect any other components
   tstamp.day = 1;

   return tstamp;
}

tstamp_t tstamp_t::last_month_day(void) const
{
   tstamp_t tstamp(*this);

   // changing a day in the time stamp does not affect any other components
   tstamp.day = last_month_day(year, month);

   return tstamp;
}
