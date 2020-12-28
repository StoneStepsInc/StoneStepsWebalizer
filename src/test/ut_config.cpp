/*
   webalizer - a web server log analysis program

   Copyright (c) 2004-2021, Stone Steps Inc. (www.stonesteps.ca)

   See COPYING and Copyright files for additional licensing and copyright information 
   
   ut_config.cpp
*/
#include "pch.h"

#include "../config.h"
#include "../tstring.h"
#include "../tmranges.h"
#include "../util_url.h"

#include <vector>

namespace sswtest {

///
/// @brief  Configuration tests.
///
class ConfigTest : public testing::Test {
   protected:
      config_t    config;
};

///
/// @brief  Tests how well HTTP port values map to URL types.
///
TEST_F(ConfigTest, URLTypePort)
{
   // test standard ports
   config.http_port = 80;
   config.https_port = 443;

   EXPECT_EQ(URL_TYPE_HTTP, config.get_url_type(80)) << "URL type should be reported URL_TYPE_HTTP for port 80";
   EXPECT_EQ(URL_TYPE_HTTPS, config.get_url_type(443)) << "URL type should be reported URL_TYPE_HTTPS for port 443";
   EXPECT_EQ(URL_TYPE_OTHER, config.get_url_type(8080)) << "URL type should be reported URL_TYPE_OTHER for port 8080";
   EXPECT_EQ(URL_TYPE_UNKNOWN, config.get_url_type(0)) << "URL type should be reported URL_TYPE_UNKNOWN for unknown ports identified as a zero";

   // make sure ports are not hard-coded in the configuration
   config.http_port = 8080;
   config.https_port = 8081;

   EXPECT_EQ(URL_TYPE_HTTP, config.get_url_type(8080)) << "URL type should be reported URL_TYPE_HTTP for port 8080";
   EXPECT_EQ(URL_TYPE_HTTPS, config.get_url_type(8081)) << "URL type should be reported URL_TYPE_HTTPS for port 8081";
   EXPECT_EQ(URL_TYPE_OTHER, config.get_url_type(8090)) << "URL type should be reported URL_TYPE_OTHER for port 8090";
   EXPECT_EQ(URL_TYPE_UNKNOWN, config.get_url_type(0)) << "URL type should be reported URL_TYPE_UNKNOWN for unknown ports identified as a zero";
}

///
/// @brief  Tests how UseHTTPS setting affects which which URL types are secure.
///
TEST_F(ConfigTest, SecureURLType)
{
   config.use_https = false;

   EXPECT_TRUE(config.is_secure_url(URL_TYPE_HTTPS)) << "URL_TYPE_HTTPS is a secure URL type if UseHTTPS is set to 'no'";

   EXPECT_FALSE(config.is_secure_url(URL_TYPE_HTTP)) << "URL_TYPE_HTTP is not a secure URL type if UseHTTPS is set to 'no'";
   EXPECT_FALSE(config.is_secure_url(URL_TYPE_OTHER)) << "URL_TYPE_OTHER is not a secure URL type if UseHTTPS is set to 'no'";
   EXPECT_FALSE(config.is_secure_url(URL_TYPE_HTTP | URL_TYPE_OTHER)) << "URL_TYPE_HTTP | URL_TYPE_OTHER is not a secure URL type if UseHTTPS is set to 'no'";
   EXPECT_FALSE(config.is_secure_url(URL_TYPE_HTTPS | URL_TYPE_OTHER)) << "URL_TYPE_HTTPS | URL_TYPE_OTHER is not a secure URL type if UseHTTPS is set to 'no'";
   EXPECT_FALSE(config.is_secure_url(URL_TYPE_HTTP | URL_TYPE_HTTPS)) << "URL_TYPE_HTTP | URL_TYPE_HTTPS is not a secure URL type if UseHTTPS is set to 'no'";
   EXPECT_FALSE(config.is_secure_url(URL_TYPE_HTTP | URL_TYPE_HTTPS | URL_TYPE_OTHER)) << "URL_TYPE_HTTP | URL_TYPE_HTTPS | URL_TYPE_OTHER is not a secure URL type if UseHTTPS is set to 'no'";
   EXPECT_FALSE(config.is_secure_url(URL_TYPE_UNKNOWN)) << "URL_TYPE_UNKNOWN is not a secure URL type if UseHTTPS is set to 'no'";

   config.use_https = true;

   EXPECT_TRUE(config.is_secure_url(URL_TYPE_HTTPS)) << "URL_TYPE_HTTPS is a secure URL type if UseHTTPS is set to 'yes'";
   EXPECT_TRUE(config.is_secure_url(URL_TYPE_HTTPS | URL_TYPE_OTHER)) << "URL_TYPE_HTTPS | URL_TYPE_OTHER is a secure URL type if UseHTTPS is set to 'yes'";
   EXPECT_TRUE(config.is_secure_url(URL_TYPE_HTTP | URL_TYPE_HTTPS)) << "URL_TYPE_HTTP | URL_TYPE_HTTPS is a secure URL type if UseHTTPS is set to 'yes'";
   EXPECT_TRUE(config.is_secure_url(URL_TYPE_HTTP | URL_TYPE_HTTPS | URL_TYPE_OTHER)) << "URL_TYPE_HTTP | URL_TYPE_HTTPS | URL_TYPE_OTHER is a secure URL type if UseHTTPS is set to 'yes'";
   EXPECT_TRUE(config.is_secure_url(URL_TYPE_UNKNOWN)) << "URL_TYPE_UNKNOWN is a secure URL type if UseHTTPS is set to 'yes'";

   EXPECT_FALSE(config.is_secure_url(URL_TYPE_HTTP)) << "URL_TYPE_HTTP is not a secure URL type if UseHTTPS is set to 'yes'";
   EXPECT_FALSE(config.is_secure_url(URL_TYPE_OTHER)) << "URL_TYPE_OTHER is not a secure URL type if UseHTTPS is set to 'yes'";
   EXPECT_FALSE(config.is_secure_url(URL_TYPE_HTTP | URL_TYPE_OTHER)) << "URL_TYPE_HTTP | URL_TYPE_OTHER is not a secure URL type if UseHTTPS is set to 'yes'";
}

///
/// @brief  Tests if a URL is a page or not
///
TEST_F(ConfigTest, PageType)
{
   config.page_type.add_nlist("htm");
   config.page_type.add_nlist("html");
   config.page_type.add_nlist("txt");

   // empty URL is considered a directory with an index page
   EXPECT_TRUE(config.ispage(string_t("")));

   // a directory is associated with an index page
   EXPECT_TRUE(config.ispage(string_t("/")));
   EXPECT_TRUE(config.ispage(string_t("/abc/")));
   EXPECT_TRUE(config.ispage(string_t("/abc/xyz/")));

   // test page URLs
   EXPECT_TRUE(config.ispage(string_t("/abc/page.htm")));
   EXPECT_TRUE(config.ispage(string_t("/page.html")));
   EXPECT_TRUE(config.ispage(string_t("/abc/xyz/page.txt")));

   // test non-page URLs
   EXPECT_FALSE(config.ispage(string_t("/abc/image.png")));
   EXPECT_FALSE(config.ispage(string_t("/image.jpg")));
   EXPECT_FALSE(config.ispage(string_t("/abc/xyz/image.jpeg")));
}

///
/// @brief  Tests input for time interval values.
///
TEST_F(ConfigTest, GetInterval)
{
   std::vector<string_t> errors;

   // nullptr string pointer
   EXPECT_EQ(0, config.get_interval(nullptr, errors));
   ASSERT_TRUE(errors.empty()) << "A nullptr pointer should not generate any errors";
   
   // empty string
   EXPECT_EQ(0, config.get_interval("", errors));
   ASSERT_TRUE(errors.empty()) << "An empty string should not generate any errors";

   // default seconds
   EXPECT_EQ(123, config.get_interval("123", errors));
   ASSERT_TRUE(errors.empty()) << "Plain number should not generate any errors";

   // seconds
   EXPECT_EQ(123, config.get_interval("123s", errors));
   ASSERT_TRUE(errors.empty()) << "A number with a suffix 's' should not generate any errors";

   EXPECT_EQ(123, config.get_interval("123 seconds", errors));
   ASSERT_TRUE(errors.empty()) << "A number with a suffix ' seconds' should not generate any errors";

   // minutes
   EXPECT_EQ(123*60, config.get_interval("123m", errors));
   ASSERT_TRUE(errors.empty()) << "A number with a suffix 'm' should not generate any errors";

   EXPECT_EQ(123*60, config.get_interval("123 minutes", errors));
   ASSERT_TRUE(errors.empty()) << "A number with a suffix ' minutes' should not generate any errors";

   // hours
   EXPECT_EQ(123*60*60, config.get_interval("123h", errors));
   ASSERT_TRUE(errors.empty()) << "A number with a suffix 'h' should not generate any errors";

   EXPECT_EQ(123*60*60, config.get_interval("123 hours", errors));
   ASSERT_TRUE(errors.empty()) << "A number with a suffix ' hours' should not generate any errors";

   // not a number
   EXPECT_EQ(0, config.get_interval("bad", errors));
   EXPECT_TRUE(!errors.empty()) << "A string 'bad' is expected to fail";
   errors.clear();

   // bad suffix
   EXPECT_EQ(0, config.get_interval("123bad", errors));
   EXPECT_TRUE(!errors.empty()) << "A number followed by a string 'bad' is expected to fail";
   errors.clear();

   EXPECT_EQ(0, config.get_interval("123 bad", errors));
   EXPECT_TRUE(!errors.empty()) << "A number followed by a string ' bad' is expected to fail";
   errors.clear();
}

///
/// @brief  Tests UTC/DST offsets for a few DST ranges
///
TEST_F(ConfigTest, DSTRanges)
{
   static const int utc_offset = -5 * 60;    // -5h in minutes
   static const int dst_offset = +1 * 60;    // +1h in minutes

   // set Eastern Time UTC/DST offsets
   config.utc_offset = utc_offset;
   config.dst_offset = dst_offset;

   // pass start/end time stamps individually
   config.set_dst_range("2007/03/11 2:00", nullptr);
   config.set_dst_range(nullptr, "2007/11/04 2:00");

   config.set_dst_range("2008/03/09 2:00", nullptr);
   config.set_dst_range(nullptr, "2008/11/02 2:00");

   // out of order range should be processed in the right order
   config.set_dst_range("2017/03/12 2:00", nullptr);
   config.set_dst_range(nullptr, "2017/11/05 2:00");

   // pass start/end time stamps in one call
   config.set_dst_range("2009/03/09 2:00", "2009/11/01 2:00");
   config.set_dst_range("2010/03/09 2:00", "2010/11/01 2:00");

   //
   // Process time stamp input and populate DST time ranges
   //
   config.proc_dst_ranges();
   ASSERT_TRUE(config.errors.empty()) << "DST ranges must be processed without any errors";

   //
   // Log time stamps must be in ascending order, so we can skip processed ranges
   //
   tm_ranges_t::iterator dst_iter = config.dst_ranges.begin();

   struct {
      int expect_offset;
      const tstamp_t tstamp;
      const char *test_desc;
   } test_data[] = {
      // 2007 EST before EDT
      {utc_offset, {2007, 2, 15, 0, 0, 0, utc_offset}, "Well before the 2007 DST start"},
      {utc_offset, {2007, 3, 11, 1, 59, 59, utc_offset}, "One second before the 2007 DST start"},

      // 2007 EDT
      {utc_offset + dst_offset, {2007, 3, 11, 2, 0, 0, utc_offset}, "First second of the 2007 DST range"},
      {utc_offset + dst_offset, {2007, 4, 15, 0, 0, 0, utc_offset + dst_offset}, "Well after the 2007 DST start"},
      {utc_offset + dst_offset, {2007, 11, 4, 1, 59, 59, utc_offset + dst_offset}, "One second before the end of the 2007 DST range"},

      // 2007 EST after EDT
      {utc_offset, {2007, 11, 4, 2, 0, 1, utc_offset}, "One second after the 2007 DST end"},
      {utc_offset, {2007, 12, 15, 0, 0, 0, utc_offset}, "Well after the 2007 DST end"},

      // 2008 EST before EDT
      {utc_offset, {2008, 2, 15, 0, 0, 0, utc_offset}, "Well before the 2008 DST start"},
      {utc_offset, {2008, 3, 9, 1, 59, 59}, "One second before the 2008 DST start"},

      // 2008 EDT
      {utc_offset + dst_offset, {2008, 3, 9, 2, 0, 0, utc_offset}, "First second of the 2008 DST range"},
      {utc_offset + dst_offset, {2008, 4, 15, 0, 0, 0, utc_offset + dst_offset}, "Well after the 2008 DST start"},
      {utc_offset + dst_offset, {2008, 11, 2, 1, 59, 59, utc_offset + dst_offset}, "One second before the end of the 2008 DST range"},

      // 2008 EST after EDT
      {utc_offset, {2008, 11, 2, 2, 0, 1, utc_offset}, "One second after the 2008 DST start"},
      {utc_offset, {2008, 12, 15, 0, 0, 0, utc_offset}, "Well after the 2008 DST end"},

      // 2009 EST before EDT
      {utc_offset, {2009, 2, 15, 0, 0, 0, utc_offset}, "Well before the 2009 DST start"},
      {utc_offset, {2009, 3, 9, 1, 59, 59, utc_offset}, "One second before the 2009 DST start"},

      // 2009 EDT
      {utc_offset + dst_offset, {2009, 3, 9, 2, 0, 0, utc_offset}, "Start of the 2009 DST range"},
      {utc_offset + dst_offset, {2009, 4, 15, 0, 0, 0, utc_offset + dst_offset}, "Well after the 2009 DST start"},
      {utc_offset + dst_offset, {2009, 11, 1, 1, 59, 59, utc_offset + dst_offset}, "One second before the end of the 2009 DST range"},

      // 2009 EST after EDT
      {utc_offset, {2009, 11, 1, 2, 0, 1, utc_offset}, "One second after the 2009 DST range"},
      {utc_offset, {2009, 12, 15, 0, 0, 0, utc_offset}, "Well after the 2009 DST end"},

      // 2017 EST before EDT
      {utc_offset, {2017, 2, 15, 0, 0, 0, utc_offset}, "Well before the 2017 DST start"},
      {utc_offset, {2017, 3, 12, 1, 59, 59, utc_offset}, "One second before the 2017 DST start"},

      // 2017 EDT
      {utc_offset + dst_offset, {2017, 3, 12, 2, 0, 0, utc_offset}, "Start of the 2017 DST range"},
      {utc_offset + dst_offset, {2017, 4, 15, 0, 0, 0, utc_offset + dst_offset}, "Well after the 2017 DST start"},
      {utc_offset + dst_offset, {2017, 11, 5, 1, 59, 59, utc_offset + dst_offset}, "One second before the end of the 2017 DST range"},

      // 2017 EST after EDT
      {utc_offset, {2017, 11, 5, 2, 0, 1, utc_offset}, "One second after the 2017 DST range"},
      {utc_offset, {2017, 12, 15, 0, 0, 0, utc_offset}, "Well after the 2017 DST end"},
   };

   // check expected time zone offset values for all test data points
   for(size_t i = 0; i < sizeof(test_data)/sizeof(test_data[0]); i++)
      EXPECT_EQ(test_data[i].expect_offset, config.get_utc_offset(test_data[i].tstamp, dst_iter));
}

}
