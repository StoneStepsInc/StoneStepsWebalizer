/*
   webalizer - a web server log analysis program

   Copyright (c) 2004-2019, Stone Steps Inc. (www.stonesteps.ca)

   See COPYING and Copyright files for additional licensing and copyright information 
   
   ut_strsrch.cpp
*/
#include "pchtest.h"

#include "../tstring.h"

//
// StringFormatting
//
namespace sswtest {

///
/// @brief  Format a string using printf-style formats
///
TEST(StringFormattingTest, FormatString)
{
   string_t str;

   // make sure it's not an empty string
   str.reserve(1);

   EXPECT_STREQ("ABC12345678901234567890XYZ", str.format("%s", "ABC12345678901234567890XYZ")) << "Format a string with insufficient initial internal buffer size";
   EXPECT_STREQ("ABC123456789XYZ", str.format("ABC%dXYZ", 123456789)) << "Format a string with a sufficient internal buffer size";

   // test the static formatting function
   EXPECT_STREQ("ABC12345678901234567890XYZ", string_t::_format("%s", "ABC12345678901234567890XYZ")) << "Format a string with an empty initial internal buffer size";
   EXPECT_STREQ("ABC123456789XYZ", string_t::_format("ABC%dXYZ", 123456789)) << "Format a string with a format length less than the resulting string legth";
}
}
