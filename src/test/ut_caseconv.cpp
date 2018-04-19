/*
   webalizer - a web server log analysis program

   Copyright (c) 2004-2018, Stone Steps Inc. (www.stonesteps.ca)

   See COPYING and Copyright files for additional licensing and copyright information 
   
   ut_caseconv.cpp
*/
#include "pchtest.h"

#include "../tstring.h"

namespace sswtest {

///
/// @brief  ASCII character case conversion tests
///
TEST(StringCaseConversion, CaseConversionASCII)
{
   // ASCII string
   string_t s1("abc123xyz");  // 9 bytes

   EXPECT_STREQ("ABC123XYZ", s1.toupper()) << "Convert to upper case all characters in a string";
   EXPECT_STREQ("abc123xyz", s1.tolower()) << "Convert to lower case all characters in a string"; 

   EXPECT_STREQ("aBC123XYz", s1.toupper(1, 7)) << "Convert to upper case middle characters of a string";
   EXPECT_STREQ("aBc123xyz", s1.tolower((size_t) 2)) << "Convert to lower case all characters after the 2nd";

   EXPECT_STREQ("aBc123xyz", s1.toupper((size_t) 10)) << "Convert to upper case starting at past string length (no-op)";
   EXPECT_STREQ("aBc123xyz", s1.toupper(10, 32)) << "Convert to upper case starting at past string length, with length (no-op)";

   EXPECT_STREQ("ABC123XYZ", s1.toupper(0, 9)) << "Convert to upper case all characters from 0th to exactly the string length";

   EXPECT_STREQ("abc123xyz", s1.tolower(0, 32)) << "Convert to lower case all characters from 0th to past string length";
}

///
/// @brief  Two-byte UTF-8 character case conversion tests
///
TEST(StringCaseConversion, CaseConversionUTF8Latin1Char)
{
   // UTF-8 string with a two-byte Latin1 character
   string_t s1(u8"abc\u00A3xyz");   // 8 bytes

   EXPECT_STREQ(u8"ABC\u00A3XYZ", s1.toupper()) << "Convert to upper case all characters in a string";
   EXPECT_STREQ(u8"abc\u00A3xyz", s1.tolower()) << "Convert to lower case all characters in a string";

   EXPECT_STREQ(u8"aBC\u00A3XYz", s1.toupper(1, 6)) << "Convert to upper case middle characters of a string";
   EXPECT_STREQ(u8"aBc\u00A3xyz", s1.tolower((size_t) 2)) << "Convert to lower case all characters after the 2nd";

   EXPECT_STREQ(u8"aBc\u00A3xyz", s1.toupper((size_t) 10)) << "Convert to upper case starting at past string length (no-op)";
   EXPECT_STREQ(u8"aBc\u00A3xyz", s1.toupper(10, 32)) << "Convert to upper case starting at past string length, with length (no-op)";

   EXPECT_STREQ(u8"ABC\u00A3XYZ", s1.toupper(0, 9)) << "Convert to upper case all characters from 0th to exactly the string length";

   EXPECT_STREQ(u8"abc\u00A3xyz", s1.tolower(0, 32)) << "Convert to lower case all characters from 0th to past string length";
}

///
/// @brief  Three-byte UTF-8 character case conversion tests
///
TEST(StringCaseConversion, CaseConversionUTF8CJKChar)
{
   // UTF-8 string with a two-byte Latin1 character
   string_t s1(u8"abc\u898Bxyz");   // 9 bytes

   EXPECT_STREQ(u8"ABC\u898BXYZ", s1.toupper()) << "Convert to upper case all characters in a string";
   EXPECT_STREQ(u8"abc\u898Bxyz", s1.tolower()) << "Convert to lower case all characters in a string";

   EXPECT_STREQ(u8"aBC\u898BXYz", s1.toupper(1, 7)) << "Convert to upper case middle characters of a string";
   EXPECT_STREQ(u8"aBc\u898Bxyz", s1.tolower((size_t) 2)) << "Convert to lower case all characters after the 2nd";

   EXPECT_STREQ(u8"aBc\u898Bxyz", s1.toupper((size_t) 10)) << "Convert to upper case starting at past string length (no-op)";
   EXPECT_STREQ(u8"aBc\u898Bxyz", s1.toupper(10, 32)) << "Convert to upper case starting at past string length, with length (no-op)";

   EXPECT_STREQ(u8"ABC\u898BXYZ", s1.toupper(0, 9)) << "Convert to upper case all characters from 0th to exactly the string length";

   EXPECT_STREQ(u8"abc\u898Bxyz", s1.tolower(0, 32)) << "Convert to lower case all characters from 0th to past string length";
}
}