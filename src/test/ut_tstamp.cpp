/*
   webalizer - a web server log analysis program

   Copyright (c) 2004-2019, Stone Steps Inc. (www.stonesteps.ca)

   See COPYING and Copyright files for additional licensing and copyright information 
   
   ut_tstamp.cpp
*/
#include "pchtest.h"

#include "../tstamp.h"
#include "../tstring.h"
#include <ctime>

//
// TimeStampTest
//
namespace sswtest {
class TimeStampTest : public testing::Test {
   protected:
      int tz_offset;             ///< UTC offset in minutes of the machine running this test.

   public:
      TimeStampTest(void)
      {
         // 2017-01-01 00:00:00, local time, no DST
         struct tm tms = {0, 0, 0, 1, 0, 117};

         // get UTF time stamp for this local time
         time_t ts = mktime(&tms);

         // convert the same UTC time stamp to local and UTC date/time components
         struct tm lct = *localtime(&ts);
         struct tm gmt = *gmtime(&ts);

         // treat UTC as if it is local time to get a time zone offset in minutes
         tz_offset = (mktime(&lct) - mktime(&gmt)) / 60;
      }
};

///
/// @brief  Weekday Test Non-Leap Year
///
TEST_F(TimeStampTest, WeekdayNonLeapYear)
      {
         // 2017-01-01 00:00:00
         struct tm tms = {0, 0, 0, 1, 0, 117};
         tstamp_t tstamp(2017, 1, 1, 0, 0, 0, tz_offset);

         time_t ts = tstamp.mktime();

         EXPECT_EQ(mktime(&tms), ts) << "Start UTC time of a non-leap yearshould match that from the standard mktime return value";

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
   tstamp_t tstamp(2016, 1, 1, 0, 0, 0, tz_offset);

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
   tstamp_t tstamp_local(2017, 8, 30, 15, 10, 25, tz_offset);
   tstamp_t tstamp;

   EXPECT_TRUE(tstamp.parse("2017-08-30 15:10:25", tz_offset)) << "2017-08-30 15:10:25 local time should be parsed without an error";
   EXPECT_TRUE(tstamp_local == tstamp) << "Parsed 2017-08-30 15:10:25 local time should compare equal to a constructed time stamp";

   EXPECT_TRUE(tstamp.parse("2017-08-30T15:10:25", tz_offset)) << "2017-08-30T15:10:25 local time should be parsed without an error";
   EXPECT_TRUE(tstamp_local == tstamp) << "Parsed 2017-08-30T15:10:25 local time should compare equal to a constructed time stamp";

   EXPECT_TRUE(tstamp.parse("2017-08-30T15:10", tz_offset)) << "2017-08-30T15:10 local time should be parsed without an error";
   EXPECT_EQ(0, tstamp_local.compare(tstamp, tstamp_t::tm_parts::DATE | tstamp_t::tm_parts::HOUR | tstamp_t::tm_parts::MINUTE)) << "Parsed 2017-08-30T15:10 local time should compare equal to a constructed time stamp";

   EXPECT_TRUE(tstamp.parse("2017-08-30", tz_offset)) << "2017-08-30 local time should be parsed without an error";
   EXPECT_EQ(0, tstamp_local.compare(tstamp, tstamp_t::tm_parts::DATE)) << "Parsed 2017-08-30 local time should compare equal to a constructed time stamp";

   EXPECT_TRUE(tstamp.parse("2017/08/30", tz_offset)) << "2017/08/30 local time should be parsed without an error";
   EXPECT_EQ(0, tstamp_local.compare(tstamp, tstamp_t::tm_parts::DATE)) << "Parsed 2017/08/30 local time should compare equal to a constructed time stamp";
}

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

}

