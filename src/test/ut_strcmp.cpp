/*
   webalizer - a web server log analysis program

   Copyright (c) 2004-2019, Stone Steps Inc. (www.stonesteps.ca)

   See COPYING and Copyright files for additional licensing and copyright information 
   
   ut_strcmp.cpp
*/
#include "pch.h"

#include "../tstring.h"

//
// StringSearch
//
namespace sswtest {

///
/// @brief  Compare two null-terminated strings case-sensitively
///
TEST(StringCompareTest, CompareStrings)
{
   EXPECT_TRUE(string_t::compare("abc", "abc") == 0) << "Two equal strings";

   EXPECT_TRUE(string_t::compare("aBc", "abc") < 0) << "A string with upper-case characters is less than the same lower-case string";
   EXPECT_TRUE(string_t::compare("abc", "aBc") > 0) << "A lower-case string is greater than the same string with upper-case characters";

   EXPECT_TRUE(string_t::compare("abc", "ab") > 0) << "A longer string is greater than a shorter one with the same initial characters";
   EXPECT_TRUE(string_t::compare("ab", "abs") < 0) << "A shorter string is less than a longer one with the same initial characters string";

   EXPECT_TRUE(string_t::compare("abc", "") > 0) << "Any string is greater than an empty one";
   EXPECT_TRUE(string_t::compare("", "abc") < 0) << "An empty string is less than any other string";
}

///
/// @brief  Compare up to N characters of two strings case-sensitively
///
TEST(StringCompareTest, CompareStringsNChars)
{
   // same tests as for plain string comparisons, but with N greater than both lengths
   EXPECT_TRUE(string_t::compare("abc", "abc", 6) == 0) << "Equal strings are shorter than N";

   EXPECT_TRUE(string_t::compare("aBc", "abc", 6) < 0) << "A string with upper-case characters is less than the same lower-case string";
   EXPECT_TRUE(string_t::compare("abc", "aBc", 6) > 0) << "A lower-case string is greater than the same string with upper-case characters";

   EXPECT_TRUE(string_t::compare("abc", "ab", 6) > 0) << "A longer string is greater than a shorter one with the same initial characters";
   EXPECT_TRUE(string_t::compare("ab", "abs", 6) < 0) << "A shorter string is less than a longer one with the same initial characters string";

   EXPECT_TRUE(string_t::compare("abc", "", 6) > 0) << "Any string is greater than an empty one";
   EXPECT_TRUE(string_t::compare("", "abc", 6) < 0) << "An empty string is less than any other string";

   // tests with N shorter than one or both strings
   EXPECT_TRUE(string_t::compare("abcdef", "abcxyz", 3) == 0) << "Two strings with equal initial characters";

   EXPECT_TRUE(string_t::compare("abcdef", "abc", 4) > 0) << "The second string is shorter than the first and shorter than N";
   EXPECT_TRUE(string_t::compare("abc", "abcdef", 4) < 0) << "The first string is shorter than the second and shorter than N";
}

///
/// @brief  Compare two null-terminated strings case-insensitively
///
TEST(StringCompareTest, CompareStringsNoCase)
{
   EXPECT_TRUE(string_t::compare_ci("abc", "abc") == 0) << "Two equal strings";
   EXPECT_TRUE(string_t::compare_ci("aBc", "abc") == 0) << "A string with upper-case characters is equal to the same lower-case string";
   EXPECT_TRUE(string_t::compare_ci("abc", "aBc") == 0) << "A lower-case string is equal to the same string with upper-case characters";

   EXPECT_TRUE(string_t::compare("aXc", "abc") < 0) << "A string with upper-case characters is less than all lower-case string";
   EXPECT_TRUE(string_t::compare("abc", "aXc") > 0) << "An all lower-case string is greater than a string with upper-case characters";

   EXPECT_TRUE(string_t::compare_ci("abc", "ab") > 0) << "A longer string is greater than a shorter one with the same initial characters";
   EXPECT_TRUE(string_t::compare_ci("ab", "abs") < 0) << "A shorter string is less than a longer one with the same initial characters string";

   EXPECT_TRUE(string_t::compare_ci("abc", "") > 0) << "Any string is greater than an empty one";
   EXPECT_TRUE(string_t::compare_ci("", "abc") < 0) << "An empty string is less than any other string";

   EXPECT_TRUE(string_t::compare_ci(u8"abc\u898Bxyz", u8"abc\u898Bxyz") == 0) << "Two equal strings with a Kanji character";
   EXPECT_TRUE(string_t::compare_ci(u8"abc\u898Bxyz", u8"abc") > 0) << "A string with a Kanji character is greater than a shorter one with same initial characters";
   EXPECT_TRUE(string_t::compare_ci(u8"abc", u8"abc\u898Bxyz") < 0) << "A shorter string with the same initial characters is less than one with a Kanji character";

   EXPECT_TRUE(string_t::compare_ci(u8"abc\u00c3xyz", u8"abc\u898Bxyz") < 0) << "A string with a Latin character is less than one with a Kanji character";
   EXPECT_TRUE(string_t::compare_ci(u8"abc\u898Bxyz", u8"abc\u00c3xyz") > 0) << "A string with with a Kanji character is greater than one with a Latin character";
}

///
/// @brief  Compare up to N characters of two strings case-insensitively
///
TEST(StringCompareTest, CompareStringsNCharsNoCase)
{
   // same tests as for plain string comparisons, but with N greater than both lengths
   EXPECT_TRUE(string_t::compare_ci("abc", "abc", 6) == 0) << "Equal strings are shorter than N";
   EXPECT_TRUE(string_t::compare_ci("aBc", "abc", 6) == 0) << "A string with upper-case characters is equal the same lower-case string";
   EXPECT_TRUE(string_t::compare_ci("abc", "aBc", 6) == 0) << "A lower-case string is to the same string with upper-case characters";

   EXPECT_TRUE(string_t::compare("aXc", "abc", 6) < 0) << "A string with upper-case characters is less than all lower-case string";
   EXPECT_TRUE(string_t::compare("abc", "aXc", 6) > 0) << "An all lower-case string is greater than a string with upper-case characters";

   EXPECT_TRUE(string_t::compare_ci("abc", "ab", 6) > 0) << "A longer string is greater than a shorter one with the same initial characters";
   EXPECT_TRUE(string_t::compare_ci("ab", "abs", 6) < 0) << "A shorter string is less than a longer one with the same initial characters string";

   EXPECT_TRUE(string_t::compare_ci("abc", "", 6) > 0) << "Any string is greater than an empty one";
   EXPECT_TRUE(string_t::compare_ci("", "abc", 6) < 0) << "An empty string is less than any other string";

   EXPECT_TRUE(string_t::compare_ci(u8"abc\u898Bxyz", u8"abc\u898Bxyz", 6) == 0) << "Two equal strings with a Kanji character";
   EXPECT_TRUE(string_t::compare_ci(u8"abc\u898Bxyz", u8"abc", 6) > 0) << "A string with a Kanji character is greater than a shorter one with same initial characters";
   EXPECT_TRUE(string_t::compare_ci(u8"abc", u8"abc\u898Bxyz", 6) < 0) << "A shorter string with the same initial characters is less than one with a Kanji character";

   EXPECT_TRUE(string_t::compare_ci(u8"abc\u00c3xyz", u8"abc\u898Bxyz", 6) < 0) << "A string with a Latin character is less than one with a Kanji character";
   EXPECT_TRUE(string_t::compare_ci(u8"abc\u898Bxyz", u8"abc\u00c3xyz", 6) > 0) << "A string with with a Kanji character is greater than one with a Latin character";

   // tests with N shorter than one or both strings (Kanji is 3 code units, Latin is 2 code units)
   EXPECT_TRUE(string_t::compare_ci("abcdef", "abcxyz", 3) == 0) << "Two strings with equal initial characters";

   EXPECT_TRUE(string_t::compare_ci("abcdef", "abc", 4) > 0) << "The second string is shorter than the first and shorter than N";
   EXPECT_TRUE(string_t::compare_ci("abc", "abcdef", 4) < 0) << "The first string is shorter than the second and shorter than N";

   EXPECT_TRUE(string_t::compare_ci(u8"abc\u898Bxyz", u8"abc\u898Bxyz", 6) == 0) << "First six code units of two equal strings with a Kanji character";
   EXPECT_TRUE(string_t::compare_ci(u8"abc\u898Bxyz", u8"abc", 6) > 0) << "First six code units of a string with a Kanji character is greater than a shorter one with same initial characters";
   EXPECT_TRUE(string_t::compare_ci(u8"abc", u8"abc\u898Bxyz", 6) < 0) << "A shorter string with the same initial characters is less than one with a Kanji character";

   EXPECT_TRUE(string_t::compare_ci(u8"abc\u00c3xyz", u8"abc\u898Bxyz", 6) < 0) << "A string with a Latin character is less than one with a Kanji character";
   EXPECT_TRUE(string_t::compare_ci(u8"abc\u898Bxyz", u8"abc\u00c3xyz", 6) > 0) << "A string with with a Kanji character is greater than one with a Latin character";
}

///
/// @brief  Case-sensitive string sort order
///
TEST(StringCompareTest, CompareStringsSortOrder)
{
   EXPECT_TRUE(string_t::compare("A", "X") < string_t::compare("M", "X")) << "'A' is further from 'X' than 'M' is from 'X'";
   EXPECT_TRUE(string_t::compare("X", "A") > string_t::compare("X", "M")) << "'X' is further from 'A' than 'X' is from 'M'";

   EXPECT_TRUE(string_t::compare("A", "x") < string_t::compare("M", "x")) << "'A' is further from 'x' than 'M' is from 'x'";
   EXPECT_TRUE(string_t::compare("x", "A") > string_t::compare("x", "M")) << "'x' is further from 'A' than 'x' is from 'M'";
}

///
/// @brief  Case-insensitive string sort order
///
TEST(StringCompareTest, CompareStringsSortOrderNoCase)
{
   EXPECT_TRUE(string_t::compare_ci("A", "X") < string_t::compare_ci("M", "X")) << "'A' is further from 'X' than 'M' is from 'X'";
   EXPECT_TRUE(string_t::compare_ci("X", "A") > string_t::compare_ci("X", "M")) << "'X' is further from 'A' than 'X' is from 'M'";

   EXPECT_TRUE(string_t::compare_ci("A", "X") == string_t::compare_ci("A", "x")) << "'A' is as far from 'X' as is 'a' from 'x'";
   EXPECT_TRUE(string_t::compare_ci("X", "A") == string_t::compare_ci("x", "A")) << "'X' is as far from 'A' as is 'x' from 'a'";
}

///
/// @brief  Compare a nullptr pointer to a string or two nullptr pointers
///
TEST(StringCompareTest, CompareStringsNullPtr)
{
   EXPECT_TRUE(string_t::compare(nullptr, "ABC") < 0) << "A nullptr pointer is less than any non-empty string";
   EXPECT_TRUE(string_t::compare("ABC", nullptr) > 0) << "Any non-empty string is greater than a nullptr pointer";
   EXPECT_TRUE(string_t::compare(nullptr, nullptr) == 0) << "Two nullptr pointers are equal";
   EXPECT_TRUE(string_t::compare("", nullptr) == 0) << "An empty string is equal to a nullptr pointer";
   EXPECT_TRUE(string_t::compare(nullptr, "") == 0) << "A nullptr pointer is equal to an empty string";

   EXPECT_TRUE(string_t::compare_ci(nullptr, "ABC") < 0) << "A nullptr pointer is less than any non-empty string";
   EXPECT_TRUE(string_t::compare_ci("ABC", nullptr) > 0) << "Any non-empty string is greater than a nullptr pointer";
   EXPECT_TRUE(string_t::compare_ci(nullptr, nullptr) == 0) << "Two nullptr pointers are equal";
   EXPECT_TRUE(string_t::compare_ci("", nullptr) == 0) << "An empty string is equal to a nullptr pointer";
   EXPECT_TRUE(string_t::compare_ci(nullptr, "") == 0) << "A nullptr pointer is equal to an empty string";
}
}
