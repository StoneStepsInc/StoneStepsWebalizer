/*
   webalizer - a web server log analysis program

   Copyright (c) 2004-2017, Stone Steps Inc. (www.stonesteps.ca)

   See COPYING and Copyright files for additional licensing and copyright information 
   
   ut_tstamp.cpp
*/
#include "pchtest.h"

#include "../tstamp.h"
#include "../tstring.h"
#include <ctime>

namespace mstest = Microsoft::VisualStudio::CppUnitTestFramework;

//
// TimeStampTest
//
namespace sswtest {
TEST_CLASS(TimeStampTest) {
   private:
      int tz_offset;

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

      BEGIN_TEST_METHOD_ATTRIBUTE(WeekdayNonLeapYear)
         TEST_DESCRIPTION(L"Weekday Test Non-Leap Year")
         TEST_METHOD_ATTRIBUTE(L"Category", L"TimeStamp Test")
      END_TEST_METHOD_ATTRIBUTE()

      TEST_METHOD(WeekdayNonLeapYear)
      {
         // 2017-01-01 00:00:00
         struct tm tms = {0, 0, 0, 1, 0, 117};
         tstamp_t tstamp(2017, 1, 1, 0, 0, 0, tz_offset);

         time_t ts = tstamp.mktime();

         // VS CPP unit doesn't provide string conversion for signed 64-bit integers, which is what time_t is
         mstest::Assert::AreEqual((unsigned long long) mktime(&tms), (unsigned long long) ts, L"Start UTC time of a non-leap yearshould match that from the standard mktime return value");

         // go through all days of the year and check that weekdays match those returned by C runtime
         for(int i = 0; i < 365; i++, ts += 86400) {
            tstamp.reset(ts);

            //
            // Use gmtime to avoid time zone conversions in the C runtime. Note that
            // a date without time is not affected by time zone conversions (i.e. the
            // same date components always yield the same weekday in any time zone).
            //
            mstest::Assert::AreEqual((u_int) gmtime(&ts)->tm_wday, tstamp.wday(), L"Weekday of a non-leap year must match that from the standard gmtime function");
         }
      }

      BEGIN_TEST_METHOD_ATTRIBUTE(WeekdayLeapYear)
         TEST_DESCRIPTION(L"Weekday Test Leap Year")
         TEST_METHOD_ATTRIBUTE(L"Category", L"TimeStamp Test")
      END_TEST_METHOD_ATTRIBUTE()

      TEST_METHOD(WeekdayLeapYear)
      {
         // 2016-01-01 00:00:00
         struct tm tms = {0, 0, 0, 1, 0, 116};
         tstamp_t tstamp(2016, 1, 1, 0, 0, 0, tz_offset);

         time_t ts = tstamp.mktime();

         // VS CPP unit doesn't provide string conversion for signed 64-bit integers, which is what time_t is
         mstest::Assert::AreEqual((unsigned long long) mktime(&tms), (unsigned long long) ts, L"Start UTC time of a leap year should match that from the standard mktime return value");

         // go through all days of the year and check that weekdays match those returned by C runtime
         for(int i = 0; i < 365; i++, ts += 86400) {
            tstamp.reset(ts);

            // see WeekdayNonLeapYear
            mstest::Assert::AreEqual((u_int) gmtime(&ts)->tm_wday, tstamp.wday(), L"Weekday of a leap year must match that from the standard gmtime function");
         }
      }

      BEGIN_TEST_METHOD_ATTRIBUTE(TimeStampParseUTC)
         TEST_DESCRIPTION(L"Weekday Test Non-Leap Year")
         TEST_METHOD_ATTRIBUTE(L"Category", L"TimeStamp Test")
      END_TEST_METHOD_ATTRIBUTE()

      TEST_METHOD(TimeStampParseUTC)
      {
         tstamp_t tstamp_utc(2017, 8, 30, 15, 10, 25);
         tstamp_t tstamp;

         mstest::Assert::IsTrue(tstamp.parse("2017-08-30 15:10:25"), L"2017-08-30 15:10:25 UTC should be parsed without an error");
         mstest::Assert::IsTrue(tstamp_utc == tstamp, L"Parsed 2017-08-30 15:10:25 UTC should compare equal to a constructed time stamp");

         mstest::Assert::IsTrue(tstamp.parse("2017-08-30T15:10:25"), L"2017-08-30T15:10:25 UTC should be parsed without an error");
         mstest::Assert::IsTrue(tstamp_utc == tstamp, L"Parsed 2017-08-30T15:10:25 UTC should compare equal to a constructed time stamp");

         mstest::Assert::IsTrue(tstamp.parse("2017-08-30T15:10"), L"2017-08-30T15:10 UTC should be parsed without an error");
         mstest::Assert::IsTrue(!tstamp_utc.compare(tstamp, tstamp_t::tm_parts::DATE | tstamp_t::tm_parts::HOUR | tstamp_t::tm_parts::MINUTE), L"Parsed 2017-08-30T15:10 UTC should compare equal to a constructed time stamp");

         mstest::Assert::IsTrue(tstamp.parse("2017-08-30"), L"2017-08-30 UTC should be parsed without an error");
         mstest::Assert::IsTrue(!tstamp_utc.compare(tstamp, tstamp_t::tm_parts::DATE), L"Parsed 2017-08-30 UTC should compare equal to a constructed time stamp");

         mstest::Assert::IsTrue(tstamp.parse("2017/08/30"), L"2017/08/30 UTC should be parsed without an error");
         mstest::Assert::IsTrue(!tstamp_utc.compare(tstamp, tstamp_t::tm_parts::DATE), L"Parsed 2017/08/30 UTC should compare equal to a constructed time stamp");
      }

      BEGIN_TEST_METHOD_ATTRIBUTE(TimeStampParseLocal)
         TEST_DESCRIPTION(L"Weekday Test Non-Leap Year")
         TEST_METHOD_ATTRIBUTE(L"Category", L"TimeStamp Test")
      END_TEST_METHOD_ATTRIBUTE()

      TEST_METHOD(TimeStampParseLocal)
      {
         tstamp_t tstamp_local(2017, 8, 30, 15, 10, 25, tz_offset);
         tstamp_t tstamp;

         mstest::Assert::IsTrue(tstamp.parse("2017-08-30 15:10:25", tz_offset), L"2017-08-30 15:10:25 local time should be parsed without an error");
         mstest::Assert::IsTrue(tstamp_local == tstamp, L"Parsed 2017-08-30 15:10:25 local time should compare equal to a constructed time stamp");

         mstest::Assert::IsTrue(tstamp.parse("2017-08-30T15:10:25", tz_offset), L"2017-08-30T15:10:25 local time should be parsed without an error");
         mstest::Assert::IsTrue(tstamp_local == tstamp, L"Parsed 2017-08-30T15:10:25 local time should compare equal to a constructed time stamp");

         mstest::Assert::IsTrue(tstamp.parse("2017-08-30T15:10", tz_offset), L"2017-08-30T15:10 local time should be parsed without an error");
         mstest::Assert::IsTrue(!tstamp_local.compare(tstamp, tstamp_t::tm_parts::DATE | tstamp_t::tm_parts::HOUR | tstamp_t::tm_parts::MINUTE), L"Parsed 2017-08-30T15:10 local time should compare equal to a constructed time stamp");

         mstest::Assert::IsTrue(tstamp.parse("2017-08-30", tz_offset), L"2017-08-30 local time should be parsed without an error");
         mstest::Assert::IsTrue(!tstamp_local.compare(tstamp, tstamp_t::tm_parts::DATE), L"Parsed 2017-08-30 local time should compare equal to a constructed time stamp");

         mstest::Assert::IsTrue(tstamp.parse("2017/08/30", tz_offset), L"2017/08/30 local time should be parsed without an error");
         mstest::Assert::IsTrue(!tstamp_local.compare(tstamp, tstamp_t::tm_parts::DATE), L"Parsed 2017/08/30 local time should compare equal to a constructed time stamp");
      }
};
}

