/*
   webalizer - a web server log analysis program

   Copyright (c) 2004-2017, Stone Steps Inc. (www.stonesteps.ca)

   See COPYING and Copyright files for additional licensing and copyright information 
   
   ut_strsrch.cpp
*/
#include "pchtest.h"

#include "../util.h"
#include "../tstring.h"

namespace mstest = Microsoft::VisualStudio::CppUnitTestFramework;

//
// URLHostNameParser
//
namespace sswtest {
TEST_CLASS(URLHostNameParser) {
   public:
      BEGIN_TEST_METHOD_ATTRIBUTE(SubStrDeltaSearch)
         TEST_DESCRIPTION(L"Sub-string search with a delta table")
         TEST_METHOD_ATTRIBUTE(L"Category", L"String")
      END_TEST_METHOD_ATTRIBUTE()

      TEST_METHOD(SubStrDeltaSearch)
      {
         bmh_delta_table dt;

         //                      1         2
         //            012345678901234567890123456
         string_t str("abc ABCD acd xbcd abcd abcd");
         string_t srch("abcd");

         dt.reset(srch);

         //
         // Look for a sub-string that is present in the string using
         //
         mstest::Assert::AreEqual(18, strstr_ex(str, srch, &dt) - str.c_str(), L"String and sub-string are null-terminated");
         mstest::Assert::AreEqual(18, strstr_ex(str, srch, str.length(), &dt) - str.c_str(), L"Sub-string is null-terminated");
         mstest::Assert::AreEqual(18, strstr_ex(str, srch, str.length(), srch.length(), &dt) - str.c_str(), L"String and sub-string lengths are not zeroes");
         mstest::Assert::AreEqual(18, strstr_ex(str, "abcdef", str.length(), srch.length(), &dt) - str.c_str(), L"Search is using only a part of the sub-string");

         //
         // Search the start of the string without the sub-seting
         //
         mstest::Assert::IsNull(strstr_ex(str, "abcdef", 21, srch.length(), &dt), L"Search a part of the string without the sub-string");

         // change the sub-string, so it's not present in the string
         srch = "xyz";
         dt.reset(srch);

         //
         // Look for a sub-string that is not present in the string
         //
         mstest::Assert::IsNull(strstr_ex(str, srch, &dt), L"String and sub-string are null-terminated");
         mstest::Assert::IsNull(strstr_ex(str, srch, str.length(),  &dt), L"Sub-string is null-terminated");
         mstest::Assert::IsNull(strstr_ex(str, srch, str.length(), srch.length(), &dt), L"String and sub-string lengths are not zeroes");
      }

      BEGIN_TEST_METHOD_ATTRIBUTE(SubStrSearch)
         TEST_DESCRIPTION(L"Sub-string search without a delta table")
         TEST_METHOD_ATTRIBUTE(L"Category", L"String")
      END_TEST_METHOD_ATTRIBUTE()

      TEST_METHOD(SubStrSearch)
      {
         //                      1         2
         //            012345678901234567890123456
         string_t str("abc ABCD acd xbcd abcd abcd");
         string_t srch("abcd");

         //
         // Look for a sub-string that is present in the string using
         //
         mstest::Assert::AreEqual(18, strstr_ex(str, srch, nullptr) - str.c_str(), L"String and sub-string are null-terminated");
         mstest::Assert::AreEqual(18, strstr_ex(str, srch, str.length(), nullptr) - str.c_str(), L"Sub-string is null-terminated");
         mstest::Assert::AreEqual(18, strstr_ex(str, srch, str.length(), srch.length(), nullptr) - str.c_str(), L"String and sub-string lengths are not zeroes");
         mstest::Assert::AreEqual(18, strstr_ex(str, "abcdef", str.length(), srch.length(), nullptr) - str.c_str(), L"Search is using only a part of the sub-string");

         //
         // Search the start of the string without the sub-seting
         //
         mstest::Assert::IsNull(strstr_ex(str, "abcdef", 21, srch.length(), nullptr), L"Search a part of the string without the sub-string");

         // change the sub-string, so it's not present in the string
         srch = "xyz";

         //
         // Look for a sub-string that is not present in the string
         //
         mstest::Assert::IsNull(strstr_ex(str, srch, nullptr), L"String and sub-string are null-terminated");
         mstest::Assert::IsNull(strstr_ex(str, srch, str.length(), nullptr), L"Sub-string is null-terminated");
         mstest::Assert::IsNull(strstr_ex(str, srch, str.length(), srch.length(), nullptr), L"String and sub-string lengths are not zeroes");
      }

      BEGIN_TEST_METHOD_ATTRIBUTE(SubStrEmptyOrNull)
         TEST_DESCRIPTION(L"Match empty or NULL strings and sub-strings")
         TEST_METHOD_ATTRIBUTE(L"Category", L"String")
      END_TEST_METHOD_ATTRIBUTE()

      TEST_METHOD(SubStrEmptyOrNull)
      {
         string_t str("abc ABCD acd xbcd abcd abcd");
         string_t srch("abcd");

         // all searches with empty strings must fail
         mstest::Assert::IsNull(strstr_ex(str, srch, 0, 0, nullptr), L"Fail to match if the string and the sub-string are empty");
         mstest::Assert::IsNull(strstr_ex(str, srch, str.length(), 0, nullptr), L"Fail to match if the sub-string is empty");
         mstest::Assert::IsNull(strstr_ex(nullptr, nullptr, 0, 0, nullptr), L"Fail to match if the string and the sub-string are NULL");
         mstest::Assert::IsNull(strstr_ex(str, nullptr, str.length(), 0, nullptr), L"Fail to match if the sub-string is NULL");
      }
};
}