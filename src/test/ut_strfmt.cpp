/*
   webalizer - a web server log analysis program

   Copyright (c) 2004-2017, Stone Steps Inc. (www.stonesteps.ca)

   See COPYING and Copyright files for additional licensing and copyright information 
   
   ut_strsrch.cpp
*/
#include "pchtest.h"

#include "../tstring.h"

namespace mstest = Microsoft::VisualStudio::CppUnitTestFramework;

//
// StringFormatting
//
namespace sswtest {
TEST_CLASS(StringFormatting) {
   public:
      BEGIN_TEST_METHOD_ATTRIBUTE(FormatString)
         TEST_DESCRIPTION(L"Format a string using printf-style formats")
         TEST_METHOD_ATTRIBUTE(L"Category", L"String Formatting")
      END_TEST_METHOD_ATTRIBUTE()

      TEST_METHOD(FormatString)
      {
         string_t str;

         // make sure it's not an empty string
         str.reserve(1);

         mstest::Assert::AreEqual("ABC12345678901234567890XYZ", str.format("%s", "ABC12345678901234567890XYZ"), L"Format a string with insufficient initial internal buffer size");
         mstest::Assert::AreEqual("ABC123456789XYZ", str.format("ABC%dXYZ", 123456789), L"Format a string with a sufficient internal buffer size");

         mstest::Assert::AreEqual("ABC12345678901234567890XYZ", string_t::_format("%s", "ABC12345678901234567890XYZ"), L"Format a string with an empty initial internal buffer size");
         mstest::Assert::AreEqual("ABC123456789XYZ", string_t::_format("ABC%dXYZ", 123456789), L"Format a string with a format length less than the resulting string legth");
      }
};
}
