/*
   webalizer - a web server log analysis program

   Copyright (c) 2004-2017, Stone Steps Inc. (www.stonesteps.ca)

   See COPYING and Copyright files for additional licensing and copyright information 
   
   ut_hostname.cpp
*/
#include "pchtest.h"

#include "../util_url.h"
#include "../tstring.h"

namespace mstest = Microsoft::VisualStudio::CppUnitTestFramework;

//
// URLHostNameParser
//
namespace sswtest {
TEST_CLASS(URLHostNameParser) {
   private:
      string_t GetHost(const char *url)
      {
         string_t::const_char_buffer_t host = get_url_host(url, strlen(url));
         return string_t(host, host.capacity());
      }

   public:
       BEGIN_TEST_METHOD_ATTRIBUTE(GoodURL)
         TEST_DESCRIPTION(L"Extract the host name from a well-formed URL")
         TEST_METHOD_ATTRIBUTE(L"Category", L"URL")
       END_TEST_METHOD_ATTRIBUTE()

       TEST_METHOD(GoodURL)
       {
         //
         // HTTP
         //

         // bare URL
         mstest::Assert::AreEqual("test.stonesteps.ca", GetHost("http://test.stonesteps.ca/"));
         mstest::Assert::AreEqual("test.stonesteps.ca", GetHost("http://test.stonesteps.ca"));

         // user name
         mstest::Assert::AreEqual("test.stonesteps.ca", GetHost("http://user@test.stonesteps.ca/"));
         mstest::Assert::AreEqual("test.stonesteps.ca", GetHost("http://user@test.stonesteps.ca"));

         // user name, password
         mstest::Assert::AreEqual("test.stonesteps.ca", GetHost("http://user:password@test.stonesteps.ca/"));
         mstest::Assert::AreEqual("test.stonesteps.ca", GetHost("http://user:password@test.stonesteps.ca"));

         // port
         mstest::Assert::AreEqual("test.stonesteps.ca", GetHost("http://test.stonesteps.ca:8080/"));
         mstest::Assert::AreEqual("test.stonesteps.ca", GetHost("http://test.stonesteps.ca:8080"));

         // user name, port
         mstest::Assert::AreEqual("test.stonesteps.ca", GetHost("http://user@test.stonesteps.ca:8080/"));
         mstest::Assert::AreEqual("test.stonesteps.ca", GetHost("http://user@test.stonesteps.ca:8080"));

         // user name, password port
         mstest::Assert::AreEqual("test.stonesteps.ca", GetHost("http://user:password@test.stonesteps.ca:8080/"));
         mstest::Assert::AreEqual("test.stonesteps.ca", GetHost("http://user:password@test.stonesteps.ca:8080"));

         //
         // HTTP SSL
         //

         // bare URL
         mstest::Assert::AreEqual("test.stonesteps.ca", GetHost("https://test.stonesteps.ca/"));
         mstest::Assert::AreEqual("test.stonesteps.ca", GetHost("https://test.stonesteps.ca"));

         // user name
         mstest::Assert::AreEqual("test.stonesteps.ca", GetHost("https://user@test.stonesteps.ca/"));
         mstest::Assert::AreEqual("test.stonesteps.ca", GetHost("https://user@test.stonesteps.ca"));

         // user name, password
         mstest::Assert::AreEqual("test.stonesteps.ca", GetHost("https://user:password@test.stonesteps.ca/"));
         mstest::Assert::AreEqual("test.stonesteps.ca", GetHost("https://user:password@test.stonesteps.ca"));

         // port
         mstest::Assert::AreEqual("test.stonesteps.ca", GetHost("https://test.stonesteps.ca:8080/"));
         mstest::Assert::AreEqual("test.stonesteps.ca", GetHost("https://test.stonesteps.ca:8080"));

         // user name, port
         mstest::Assert::AreEqual("test.stonesteps.ca", GetHost("https://user@test.stonesteps.ca:8080/"));
         mstest::Assert::AreEqual("test.stonesteps.ca", GetHost("https://user@test.stonesteps.ca:8080"));

         // user name, password, port
         mstest::Assert::AreEqual("test.stonesteps.ca", GetHost("https://user:password@test.stonesteps.ca:8080/"));
         mstest::Assert::AreEqual("test.stonesteps.ca", GetHost("https://user:password@test.stonesteps.ca:8080"));

         //
         // no URL scheme
         //

         // bare URL
         mstest::Assert::AreEqual("test.stonesteps.ca", GetHost("//test.stonesteps.ca/"));
         mstest::Assert::AreEqual("test.stonesteps.ca", GetHost("//test.stonesteps.ca"));

         // user name
         mstest::Assert::AreEqual("test.stonesteps.ca", GetHost("//user@test.stonesteps.ca/"));
         mstest::Assert::AreEqual("test.stonesteps.ca", GetHost("//user@test.stonesteps.ca"));

         // user name, password
         mstest::Assert::AreEqual("test.stonesteps.ca", GetHost("//user:password@test.stonesteps.ca/"));
         mstest::Assert::AreEqual("test.stonesteps.ca", GetHost("//user:password@test.stonesteps.ca"));

         // port
         mstest::Assert::AreEqual("test.stonesteps.ca", GetHost("//test.stonesteps.ca:8080/"));
         mstest::Assert::AreEqual("test.stonesteps.ca", GetHost("//test.stonesteps.ca:8080"));

         // user name, port
         mstest::Assert::AreEqual("test.stonesteps.ca", GetHost("//user@test.stonesteps.ca:8080/"));
         mstest::Assert::AreEqual("test.stonesteps.ca", GetHost("//user@test.stonesteps.ca:8080"));

         // user name, password port
         mstest::Assert::AreEqual("test.stonesteps.ca", GetHost("//user:password@test.stonesteps.ca:8080/"));
         mstest::Assert::AreEqual("test.stonesteps.ca", GetHost("//user:password@test.stonesteps.ca:8080"));

      }

       BEGIN_TEST_METHOD_ATTRIBUTE(BadURL)
         TEST_DESCRIPTION(L"A malformed URL should produce an empty host")
         TEST_METHOD_ATTRIBUTE(L"Category", L"URL")
       END_TEST_METHOD_ATTRIBUTE()

       TEST_METHOD(BadURL)
       {
         mstest::Assert::IsTrue(GetHost("http://").isempty(), L"http:// should fail");
         mstest::Assert::IsTrue(GetHost("//").isempty(), L"// should fail");
         mstest::Assert::IsTrue(GetHost("abc://test.stonesteps.ca").isempty(), L"abc://test.stonesteps.ca should fail");
         mstest::Assert::IsTrue(GetHost("test.stonesteps.ca").isempty(), L"test.stonesteps.ca should fail");
      }
   };
}
