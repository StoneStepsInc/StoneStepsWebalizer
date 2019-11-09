/*
   webalizer - a web server log analysis program

   Copyright (c) 2004-2019, Stone Steps Inc. (www.stonesteps.ca)

   See COPYING and Copyright files for additional licensing and copyright information 
   
   ut_tstamp.cpp
*/
#include "pch.h"

#include "../tstamp.h"
#include "../tstring.h"
#include <ctime>

//
// TimeStampTest
//
namespace sswtest {
class TimeStampTest : public testing::Test {
   protected:
      int tz_offset_win;         ///< UTC offset for January 1st in minutes on the machine running this test.
      int tz_offset_sum;         ///< UTC offset for August 1st  in minutes on the machine running this test.

   public:
      TimeStampTest(void)
      {
         auto get_tz_offset = [] (struct tm&& tms) -> int
         {
            // get UTC time stamp for this local time
            time_t ts = mktime(&tms);

            // convert the same UTC time stamp to local and UTC date/time components
            struct tm lct = *localtime(&ts);
            struct tm gmt = *gmtime(&ts);

            // clear the DST flag, so mktime doesn't adjust for DST for the summer offset
            if(tms.tm_isdst)
               lct.tm_isdst = false;

            // treat UTC as if it is local time to get a time zone offset in minutes
            return (mktime(&lct) - mktime(&gmt)) / 60;
         };

         // 2017-01-01 00:00:00, local winter time
         tz_offset_win = get_tz_offset({ 0, 0, 0, 1, 0, 117 });

         // 2017-08-01 00:00:00, local summer time
         tz_offset_sum = get_tz_offset({ 0, 0, 0, 1, 7, 117 });
      }
};

///
/// @brief  Weekday Test Non-Leap Year
///
TEST_F(TimeStampTest, WeekdayNonLeapYear)
      {
         // 2017-01-01 00:00:00
         struct tm tms = {0, 0, 0, 1, 0, 117};
         tstamp_t tstamp(2017, 1, 1, 0, 0, 0, tz_offset_win);

         time_t ts = tstamp.mktime();

         EXPECT_EQ(mktime(&tms), ts) << "Start UTC time of a non-leap year should match that from the standard mktime return value";

         // go through all days of the year and check that weekdays match those returned by C runtime
         for(int i = 0; i < 365; i++, ts += 86400) {
            tstamp.reset(ts);

            //
            // Use gmtime to avoid time zone conversions in the C runtime. Note that
            // a date without time is not affected by time zone conversions (i.e. the
            // same date components always yield the same weekday in any time zone).
            //
            EXPECT_EQ((u_int) gmtime(&ts)->tm_wday, tstamp.wday()) << "Weekday of a non-leap year must match that from the standard gmtime function";
         }
      }

///
/// @brief  Weekday Test Leap Year
///
TEST_F(TimeStampTest, WeekdayLeapYear)
{
   // 2016-01-01 00:00:00
   struct tm tms = {0, 0, 0, 1, 0, 116};
   tstamp_t tstamp(2016, 1, 1, 0, 0, 0, tz_offset_win);

   time_t ts = tstamp.mktime();

   EXPECT_EQ(mktime(&tms), ts) << "Start UTC time of a leap year should match that from the standard mktime return value";

   // go through all days of the year and check that weekdays match those returned by C runtime
   for(int i = 0; i < 365; i++, ts += 86400) {
      tstamp.reset(ts);

      // see WeekdayNonLeapYear
      EXPECT_EQ((u_int) gmtime(&ts)->tm_wday, tstamp.wday()) << "Weekday of a leap year must match that from the standard gmtime function";
   }
}

///
/// @brief  Weekday Test Non-Leap Year
///
TEST_F(TimeStampTest, TimeStampParseUTC)
{
   tstamp_t tstamp_utc(2017, 8, 30, 15, 10, 25);
   tstamp_t tstamp;

   EXPECT_TRUE(tstamp.parse("2017-08-30 15:10:25")) << "2017-08-30 15:10:25 UTC should be parsed without an error";
   EXPECT_TRUE(tstamp_utc == tstamp) << "Parsed 2017-08-30 15:10:25 UTC should compare equal to a constructed time stamp";

   EXPECT_TRUE(tstamp.parse("2017-08-30T15:10:25")) << L"2017-08-30T15:10:25 UTC should be parsed without an error";
   EXPECT_TRUE(tstamp_utc == tstamp) << "Parsed 2017-08-30T15:10:25 UTC should compare equal to a constructed time stamp";

   EXPECT_TRUE(tstamp.parse("2017-08-30T15:10")) << "2017-08-30T15:10 UTC should be parsed without an error";
   EXPECT_TRUE(!tstamp_utc.compare(tstamp, tstamp_t::tm_parts::DATE | tstamp_t::tm_parts::HOUR | tstamp_t::tm_parts::MINUTE)) << "Parsed 2017-08-30T15:10 UTC should compare equal to a constructed time stamp";

   EXPECT_TRUE(tstamp.parse("2017-08-30")) << "2017-08-30 UTC should be parsed without an error";
   EXPECT_TRUE(!tstamp_utc.compare(tstamp, tstamp_t::tm_parts::DATE)) << "Parsed 2017-08-30 UTC should compare equal to a constructed time stamp";

   EXPECT_TRUE(tstamp.parse("2017/08/30")) << "2017/08/30 UTC should be parsed without an error";
   EXPECT_TRUE(!tstamp_utc.compare(tstamp, tstamp_t::tm_parts::DATE)) << "Parsed 2017/08/30 UTC should compare equal to a constructed time stamp";
}

///
/// @brief  Weekday Test Non-Leap Year
///
TEST_F(TimeStampTest, TimeStampParseLocal)
{
   tstamp_t tstamp_local(2017, 8, 30, 15, 10, 25, tz_offset_sum);
   tstamp_t tstamp;

   EXPECT_TRUE(tstamp.parse("2017-08-30 15:10:25", tz_offset_sum)) << "2017-08-30 15:10:25 local time should be parsed without an error";
   EXPECT_TRUE(tstamp_local == tstamp) << "Parsed 2017-08-30 15:10:25 local time should compare equal to a constructed time stamp";

   EXPECT_TRUE(tstamp.parse("2017-08-30T15:10:25", tz_offset_sum)) << "2017-08-30T15:10:25 local time should be parsed without an error";
   EXPECT_TRUE(tstamp_local == tstamp) << "Parsed 2017-08-30T15:10:25 local time should compare equal to a constructed time stamp";

   EXPECT_TRUE(tstamp.parse("2017-08-30T15:10", tz_offset_sum)) << "2017-08-30T15:10 local time should be parsed without an error";
   EXPECT_EQ(0, tstamp_local.compare(tstamp, tstamp_t::tm_parts::DATE | tstamp_t::tm_parts::HOUR | tstamp_t::tm_parts::MINUTE)) << "Parsed 2017-08-30T15:10 local time should compare equal to a constructed time stamp";

   EXPECT_TRUE(tstamp.parse("2017-08-30", tz_offset_sum)) << "2017-08-30 local time should be parsed without an error";
   EXPECT_EQ(0, tstamp_local.compare(tstamp, tstamp_t::tm_parts::DATE)) << "Parsed 2017-08-30 local time should compare equal to a constructed time stamp";

   EXPECT_TRUE(tstamp.parse("2017/08/30", tz_offset_sum)) << "2017/08/30 local time should be parsed without an error";
   EXPECT_EQ(0, tstamp_local.compare(tstamp, tstamp_t::tm_parts::DATE)) << "Parsed 2017/08/30 local time should compare equal to a constructed time stamp";
}

///
/// @brief  Test time zone conversion functionality.
///
TEST_F(TimeStampTest, TimeZoneConversion)
{
   const tstamp_t tstamp_lcl_neg5hrs(2018, 4, 26, 15, 10, 25, -300);
   const tstamp_t tstamp_lcl_pos2hrs(2018, 4, 26, 22, 10, 25, 120);
   const tstamp_t tstamp_utc(2018, 4, 26, 20, 10, 25);

   tstamp_t tstamp;

   tstamp = tstamp_utc;
   tstamp.toutc();
   EXPECT_EQ(tstamp_utc, tstamp) << "UTC time stamp converted to UTC should not change";

   tstamp = tstamp_lcl_neg5hrs;
   tstamp.tolocal(-300);
   EXPECT_EQ(tstamp_lcl_neg5hrs, tstamp) << "Time stamp with the same UTC offset should not change";

   tstamp = tstamp_lcl_neg5hrs;
   tstamp.tolocal(120);
   EXPECT_EQ(tstamp_lcl_pos2hrs, tstamp) << "UTC-5 local time to UTC+2 local time moves 7 hours ahead";

   tstamp = tstamp_lcl_neg5hrs;
   tstamp.tolocal(-300);
   EXPECT_EQ(tstamp_lcl_neg5hrs, tstamp) << "UTC+2 local time to UTC-5 local time moves 7 hours back";

   tstamp = tstamp_lcl_neg5hrs;
   tstamp.toutc();
   EXPECT_EQ(tstamp_utc, tstamp) << "UTC-5 local time to UTC moves 5 hours ahead";

   tstamp = tstamp_lcl_pos2hrs;
   tstamp.toutc();
   EXPECT_EQ(tstamp_utc, tstamp) << "UTC+2 local time to UTC moves 2 hours back";

   tstamp = tstamp_utc;
   tstamp.tolocal(-300);
   EXPECT_EQ(tstamp_lcl_neg5hrs, tstamp) << "UTC to UTC-5 local time moves 5 hours back";

   tstamp = tstamp_utc;
   tstamp.tolocal(120);
   EXPECT_EQ(tstamp_lcl_pos2hrs, tstamp) << "UTC to UTC+2 local time moves 2 hours ahead";
}

///
/// @brief  Test constructing an null time stamp.
///
TEST_F(TimeStampTest, ConstructTimeStampNull)
{
   // empty default-constructed time stamp
   tstamp_t empty_ts;

   EXPECT_TRUE(empty_ts.null) << "A default-constructed time stamp should be null";

   // time stamp components of a null time stamp don't matter, but still should be initialized

   EXPECT_FALSE(empty_ts.utc);

   EXPECT_EQ(0, empty_ts.year);
   EXPECT_EQ(0, empty_ts.month);
   EXPECT_EQ(0, empty_ts.day);
   EXPECT_EQ(0, empty_ts.hour);
   EXPECT_EQ(0, empty_ts.min);
   EXPECT_EQ(0, empty_ts.sec);
   EXPECT_EQ(0, empty_ts.offset);
}

///
/// @brief  Test resetting a time stamp to a null value.
///
TEST_F(TimeStampTest, ResetTimeStampNull)
{
   tstamp_t tstamp(2019, 8, 18, 19, 37, 58, tz_offset_sum);

   EXPECT_FALSE(tstamp.null);

   tstamp.reset();

   EXPECT_TRUE(tstamp.null);
   EXPECT_FALSE(tstamp.utc);

   EXPECT_EQ(0, tstamp.year);
   EXPECT_EQ(0, tstamp.month);
   EXPECT_EQ(0, tstamp.day);
   EXPECT_EQ(0, tstamp.hour);
   EXPECT_EQ(0, tstamp.min);
   EXPECT_EQ(0, tstamp.sec);
   EXPECT_EQ(0, tstamp.offset);
}

///
/// @brief  Test constructing a UTC time stamp.
///
TEST_F(TimeStampTest, ConstructTimeStampUTC)
{
   // UTC time stamp
   tstamp_t utc_ts(2019, 8, 18, 9, 42, 15);

   EXPECT_FALSE(utc_ts.null);
   EXPECT_TRUE(utc_ts.utc);

   EXPECT_EQ(2019, utc_ts.year);
   EXPECT_EQ(8, utc_ts.month);
   EXPECT_EQ(18, utc_ts.day);
   EXPECT_EQ(9, utc_ts.hour);
   EXPECT_EQ(42, utc_ts.min);
   EXPECT_EQ(15, utc_ts.sec);
   EXPECT_EQ(0, utc_ts.offset);
}

///
/// @brief  Test resetting a time stamp in UTC.
///
TEST_F(TimeStampTest, ResetTimeStampUTC)
{
   tstamp_t utc_ts;

   EXPECT_TRUE(utc_ts.null);
   
   utc_ts.reset(2019, 8, 18, 9, 42, 15);

   EXPECT_FALSE(utc_ts.null);
   EXPECT_TRUE(utc_ts.utc);

   EXPECT_EQ(2019, utc_ts.year);
   EXPECT_EQ(8, utc_ts.month);
   EXPECT_EQ(18, utc_ts.day);
   EXPECT_EQ(9, utc_ts.hour);
   EXPECT_EQ(42, utc_ts.min);
   EXPECT_EQ(15, utc_ts.sec);
   EXPECT_EQ(0, utc_ts.offset);
}

///
/// @brief  Test constructing a local time stamp.
///
TEST_F(TimeStampTest, ConstructTimeStampLocalTime)
{
   // local time stamp
   tstamp_t local_ts(2019, 8, 18, 9, 42, 15, tz_offset_sum);

   EXPECT_FALSE(local_ts.null);
   EXPECT_FALSE(local_ts.utc);

   EXPECT_EQ(2019, local_ts.year);
   EXPECT_EQ(8, local_ts.month);
   EXPECT_EQ(18, local_ts.day);
   EXPECT_EQ(9, local_ts.hour);
   EXPECT_EQ(42, local_ts.min);
   EXPECT_EQ(15, local_ts.sec);
   EXPECT_EQ(tz_offset_sum, local_ts.offset);
}

///
/// @brief  Test resetting a time stamp in local time.
///
TEST_F(TimeStampTest, ResetTimeStampLocalTime)
{
   tstamp_t local_ts;
   
   EXPECT_TRUE(local_ts.null);

   local_ts.reset(2019, 8, 18, 9, 42, 15, tz_offset_sum);

   EXPECT_FALSE(local_ts.null);
   EXPECT_FALSE(local_ts.utc);

   EXPECT_EQ(2019, local_ts.year);
   EXPECT_EQ(8, local_ts.month);
   EXPECT_EQ(18, local_ts.day);
   EXPECT_EQ(9, local_ts.hour);
   EXPECT_EQ(42, local_ts.min);
   EXPECT_EQ(15, local_ts.sec);
   EXPECT_EQ(tz_offset_sum, local_ts.offset);
}

///
/// @brief  Test constructing a UTC time stamp from `time_t`.
///
TEST_F(TimeStampTest, ConstructTimeStamp_time_t)
{
   // 2019-08-18 09:42:15, local time, no DST
   struct tm tms = { 15, 42, 9, 18, 7, 119 };

   // get UTC time stamp for this local time
   time_t ts = mktime(&tms);

   // get a UTC time stamp to test against
   const struct tm gmt = *gmtime(&ts);

   tstamp_t time_t_ts(ts);

   EXPECT_FALSE(time_t_ts.null);
   EXPECT_TRUE(time_t_ts.utc);

   EXPECT_EQ(gmt.tm_year + 1900, time_t_ts.year);
   EXPECT_EQ(gmt.tm_mon + 1, time_t_ts.month);
   EXPECT_EQ(gmt.tm_mday, time_t_ts.day);
   EXPECT_EQ(gmt.tm_hour, time_t_ts.hour);
   EXPECT_EQ(gmt.tm_min, time_t_ts.min);
   EXPECT_EQ(gmt.tm_sec, time_t_ts.sec);
   EXPECT_EQ(0, time_t_ts.offset);
}

///
/// @brief  Test resetting a time stamp as UTC time from `time_t`.
///
TEST_F(TimeStampTest, ResetTimeStamp_time_t)
{
   // 2019-08-18 09:42:15, local time, no DST
   struct tm tms = { 15, 42, 9, 18, 7, 119 };

   // get UTC time stamp for this local time
   time_t ts = mktime(&tms);

   // get a UTC time stamp to test against
   const struct tm gmt = *gmtime(&ts);

   tstamp_t time_t_ts;

   EXPECT_TRUE(time_t_ts.null);
   
   time_t_ts.reset(ts);

   EXPECT_FALSE(time_t_ts.null);
   EXPECT_TRUE(time_t_ts.utc);

   EXPECT_EQ(gmt.tm_year + 1900, time_t_ts.year);
   EXPECT_EQ(gmt.tm_mon + 1, time_t_ts.month);
   EXPECT_EQ(gmt.tm_mday, time_t_ts.day);
   EXPECT_EQ(gmt.tm_hour, time_t_ts.hour);
   EXPECT_EQ(gmt.tm_min, time_t_ts.min);
   EXPECT_EQ(gmt.tm_sec, time_t_ts.sec);
   EXPECT_EQ(0, time_t_ts.offset);
}

///
/// @brief  Test constructing a local time stamp from `time_t` and a time zone offset.
///
TEST_F(TimeStampTest, ConstructTimeStamp_time_t_WithOffset)
{
   // 2019-08-18 09:42:15, local time, no DST
   struct tm tms = { 15, 42, 9, 18, 7, 119 };

   // get UTC time stamp for this local time
   time_t ts = mktime(&tms);

   // get a local time to test against
   const struct tm lct = *localtime(&ts);

   tstamp_t time_t_ts(ts, tz_offset_sum);

   EXPECT_FALSE(time_t_ts.null);
   EXPECT_FALSE(time_t_ts.utc);

   EXPECT_EQ(lct.tm_year + 1900, time_t_ts.year);
   EXPECT_EQ(lct.tm_mon + 1, time_t_ts.month);
   EXPECT_EQ(lct.tm_mday, time_t_ts.day);
   EXPECT_EQ(lct.tm_hour, time_t_ts.hour);
   EXPECT_EQ(lct.tm_min, time_t_ts.min);
   EXPECT_EQ(lct.tm_sec, time_t_ts.sec);
   EXPECT_EQ(tz_offset_sum, time_t_ts.offset);
}

///
/// @brief  Test resetting a time stamp in local time from `time_t` and a time zone offset.
///
TEST_F(TimeStampTest, ResetTimeStamp_time_t_WithOffset)
{
   // 2019-08-18 09:42:15, local time, no DST
   struct tm tms = { 15, 42, 9, 18, 7, 119 };

   // get a UTC time stamp for this local time
   time_t ts = mktime(&tms);

   // get a local time to test against
   const struct tm lct = *localtime(&ts);

   tstamp_t time_t_ts;

   EXPECT_TRUE(time_t_ts.null);
   
   time_t_ts.reset(ts, tz_offset_sum);

   EXPECT_FALSE(time_t_ts.null);
   EXPECT_FALSE(time_t_ts.utc);

   EXPECT_EQ(lct.tm_year + 1900, time_t_ts.year);
   EXPECT_EQ(lct.tm_mon + 1, time_t_ts.month);
   EXPECT_EQ(lct.tm_mday, time_t_ts.day);
   EXPECT_EQ(lct.tm_hour, time_t_ts.hour);
   EXPECT_EQ(lct.tm_min, time_t_ts.min);
   EXPECT_EQ(lct.tm_sec, time_t_ts.sec);
   EXPECT_EQ(tz_offset_sum, time_t_ts.offset);
}

TEST_F(TimeStampTest, CompareSameTimeDiffTimeZones)
{
   // same time stamps, different time zones
   const tstamp_t tstamp_lcl_neg5hrs(2018, 4, 26, 15, 10, 25, -300);
   const tstamp_t tstamp_lcl_pos2hrs(2018, 4, 26, 22, 10, 25, 120);
   const tstamp_t tstamp_utc(2018, 4, 26, 20, 10, 25);

   tstamp_t tstamp;
   
   tstamp.reset(2018, 4, 26, 15, 10, 25, -300);
   EXPECT_EQ(tstamp_lcl_neg5hrs, tstamp);
   EXPECT_EQ(tstamp_lcl_pos2hrs, tstamp);
   EXPECT_EQ(tstamp_utc, tstamp);

   tstamp.reset(2018, 4, 26, 22, 10, 25, 120);
   EXPECT_EQ(tstamp_lcl_neg5hrs, tstamp);
   EXPECT_EQ(tstamp_lcl_pos2hrs, tstamp);
   EXPECT_EQ(tstamp_utc, tstamp);

   tstamp.reset(2018, 4, 26, 20, 10, 25);
   EXPECT_EQ(tstamp_lcl_neg5hrs, tstamp);
   EXPECT_EQ(tstamp_lcl_pos2hrs, tstamp);
   EXPECT_EQ(tstamp_utc, tstamp);
}

TEST_F(TimeStampTest, CompareSameComponentsDiffTimeZones)
{
   // same time stamp components, different time zones
   const tstamp_t tstamp_lcl_neg5hrs(2018, 4, 26, 15, 10, 25, -300);
   const tstamp_t tstamp_lcl_pos2hrs(2018, 4, 26, 15, 10, 25, 120);
   const tstamp_t tstamp_utc(2018, 4, 26, 15, 10, 25);

   tstamp_t tstamp;
   
   tstamp.reset(2018, 4, 26, 15, 10, 25, -300);
   EXPECT_EQ(tstamp_lcl_neg5hrs, tstamp);
   EXPECT_LT(tstamp_lcl_pos2hrs, tstamp);
   EXPECT_LT(tstamp_utc, tstamp);

   tstamp.reset(2018, 4, 26, 15, 10, 25, 120);
   EXPECT_GT(tstamp_lcl_neg5hrs, tstamp);
   EXPECT_EQ(tstamp_lcl_pos2hrs, tstamp);
   EXPECT_GT(tstamp_utc, tstamp);

   tstamp.reset(2018, 4, 26, 15, 10, 25);
   EXPECT_GT(tstamp_lcl_neg5hrs, tstamp);
   EXPECT_LT(tstamp_lcl_pos2hrs, tstamp);
   EXPECT_EQ(tstamp_utc, tstamp);
}

}

