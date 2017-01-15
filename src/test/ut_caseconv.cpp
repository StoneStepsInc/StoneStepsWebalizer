/*
   webalizer - a web server log analysis program

   Copyright (c) 2004-2017, Stone Steps Inc. (www.stonesteps.ca)

   See COPYING and Copyright files for additional licensing and copyright information 
   
   ut_string.cpp
*/
#include "pchtest.h"

#include "../tstring.h"

namespace mstest = Microsoft::VisualStudio::CppUnitTestFramework;

//
// StringCaseConversion
//
namespace sswtest {
TEST_CLASS(StringCaseConversion) {
   public:
       BEGIN_TEST_METHOD_ATTRIBUTE(CaseConversionASCII)
         TEST_DESCRIPTION(L"ASCII character case conversion tests")
         TEST_METHOD_ATTRIBUTE(L"Category", L"String")
       END_TEST_METHOD_ATTRIBUTE()

       TEST_METHOD(CaseConversionASCII)
       {
         // ASCII string
         string_t s1("abc123xyz");  // 9 bytes

         mstest::Assert::AreEqual("ABC123XYZ", s1.toupper(), "Convert to upper case all characters in a string");
         mstest::Assert::AreEqual("abc123xyz", s1.tolower(), "Convert to lower case all characters in a string"); 

         mstest::Assert::AreEqual("aBC123XYz", s1.toupper(1, 7), "Convert to upper case middle characters of a string");
         mstest::Assert::AreEqual("aBc123xyz", s1.tolower((size_t) 2), "Convert to lower case all characters after the 2nd");

         mstest::Assert::AreEqual("aBc123xyz", s1.toupper((size_t) 10), "Convert to upper case starting at past string length (no-op)");
         mstest::Assert::AreEqual("aBc123xyz", s1.toupper(10, 32), "Convert to upper case starting at past string length, with length (no-op)");

         mstest::Assert::AreEqual("ABC123XYZ", s1.toupper(0, 9), "Convert to upper case all characters from 0th to exactly the string length");

         mstest::Assert::AreEqual("abc123xyz", s1.tolower(0, 32), "Convert to lower case all characters from 0th to past string length");
       }

       BEGIN_TEST_METHOD_ATTRIBUTE(CaseConversionUTF8Latin1Char)
         TEST_DESCRIPTION(L"Two-byte UTF-8 character case conversion tests")
         TEST_METHOD_ATTRIBUTE(L"Category", L"String")
       END_TEST_METHOD_ATTRIBUTE()

       TEST_METHOD(CaseConversionUTF8Latin1Char)
       {
         // UTF-8 string with a two-byte Latin1 character
         string_t s1(u8"abc\u00A3xyz");   // 8 bytes

         mstest::Assert::AreEqual(u8"ABC\u00A3XYZ", s1.toupper(), "Convert to upper case all characters in a string");
         mstest::Assert::AreEqual(u8"abc\u00A3xyz", s1.tolower(), "Convert to lower case all characters in a string"); 

         mstest::Assert::AreEqual(u8"aBC\u00A3XYz", s1.toupper(1, 6), "Convert to upper case middle characters of a string");
         mstest::Assert::AreEqual(u8"aBc\u00A3xyz", s1.tolower((size_t) 2), "Convert to lower case all characters after the 2nd");

         mstest::Assert::AreEqual(u8"aBc\u00A3xyz", s1.toupper((size_t) 10), "Convert to upper case starting at past string length (no-op)");
         mstest::Assert::AreEqual(u8"aBc\u00A3xyz", s1.toupper(10, 32), "Convert to upper case starting at past string length, with length (no-op)");

         mstest::Assert::AreEqual(u8"ABC\u00A3XYZ", s1.toupper(0, 9), "Convert to upper case all characters from 0th to exactly the string length");

         mstest::Assert::AreEqual(u8"abc\u00A3xyz", s1.tolower(0, 32), "Convert to lower case all characters from 0th to past string length");
      }

       BEGIN_TEST_METHOD_ATTRIBUTE(CaseConversionUTF8CJKChar)
         TEST_DESCRIPTION(L"Three-byte UTF-8 character case conversion tests")
         TEST_METHOD_ATTRIBUTE(L"Category", L"String")
       END_TEST_METHOD_ATTRIBUTE()

       TEST_METHOD(CaseConversionUTF8CJKChar)
       {
         // UTF-8 string with a two-byte Latin1 character
         string_t s1(u8"abc\u898Bxyz");   // 9 bytes

         mstest::Assert::AreEqual(u8"ABC\u898BXYZ", s1.toupper(), "Convert to upper case all characters in a string");
         mstest::Assert::AreEqual(u8"abc\u898Bxyz", s1.tolower(), "Convert to lower case all characters in a string"); 

         mstest::Assert::AreEqual(u8"aBC\u898BXYz", s1.toupper(1, 7), "Convert to upper case middle characters of a string");
         mstest::Assert::AreEqual(u8"aBc\u898Bxyz", s1.tolower((size_t) 2), "Convert to lower case all characters after the 2nd");

         mstest::Assert::AreEqual(u8"aBc\u898Bxyz", s1.toupper((size_t) 10), "Convert to upper case starting at past string length (no-op)");
         mstest::Assert::AreEqual(u8"aBc\u898Bxyz", s1.toupper(10, 32), "Convert to upper case starting at past string length, with length (no-op)");

         mstest::Assert::AreEqual(u8"ABC\u898BXYZ", s1.toupper(0, 9), "Convert to upper case all characters from 0th to exactly the string length");

         mstest::Assert::AreEqual(u8"abc\u898Bxyz", s1.tolower(0, 32), "Convert to lower case all characters from 0th to past string length");
      }
};
}