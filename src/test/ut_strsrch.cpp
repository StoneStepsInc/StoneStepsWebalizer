/*
   webalizer - a web server log analysis program

   Copyright (c) 2004-2019, Stone Steps Inc. (www.stonesteps.ca)

   See COPYING and Copyright files for additional licensing and copyright information 
   
   ut_strsrch.cpp
*/
#include "pchtest.h"

#include "../util_string.h"
#include "../tstring.h"

//
// StringSearch
//
namespace sswtest {

///
/// @brief  Sub-string search with a delta table
///
TEST(StringSearchTest, SubStrDeltaSearch)
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
   EXPECT_EQ(18, strstr_ex(str, srch, &dt) - str.c_str()) << "String and sub-string are null-terminated";
   EXPECT_EQ(18, strstr_ex(str, srch, str.length(), &dt) - str.c_str()) << "Sub-string is null-terminated";
   EXPECT_EQ(18, strstr_ex(str, srch, str.length(), srch.length(), &dt) - str.c_str()) << "String and sub-string lengths are not zeroes";
   EXPECT_EQ(18, strstr_ex(str, "abcdef", str.length(), srch.length(), &dt) - str.c_str()) << "Search is using only a part of the sub-string";

   //
   // Search the start of the string without the sub-string
   //
   EXPECT_EQ(nullptr, strstr_ex(str, "abcdef", 21, srch.length(), &dt)) << "Search a part of the string without the sub-string";

   // change the sub-string, so it's not present in the string
   srch = "xyz";
   dt.reset(srch);

   //
   // Look for a sub-string that is not present in the string
   //
   EXPECT_EQ(nullptr, strstr_ex(str, srch, &dt)) << "String and sub-string are null-terminated";
   EXPECT_EQ(nullptr, strstr_ex(str, srch, str.length(),  &dt)) << "Sub-string is null-terminated";
   EXPECT_EQ(nullptr, strstr_ex(str, srch, str.length(), srch.length(), &dt)) << "String and sub-string lengths are not zeroes";
}

///
/// @brief  Sub-string search without a delta table
///
TEST(StringSearchTest, SubStrSearch)
{
   //                      1         2
   //            012345678901234567890123456
   string_t str("abc ABCD acd xbcd abcd abcd");
   string_t srch("abcd");

   //
   // Look for a sub-string that is present in the string without a delta table
   //
   EXPECT_EQ(18, strstr_ex(str, srch, nullptr) - str.c_str()) << "String and sub-string are null-terminated";
   EXPECT_EQ(18, strstr_ex(str, srch, str.length(), nullptr) - str.c_str()) << "Sub-string is null-terminated";
   EXPECT_EQ(18, strstr_ex(str, srch, str.length(), srch.length(), nullptr) - str.c_str()) << "String and sub-string lengths are not zeroes";
   EXPECT_EQ(18, strstr_ex(str, "abcdef", str.length(), srch.length(), nullptr) - str.c_str()) << "Search is using only a part of the sub-string";

   //
   // Search the start of the string without the sub-string
   //
   EXPECT_EQ(nullptr, strstr_ex(str, "abcdef", 21, srch.length(), nullptr)) << "Search a part of the string without the sub-string";

   // change the sub-string, so it's not present in the string
   srch = "xyz";

   //
   // Look for a sub-string that is not present in the string
   //
   EXPECT_EQ(nullptr, strstr_ex(str, srch, nullptr)) << "String and sub-string are null-terminated";
   EXPECT_EQ(nullptr, strstr_ex(str, srch, str.length(), nullptr)) << "Sub-string is null-terminated";
   EXPECT_EQ(nullptr, strstr_ex(str, srch, str.length(), srch.length(), nullptr)) << "String and sub-string lengths are not zeroes";
}

///
/// @brief  Case-insensitive sub-string search with a delta table
///
TEST(StringSearchTest, SubStrDeltaSearchNoCase)
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
   EXPECT_EQ(18, strstr_ex(str, srch, &dt, true) - str.c_str()) << "String and sub-string are null-terminated";
   EXPECT_EQ(18, strstr_ex(str, srch, str.length(), &dt, true) - str.c_str()) << "Sub-string is null-terminated";
   EXPECT_EQ(18, strstr_ex(str, srch, str.length(), srch.length(), &dt, true) - str.c_str()) << "String and sub-string lengths are not zeroes";
   EXPECT_EQ(18, strstr_ex(str, "abcdef", str.length(), srch.length(), &dt, true) - str.c_str()) << "Search is using only a part of the sub-string";

   //
   // Search the start of the string without the sub-string
   //
   EXPECT_EQ(nullptr, strstr_ex(str, "abcdef", 21, srch.length(), &dt, true)) << "Search a part of the string without the sub-string";

   // change the sub-string, so it's not present in the string
   srch = "xyz";
   dt.reset(srch);

   //
   // Look for a sub-string that is not present in the string
   //
   EXPECT_EQ(nullptr, strstr_ex(str, srch, &dt)) << "String and sub-string are null-terminated";
   EXPECT_EQ(nullptr, strstr_ex(str, srch, str.length(),  &dt)) << "Sub-string is null-terminated";
   EXPECT_EQ(nullptr, strstr_ex(str, srch, str.length(), srch.length(), &dt)) << "String and sub-string lengths are not zeroes";
}

///
/// @brief  Case-insensitive sub-string search without a delta table
///
TEST(StringSearchTest, SubStrSearchNoCase)
{
   //                      1         2
   //            012345678901234567890123456
   string_t str("abc ABCX acd xbcd abcD abcd");
   string_t srch("aBCd");

   //
   // Look for a sub-string that is present in the string without a delta table
   //
   EXPECT_EQ(18, strstr_ex(str, srch, nullptr, true) - str.c_str()) << "String and sub-string are null-terminated";
   EXPECT_EQ(18, strstr_ex(str, srch, str.length(), nullptr, true) - str.c_str()) << "Sub-string is null-terminated";
   EXPECT_EQ(18, strstr_ex(str, srch, str.length(), srch.length(), nullptr, true) - str.c_str()) << "String and sub-string lengths are not zeroes";

   //
   // Search only using a part of a sub-string
   //
   EXPECT_EQ(18, strstr_ex(str, "abcdef", str.length(), srch.length(), nullptr, true) - str.c_str()) << "Search is using only a part of the sub-string";

   //
   // Search the start of the string without the sub-string
   //
   EXPECT_EQ(nullptr, strstr_ex(str, "abcdef", 21, srch.length(), nullptr, true)) << "Search a part of the string without the sub-string";

   // change the sub-string, so it's not present in the string
   srch = "xyz";

   //
   // Look for a sub-string that is not present in the string
   //
   EXPECT_EQ(nullptr, strstr_ex(str, srch, nullptr, true)) << "String and sub-string are null-terminated";
   EXPECT_EQ(nullptr, strstr_ex(str, srch, str.length(), nullptr, true)) << "Sub-string is null-terminated";
   EXPECT_EQ(nullptr, strstr_ex(str, srch, str.length(), srch.length(), nullptr, true)) << "String and sub-string lengths are not zeroes";
}

///
/// @brief  Match empty or NULL strings and sub-strings
///
TEST(StringSearchTest, SubStrEmptyOrNull)
{
   string_t str("abc ABCD acd xbcd abcd abcd");
   string_t srch("abcd");

   // all searches with empty strings must fail
   EXPECT_EQ(nullptr, strstr_ex(str, srch, 0, 0, nullptr)) << "Fail to match if the string and the sub-string are empty";
   EXPECT_EQ(nullptr, strstr_ex(str, srch, str.length(), 0, nullptr)) << "Fail to match if the sub-string is empty";
   EXPECT_EQ(nullptr, strstr_ex(nullptr, nullptr, 0, 0, nullptr)) << "Fail to match if the string and the sub-string are NULL";
   EXPECT_EQ(nullptr, strstr_ex(str, nullptr, str.length(), 0, nullptr)) << "Fail to match if the sub-string is NULL";
}

///
/// @brief  Match a string pattern case-sensitively and without a delta table
///
TEST(StringSearchTest, StrPatternMatch)
{
   string_t str("abc ABCD acd xbcd abcd abcd");
   string_t nmstr("Xbc ABCD acd xbcd aXcd abcX");
   string_t bpat("abc ABCD*");
   string_t epat("*abcd abcd");
   string_t spat("abcd");

   // match all patterns
   EXPECT_TRUE(isinstrex(str, bpat, str.length(), bpat.length(), false, nullptr, false)) << "Match the beginning of the string";
   EXPECT_TRUE(isinstrex(str, epat, str.length(), epat.length(), false, nullptr, false)) << "Match the end of the string";
   EXPECT_TRUE(isinstrex(str, spat, str.length(), spat.length(), true, nullptr, false)) << "Match a sub-string within the string";

   // fail to match all patterns
   EXPECT_FALSE(isinstrex(nmstr, bpat, str.length(), bpat.length(), false, nullptr, false)) << "Fail to match the beginning of the string";
   EXPECT_FALSE(isinstrex(nmstr, epat, str.length(), epat.length(), false, nullptr, false)) << "Fail to match the end of the string";
   EXPECT_FALSE(isinstrex(nmstr, spat, str.length(), spat.length(), true, nullptr, false)) << "Fail to match a sub-string within the string";

   // fail to match when either of the arguments is NULL or zero
   EXPECT_FALSE(isinstrex(nmstr, nullptr, str.length(), bpat.length(), false, nullptr, false)) << "Fail to match if the pattern is NULL";
   EXPECT_FALSE(isinstrex(nmstr, bpat, str.length(), 0, false, nullptr, false)) << "Fail to match if the pattern length is zero";
   EXPECT_FALSE(isinstrex(nullptr, bpat, str.length(), bpat.length(), false, nullptr, false)) << "Fail to match if the string is NULL";
   EXPECT_FALSE(isinstrex(nmstr, bpat, 0, bpat.length(), false, nullptr, false)) << "Fail to match if the string length is zero";
}

///
/// @brief  Match a string pattern case-sensitively and with a delta table
///
TEST(StringSearchTest, StrPatternMatchDelta)
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
   EXPECT_TRUE(isinstrex(str, bpat, str.length(), bpat.length(), false, &bdt, false)) << "Match the beginning of the string";
   EXPECT_TRUE(isinstrex(str, epat, str.length(), epat.length(), false, &edt, false)) << "Match the end of the string";
   EXPECT_TRUE(isinstrex(str, spat, str.length(), spat.length(), true, &sdt, false)) << "Match a sub-string within the string";

   // fail to match all patterns
   EXPECT_FALSE(isinstrex(nmstr, bpat, str.length(), bpat.length(), false, &bdt, false)) << "Fail to match the beginning of the string";
   EXPECT_FALSE(isinstrex(nmstr, epat, str.length(), epat.length(), false, &edt, false)) << "Fail to match the end of the string";
   EXPECT_FALSE(isinstrex(nmstr, spat, str.length(), spat.length(), true, &sdt, false)) << "Fail to match a sub-string within the string";

   // fail to match when either of the arguments is NULL or zero
   EXPECT_FALSE(isinstrex(nmstr, nullptr, str.length(), bpat.length(), false, &bdt, false)) << "Fail to match if the pattern is NULL";
   EXPECT_FALSE(isinstrex(nmstr, bpat, str.length(), 0, false, &bdt, false)) << "Fail to match if the pattern length is zero";
   EXPECT_FALSE(isinstrex(nullptr, bpat, str.length(), bpat.length(), false, &bdt, false)) << "Fail to match if the string is NULL";
   EXPECT_FALSE(isinstrex(nmstr, bpat, 0, bpat.length(), false, &bdt, false)) << "Fail to match if the string length is zero";
}

///
/// @brief  Match a string pattern case-insensitively and without a delta table
///
TEST(StringSearchTest, StrPatternMatchNoCase)
{
   string_t str("abc ABCD acd xbcd abcd abcd");
   string_t nmstr("Xbc AxCD acd xbcd aXcd abcX");
   string_t bpat("aBC ABCD*");
   string_t epat("*abcd aBCd");
   string_t spat("aBCd");

   // match all patterns
   EXPECT_TRUE(isinstrex(str, bpat, str.length(), bpat.length(), false, nullptr, true)) << "Match the beginning of the string";
   EXPECT_TRUE(isinstrex(str, epat, str.length(), epat.length(), false, nullptr, true)) << "Match the end of the string";
   EXPECT_TRUE(isinstrex(str, spat, str.length(), spat.length(), true, nullptr, true)) << "Match a sub-string within the string";

   // fail to match all patterns
   EXPECT_FALSE(isinstrex(nmstr, bpat, str.length(), bpat.length(), false, nullptr, true)) << "Fail to match the beginning of the string";
   EXPECT_FALSE(isinstrex(nmstr, epat, str.length(), epat.length(), false, nullptr, true)) << "Fail to match the end of the string";
   EXPECT_FALSE(isinstrex(nmstr, spat, str.length(), spat.length(), true, nullptr, true)) << "Fail to match a sub-string within the string";

   // fail to match when either of the arguments is NULL or zero
   EXPECT_FALSE(isinstrex(nmstr, nullptr, str.length(), bpat.length(), false, nullptr, true)) << "Fail to match if the pattern is NULL";
   EXPECT_FALSE(isinstrex(nmstr, bpat, str.length(), 0, false, nullptr, true)) << "Fail to match if the pattern length is zero";
   EXPECT_FALSE(isinstrex(nullptr, bpat, str.length(), bpat.length(), false, nullptr, true)) << "Fail to match if the string is NULL";
   EXPECT_FALSE(isinstrex(nmstr, bpat, 0, bpat.length(), false, nullptr, true)) << "Fail to match if the string length is zero";
}

///
/// @brief  Match a string pattern case-insensitively and with a delta table
///
TEST(StringSearchTest, StrPatternMatchDeltaNoCase)
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
   EXPECT_TRUE(isinstrex(str, bpat, str.length(), bpat.length(), false, &bdt, true)) << "Match the beginning of the string";
   EXPECT_TRUE(isinstrex(str, epat, str.length(), epat.length(), false, &edt, true)) << "Match the end of the string";
   EXPECT_TRUE(isinstrex(str, spat, str.length(), spat.length(), true, &sdt, true)) << "Match a sub-string within the string";

   // fail to match all patterns
   EXPECT_FALSE(isinstrex(nmstr, bpat, str.length(), bpat.length(), false, &bdt, true)) << "Fail to match the beginning of the string";
   EXPECT_FALSE(isinstrex(nmstr, epat, str.length(), epat.length(), false, &edt, true)) << "Fail to match the end of the string";
   EXPECT_FALSE(isinstrex(nmstr, spat, str.length(), spat.length(), true, &sdt, true)) << "Fail to match a sub-string within the string";

   // fail to match when either of the arguments is NULL or zero
   EXPECT_FALSE(isinstrex(nmstr, nullptr, str.length(), bpat.length(), false, &bdt, true)) << "Fail to match if the pattern is NULL";
   EXPECT_FALSE(isinstrex(nmstr, bpat, str.length(), 0, false, &bdt, true)) << "Fail to match if the pattern length is zero";
   EXPECT_FALSE(isinstrex(nullptr, bpat, str.length(), bpat.length(), false, &bdt, true)) << "Fail to match if the string is NULL";
   EXPECT_FALSE(isinstrex(nmstr, bpat, 0, bpat.length(), false, &bdt, true)) << "Fail to match if the string length is zero";
}
}
