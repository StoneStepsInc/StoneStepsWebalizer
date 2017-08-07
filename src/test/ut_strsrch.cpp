/*
   webalizer - a web server log analysis program

   Copyright (c) 2004-2017, Stone Steps Inc. (www.stonesteps.ca)

   See COPYING and Copyright files for additional licensing and copyright information 
   
   ut_strsrch.cpp
*/
#include "pchtest.h"

#include "../util_string.h"
#include "../tstring.h"

namespace mstest = Microsoft::VisualStudio::CppUnitTestFramework;

//
// StringSearch
//
namespace sswtest {
TEST_CLASS(StringSearch) {
   public:
      BEGIN_TEST_METHOD_ATTRIBUTE(SubStrDeltaSearch)
         TEST_DESCRIPTION(L"Sub-string search with a delta table")
         TEST_METHOD_ATTRIBUTE(L"Category", L"Sub-string Search")
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
         // Search the start of the string without the sub-string
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
         TEST_METHOD_ATTRIBUTE(L"Category", L"Sub-string Search")
      END_TEST_METHOD_ATTRIBUTE()

      TEST_METHOD(SubStrSearch)
      {
         //                      1         2
         //            012345678901234567890123456
         string_t str("abc ABCD acd xbcd abcd abcd");
         string_t srch("abcd");

         //
         // Look for a sub-string that is present in the string without a delta table
         //
         mstest::Assert::AreEqual(18, strstr_ex(str, srch, nullptr) - str.c_str(), L"String and sub-string are null-terminated");
         mstest::Assert::AreEqual(18, strstr_ex(str, srch, str.length(), nullptr) - str.c_str(), L"Sub-string is null-terminated");
         mstest::Assert::AreEqual(18, strstr_ex(str, srch, str.length(), srch.length(), nullptr) - str.c_str(), L"String and sub-string lengths are not zeroes");
         mstest::Assert::AreEqual(18, strstr_ex(str, "abcdef", str.length(), srch.length(), nullptr) - str.c_str(), L"Search is using only a part of the sub-string");

         //
         // Search the start of the string without the sub-string
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

      BEGIN_TEST_METHOD_ATTRIBUTE(SubStrDeltaSearchNoCase)
         TEST_DESCRIPTION(L"Case-insensitive sub-string search with a delta table")
         TEST_METHOD_ATTRIBUTE(L"Category", L"Sub-string Search")
      END_TEST_METHOD_ATTRIBUTE()

      TEST_METHOD(SubStrDeltaSearchNoCase)
      {
         bmh_delta_table dt;

         //                      1         2
         //            012345678901234567890123456
         string_t str("abc ABCX acd xbcd abcd abcd");
         string_t srch("aBCd");

         dt.reset(srch);

         //
         // Look for a sub-string that is present in the string using
         //
         mstest::Assert::AreEqual(18, strstr_ex(str, srch, &dt, true) - str.c_str(), L"String and sub-string are null-terminated");
         mstest::Assert::AreEqual(18, strstr_ex(str, srch, str.length(), &dt, true) - str.c_str(), L"Sub-string is null-terminated");
         mstest::Assert::AreEqual(18, strstr_ex(str, srch, str.length(), srch.length(), &dt, true) - str.c_str(), L"String and sub-string lengths are not zeroes");
         mstest::Assert::AreEqual(18, strstr_ex(str, "abcdef", str.length(), srch.length(), &dt, true) - str.c_str(), L"Search is using only a part of the sub-string");

         //
         // Search the start of the string without the sub-string
         //
         mstest::Assert::IsNull(strstr_ex(str, "abcdef", 21, srch.length(), &dt, true), L"Search a part of the string without the sub-string");

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

      BEGIN_TEST_METHOD_ATTRIBUTE(SubStrSearchNoCase)
         TEST_DESCRIPTION(L"Case-insensitive sub-string search without a delta table")
         TEST_METHOD_ATTRIBUTE(L"Category", L"Sub-string Search")
      END_TEST_METHOD_ATTRIBUTE()

      TEST_METHOD(SubStrSearchNoCase)
      {
         //                      1         2
         //            012345678901234567890123456
         string_t str("abc ABCX acd xbcd abcD abcd");
         string_t srch("aBCd");

         //
         // Look for a sub-string that is present in the string without a delta table
         //
         mstest::Assert::AreEqual(18, strstr_ex(str, srch, nullptr, true) - str.c_str(), L"String and sub-string are null-terminated");
         mstest::Assert::AreEqual(18, strstr_ex(str, srch, str.length(), nullptr, true) - str.c_str(), L"Sub-string is null-terminated");
         mstest::Assert::AreEqual(18, strstr_ex(str, srch, str.length(), srch.length(), nullptr, true) - str.c_str(), L"String and sub-string lengths are not zeroes");

         //
         // Search only using a part of a sub-string
         //
         mstest::Assert::AreEqual(18, strstr_ex(str, "abcdef", str.length(), srch.length(), nullptr, true) - str.c_str(), L"Search is using only a part of the sub-string");

         //
         // Search the start of the string without the sub-string
         //
         mstest::Assert::IsNull(strstr_ex(str, "abcdef", 21, srch.length(), nullptr, true), L"Search a part of the string without the sub-string");

         // change the sub-string, so it's not present in the string
         srch = "xyz";

         //
         // Look for a sub-string that is not present in the string
         //
         mstest::Assert::IsNull(strstr_ex(str, srch, nullptr, true), L"String and sub-string are null-terminated");
         mstest::Assert::IsNull(strstr_ex(str, srch, str.length(), nullptr, true), L"Sub-string is null-terminated");
         mstest::Assert::IsNull(strstr_ex(str, srch, str.length(), srch.length(), nullptr, true), L"String and sub-string lengths are not zeroes");
      }

      BEGIN_TEST_METHOD_ATTRIBUTE(SubStrEmptyOrNull)
         TEST_DESCRIPTION(L"Match empty or NULL strings and sub-strings")
         TEST_METHOD_ATTRIBUTE(L"Category", L"Sub-string Search")
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

      BEGIN_TEST_METHOD_ATTRIBUTE(StrPatternMatch)
         TEST_DESCRIPTION(L"Match a string pattern case-sensitively and without a delta table")
         TEST_METHOD_ATTRIBUTE(L"Category", L"String Pattern Match")
      END_TEST_METHOD_ATTRIBUTE()

      TEST_METHOD(StrPatternMatch)
      {
         string_t str("abc ABCD acd xbcd abcd abcd");
         string_t nmstr("Xbc ABCD acd xbcd aXcd abcX");
         string_t bpat("abc ABCD*");
         string_t epat("*abcd abcd");
         string_t spat("abcd");

         // match all patterns
         mstest::Assert::IsTrue(isinstrex(str, bpat, str.length(), bpat.length(), false, nullptr, false), L"Match the beginning of the string");
         mstest::Assert::IsTrue(isinstrex(str, epat, str.length(), epat.length(), false, nullptr, false), L"Match the end of the string");
         mstest::Assert::IsTrue(isinstrex(str, spat, str.length(), spat.length(), true, nullptr, false), L"Match a sub-string within the string");

         // fail to match all patterns
         mstest::Assert::IsFalse(isinstrex(nmstr, bpat, str.length(), bpat.length(), false, nullptr, false), L"Fail to match the beginning of the string");
         mstest::Assert::IsFalse(isinstrex(nmstr, epat, str.length(), epat.length(), false, nullptr, false), L"Fail to match the end of the string");
         mstest::Assert::IsFalse(isinstrex(nmstr, spat, str.length(), spat.length(), true, nullptr, false), L"Fail to match a sub-string within the string");

         // fail to match when either of the arguments is NULL or zero
         mstest::Assert::IsFalse(isinstrex(nmstr, nullptr, str.length(), bpat.length(), false, nullptr, false), L"Fail to match if the pattern is NULL");
         mstest::Assert::IsFalse(isinstrex(nmstr, bpat, str.length(), 0, false, nullptr, false), L"Fail to match if the pattern length is zero");
         mstest::Assert::IsFalse(isinstrex(nullptr, bpat, str.length(), bpat.length(), false, nullptr, false), L"Fail to match if the string is NULL");
         mstest::Assert::IsFalse(isinstrex(nmstr, bpat, 0, bpat.length(), false, nullptr, false), L"Fail to match if the string length is zero");
      }

      BEGIN_TEST_METHOD_ATTRIBUTE(StrPatternMatchDelta)
         TEST_DESCRIPTION(L"Match a string pattern case-sensitively and with a delta table")
         TEST_METHOD_ATTRIBUTE(L"Category", L"String Pattern Match")
      END_TEST_METHOD_ATTRIBUTE()

      TEST_METHOD(StrPatternMatchDelta)
      {
         string_t str("abc ABCD acd xbcd abcd abcd");
         string_t nmstr("Xbc ABCD acd xbcd aXcd abcX");
         string_t bpat("abc ABCD*");
         string_t epat("*abcd abcd");
         string_t spat("abcd");

         bmh_delta_table bdt(bpat);
         bmh_delta_table edt(epat);
         bmh_delta_table sdt(spat);

         // match all patterns
         mstest::Assert::IsTrue(isinstrex(str, bpat, str.length(), bpat.length(), false, &bdt, false), L"Match the beginning of the string");
         mstest::Assert::IsTrue(isinstrex(str, epat, str.length(), epat.length(), false, &edt, false), L"Match the end of the string");
         mstest::Assert::IsTrue(isinstrex(str, spat, str.length(), spat.length(), true, &sdt, false), L"Match a sub-string within the string");

         // fail to match all patterns
         mstest::Assert::IsFalse(isinstrex(nmstr, bpat, str.length(), bpat.length(), false, &bdt, false), L"Fail to match the beginning of the string");
         mstest::Assert::IsFalse(isinstrex(nmstr, epat, str.length(), epat.length(), false, &edt, false), L"Fail to match the end of the string");
         mstest::Assert::IsFalse(isinstrex(nmstr, spat, str.length(), spat.length(), true, &sdt, false), L"Fail to match a sub-string within the string");

         // fail to match when either of the arguments is NULL or zero
         mstest::Assert::IsFalse(isinstrex(nmstr, nullptr, str.length(), bpat.length(), false, &bdt, false), L"Fail to match if the pattern is NULL");
         mstest::Assert::IsFalse(isinstrex(nmstr, bpat, str.length(), 0, false, &bdt, false), L"Fail to match if the pattern length is zero");
         mstest::Assert::IsFalse(isinstrex(nullptr, bpat, str.length(), bpat.length(), false, &bdt, false), L"Fail to match if the string is NULL");
         mstest::Assert::IsFalse(isinstrex(nmstr, bpat, 0, bpat.length(), false, &bdt, false), L"Fail to match if the string length is zero");
      }

      BEGIN_TEST_METHOD_ATTRIBUTE(StrPatternMatchNoCase)
         TEST_DESCRIPTION(L"Match a string pattern case-insensitively and without a delta table")
         TEST_METHOD_ATTRIBUTE(L"Category", L"String Pattern Match")
      END_TEST_METHOD_ATTRIBUTE()

      TEST_METHOD(StrPatternMatchNoCase)
      {
         string_t str("abc ABCD acd xbcd abcd abcd");
         string_t nmstr("Xbc AxCD acd xbcd aXcd abcX");
         string_t bpat("aBC ABCD*");
         string_t epat("*abcd aBCd");
         string_t spat("aBCd");

         // match all patterns
         mstest::Assert::IsTrue(isinstrex(str, bpat, str.length(), bpat.length(), false, nullptr, true), L"Match the beginning of the string");
         mstest::Assert::IsTrue(isinstrex(str, epat, str.length(), epat.length(), false, nullptr, true), L"Match the end of the string");
         mstest::Assert::IsTrue(isinstrex(str, spat, str.length(), spat.length(), true, nullptr, true), L"Match a sub-string within the string");

         // fail to match all patterns
         mstest::Assert::IsFalse(isinstrex(nmstr, bpat, str.length(), bpat.length(), false, nullptr, true), L"Fail to match the beginning of the string");
         mstest::Assert::IsFalse(isinstrex(nmstr, epat, str.length(), epat.length(), false, nullptr, true), L"Fail to match the end of the string");
         mstest::Assert::IsFalse(isinstrex(nmstr, spat, str.length(), spat.length(), true, nullptr, true), L"Fail to match a sub-string within the string");

         // fail to match when either of the arguments is NULL or zero
         mstest::Assert::IsFalse(isinstrex(nmstr, nullptr, str.length(), bpat.length(), false, nullptr, true), L"Fail to match if the pattern is NULL");
         mstest::Assert::IsFalse(isinstrex(nmstr, bpat, str.length(), 0, false, nullptr, true), L"Fail to match if the pattern length is zero");
         mstest::Assert::IsFalse(isinstrex(nullptr, bpat, str.length(), bpat.length(), false, nullptr, true), L"Fail to match if the string is NULL");
         mstest::Assert::IsFalse(isinstrex(nmstr, bpat, 0, bpat.length(), false, nullptr, true), L"Fail to match if the string length is zero");
      }

      BEGIN_TEST_METHOD_ATTRIBUTE(StrPatternMatchDeltaNoCase)
         TEST_DESCRIPTION(L"Match a string pattern case-insensitively and with a delta table")
         TEST_METHOD_ATTRIBUTE(L"Category", L"String Pattern Match")
      END_TEST_METHOD_ATTRIBUTE()

      TEST_METHOD(StrPatternMatchDeltaNoCase)
      {
         string_t str("abc ABCD acd xbcd abcd abcd");
         string_t nmstr("Xbc AxCD acd xbcd aXcd abcX");
         string_t bpat("aBC ABCD*");
         string_t epat("*abcd aBCd");
         string_t spat("aBCd");

         bmh_delta_table bdt(bpat);
         bmh_delta_table edt(epat);
         bmh_delta_table sdt(spat);

         // match all patterns
         mstest::Assert::IsTrue(isinstrex(str, bpat, str.length(), bpat.length(), false, &bdt, true), L"Match the beginning of the string");
         mstest::Assert::IsTrue(isinstrex(str, epat, str.length(), epat.length(), false, &edt, true), L"Match the end of the string");
         mstest::Assert::IsTrue(isinstrex(str, spat, str.length(), spat.length(), true, &sdt, true), L"Match a sub-string within the string");

         // fail to match all patterns
         mstest::Assert::IsFalse(isinstrex(nmstr, bpat, str.length(), bpat.length(), false, &bdt, true), L"Fail to match the beginning of the string");
         mstest::Assert::IsFalse(isinstrex(nmstr, epat, str.length(), epat.length(), false, &edt, true), L"Fail to match the end of the string");
         mstest::Assert::IsFalse(isinstrex(nmstr, spat, str.length(), spat.length(), true, &sdt, true), L"Fail to match a sub-string within the string");

         // fail to match when either of the arguments is NULL or zero
         mstest::Assert::IsFalse(isinstrex(nmstr, nullptr, str.length(), bpat.length(), false, &bdt, true), L"Fail to match if the pattern is NULL");
         mstest::Assert::IsFalse(isinstrex(nmstr, bpat, str.length(), 0, false, &bdt, true), L"Fail to match if the pattern length is zero");
         mstest::Assert::IsFalse(isinstrex(nullptr, bpat, str.length(), bpat.length(), false, &bdt, true), L"Fail to match if the string is NULL");
         mstest::Assert::IsFalse(isinstrex(nmstr, bpat, 0, bpat.length(), false, &bdt, true), L"Fail to match if the string length is zero");
      }
};
}
