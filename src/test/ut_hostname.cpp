/*
   webalizer - a web server log analysis program

   Copyright (c) 2004-2018, Stone Steps Inc. (www.stonesteps.ca)

   See COPYING and Copyright files for additional licensing and copyright information 
   
   ut_hostname.cpp
*/
#include "pchtest.h"

#include "../util_url.h"
#include "../tstring.h"

//
// URLHostNameParser
//
namespace sswtest {
class URLHostNameParser : public testing::Test {
   protected:
      string_t GetHost(const char *url)
      {
         string_t::const_char_buffer_t host = get_url_host(url, strlen(url));
         return string_t(host, host.capacity());
      }
};

///
/// @brief  Extract the host name from a well-formed HTTP URL
///
TEST_F(URLHostNameParser, GoodURLHTTP)
{
   // bare URL
   EXPECT_STREQ("test.stonesteps.ca", GetHost("http://test.stonesteps.ca/"));
   EXPECT_STREQ("test.stonesteps.ca", GetHost("http://test.stonesteps.ca"));

   // user name
   EXPECT_STREQ("test.stonesteps.ca", GetHost("http://user@test.stonesteps.ca/"));
   EXPECT_STREQ("test.stonesteps.ca", GetHost("http://user@test.stonesteps.ca"));

   // user name, password
   EXPECT_STREQ("test.stonesteps.ca", GetHost("http://user:password@test.stonesteps.ca/"));
   EXPECT_STREQ("test.stonesteps.ca", GetHost("http://user:password@test.stonesteps.ca"));

   // port
   EXPECT_STREQ("test.stonesteps.ca", GetHost("http://test.stonesteps.ca:8080/"));
   EXPECT_STREQ("test.stonesteps.ca", GetHost("http://test.stonesteps.ca:8080"));

   // user name, port
   EXPECT_STREQ("test.stonesteps.ca", GetHost("http://user@test.stonesteps.ca:8080/"));
   EXPECT_STREQ("test.stonesteps.ca", GetHost("http://user@test.stonesteps.ca:8080"));

   // user name, password port
   EXPECT_STREQ("test.stonesteps.ca", GetHost("http://user:password@test.stonesteps.ca:8080/"));
   EXPECT_STREQ("test.stonesteps.ca", GetHost("http://user:password@test.stonesteps.ca:8080"));
}

///
/// @brief  Extract the host name from a well-formed HTTP SSL URL
///
TEST_F(URLHostNameParser, GoodURLSSL)
{
   // bare URL
   EXPECT_STREQ("test.stonesteps.ca", GetHost("https://test.stonesteps.ca/"));
   EXPECT_STREQ("test.stonesteps.ca", GetHost("https://test.stonesteps.ca"));

   // user name
   EXPECT_STREQ("test.stonesteps.ca", GetHost("https://user@test.stonesteps.ca/"));
   EXPECT_STREQ("test.stonesteps.ca", GetHost("https://user@test.stonesteps.ca"));

   // user name, password
   EXPECT_STREQ("test.stonesteps.ca", GetHost("https://user:password@test.stonesteps.ca/"));
   EXPECT_STREQ("test.stonesteps.ca", GetHost("https://user:password@test.stonesteps.ca"));

   // port
   EXPECT_STREQ("test.stonesteps.ca", GetHost("https://test.stonesteps.ca:8080/"));
   EXPECT_STREQ("test.stonesteps.ca", GetHost("https://test.stonesteps.ca:8080"));

   // user name, port
   EXPECT_STREQ("test.stonesteps.ca", GetHost("https://user@test.stonesteps.ca:8080/"));
   EXPECT_STREQ("test.stonesteps.ca", GetHost("https://user@test.stonesteps.ca:8080"));

   // user name, password, port
   EXPECT_STREQ("test.stonesteps.ca", GetHost("https://user:password@test.stonesteps.ca:8080/"));
   EXPECT_STREQ("test.stonesteps.ca", GetHost("https://user:password@test.stonesteps.ca:8080"));
}

///
/// @brief  Extract the host name from a well-formed URL without a scheme.
///
TEST_F(URLHostNameParser, GoodURLNoScheme)
{
   // bare URL
   EXPECT_STREQ("test.stonesteps.ca", GetHost("//test.stonesteps.ca/"));
   EXPECT_STREQ("test.stonesteps.ca", GetHost("//test.stonesteps.ca"));

   // user name
   EXPECT_STREQ("test.stonesteps.ca", GetHost("//user@test.stonesteps.ca/"));
   EXPECT_STREQ("test.stonesteps.ca", GetHost("//user@test.stonesteps.ca"));

   // user name, password
   EXPECT_STREQ("test.stonesteps.ca", GetHost("//user:password@test.stonesteps.ca/"));
   EXPECT_STREQ("test.stonesteps.ca", GetHost("//user:password@test.stonesteps.ca"));

   // port
   EXPECT_STREQ("test.stonesteps.ca", GetHost("//test.stonesteps.ca:8080/"));
   EXPECT_STREQ("test.stonesteps.ca", GetHost("//test.stonesteps.ca:8080"));

   // user name, port
   EXPECT_STREQ("test.stonesteps.ca", GetHost("//user@test.stonesteps.ca:8080/"));
   EXPECT_STREQ("test.stonesteps.ca", GetHost("//user@test.stonesteps.ca:8080"));

   // user name, password port
   EXPECT_STREQ("test.stonesteps.ca", GetHost("//user:password@test.stonesteps.ca:8080/"));
   EXPECT_STREQ("test.stonesteps.ca", GetHost("//user:password@test.stonesteps.ca:8080"));
}

///
/// @brief  A malformed URL should produce an empty host
///
TEST_F(URLHostNameParser, BadURL)
{
   EXPECT_TRUE(GetHost("http://").isempty()) << "http:// should fail";
   EXPECT_TRUE(GetHost("//").isempty()) << "// should fail";
   EXPECT_TRUE(GetHost("abc://test.stonesteps.ca").isempty()) << "abc://test.stonesteps.ca should fail";
   EXPECT_TRUE(GetHost("test.stonesteps.ca").isempty()) << "test.stonesteps.ca should fail";
}
}
