/*
   webalizer - a web server log analysis program

   Copyright (c) 2004-2017, Stone Steps Inc. (www.stonesteps.ca)

   See COPYING and Copyright files for additional licensing and copyright information 
   
   ut_strcmp.cpp
*/
#include "pchtest.h"

#include "../tstring.h"

namespace mstest = Microsoft::VisualStudio::CppUnitTestFramework;

//
// StringSearch
//
namespace sswtest {
TEST_CLASS(StringCompare) {
   public:
      BEGIN_TEST_METHOD_ATTRIBUTE(CompareStrings)
         TEST_DESCRIPTION(L"Compare two null-terminated strings case-sensitively")
         TEST_METHOD_ATTRIBUTE(L"Category", L"String Comparison")
      END_TEST_METHOD_ATTRIBUTE()

      TEST_METHOD(CompareStrings)
      {
         mstest::Assert::IsTrue(string_t::compare("abc", "abc") == 0, L"Two equal strings");

         mstest::Assert::IsTrue(string_t::compare("aBc", "abc") < 0, L"A string with upper-case characters is less than the same lower-case string");
         mstest::Assert::IsTrue(string_t::compare("abc", "aBc") > 0, L"A lower-case string is greater than the same string with upper-case characters");

         mstest::Assert::IsTrue(string_t::compare("abc", "ab") > 0, L"A longer string is greater than a shorter one with the same initial characters");
         mstest::Assert::IsTrue(string_t::compare("ab", "abs") < 0, L"A shorter string is less than a longer one with the same initial characters string");

         mstest::Assert::IsTrue(string_t::compare("abc", "") > 0, L"Any string is greater than an empty one");
         mstest::Assert::IsTrue(string_t::compare("", "abc") < 0, L"An empty string is less than any other string");
      }

      BEGIN_TEST_METHOD_ATTRIBUTE(CompareStringsNChars)
         TEST_DESCRIPTION(L"Compare up to N characters of two strings case-sensitively")
         TEST_METHOD_ATTRIBUTE(L"Category", L"String Comparison")
      END_TEST_METHOD_ATTRIBUTE()

      TEST_METHOD(CompareStringsNChars)
      {
         // same tests as for plain string comparisons, but with N greater than both lengths
         mstest::Assert::IsTrue(string_t::compare("abc", "abc", 6) == 0, L"Equal strings are shorter than N");

         mstest::Assert::IsTrue(string_t::compare("aBc", "abc", 6) < 0, L"A string with upper-case characters is less than the same lower-case string");
         mstest::Assert::IsTrue(string_t::compare("abc", "aBc", 6) > 0, L"A lower-case string is greater than the same string with upper-case characters");

         mstest::Assert::IsTrue(string_t::compare("abc", "ab", 6) > 0, L"A longer string is greater than a shorter one with the same initial characters");
         mstest::Assert::IsTrue(string_t::compare("ab", "abs", 6) < 0, L"A shorter string is less than a longer one with the same initial characters string");

         mstest::Assert::IsTrue(string_t::compare("abc", "", 6) > 0, L"Any string is greater than an empty one");
         mstest::Assert::IsTrue(string_t::compare("", "abc", 6) < 0, L"An empty string is less than any other string");

         // tests with N shorter than one or both strings
         mstest::Assert::IsTrue(string_t::compare("abcdef", "abcxyz", 3) == 0, L"Two strings with equal initial characters");

         mstest::Assert::IsTrue(string_t::compare("abcdef", "abc", 4) > 0, L"The second string is shorter than the first and shorter than N");
         mstest::Assert::IsTrue(string_t::compare("abc", "abcdef", 4) < 0, L"The first string is shorter than the second and shorter than N");
      }

      BEGIN_TEST_METHOD_ATTRIBUTE(CompareStringsNoCase)
         TEST_DESCRIPTION(L"Compare two null-terminated strings case-insensitively")
         TEST_METHOD_ATTRIBUTE(L"Category", L"String Comparison")
      END_TEST_METHOD_ATTRIBUTE()

      TEST_METHOD(CompareStringsNoCase)
      {
         mstest::Assert::IsTrue(string_t::compare_ci("abc", "abc") == 0, L"Two equal strings");
         mstest::Assert::IsTrue(string_t::compare_ci("aBc", "abc") == 0, L"A string with upper-case characters is equal to the same lower-case string");
         mstest::Assert::IsTrue(string_t::compare_ci("abc", "aBc") == 0, L"A lower-case string is equal to the same string with upper-case characters");

         mstest::Assert::IsTrue(string_t::compare("aXc", "abc") < 0, L"A string with upper-case characters is less than all lower-case string");
         mstest::Assert::IsTrue(string_t::compare("abc", "aXc") > 0, L"An all lower-case string is greater than a string with upper-case characters");

         mstest::Assert::IsTrue(string_t::compare_ci("abc", "ab") > 0, L"A longer string is greater than a shorter one with the same initial characters");
         mstest::Assert::IsTrue(string_t::compare_ci("ab", "abs") < 0, L"A shorter string is less than a longer one with the same initial characters string");

         mstest::Assert::IsTrue(string_t::compare_ci("abc", "") > 0, L"Any string is greater than an empty one");
         mstest::Assert::IsTrue(string_t::compare_ci("", "abc") < 0, L"An empty string is less than any other string");

         mstest::Assert::IsTrue(string_t::compare_ci(u8"abc\u898Bxyz", u8"abc\u898Bxyz") == 0, L"Two equal strings with a Kanji character");
         mstest::Assert::IsTrue(string_t::compare_ci(u8"abc\u898Bxyz", u8"abc") > 0, L"A string with a Kanji character is greater than a shorter one with same initial characters");
         mstest::Assert::IsTrue(string_t::compare_ci(u8"abc", u8"abc\u898Bxyz") < 0, L"A shorter string with the same initial characters is less than one with a Kanji character");

         mstest::Assert::IsTrue(string_t::compare_ci(u8"abc\u00c3xyz", u8"abc\u898Bxyz") < 0, L"A string with a Latin character is less than one with a Kanji character");
         mstest::Assert::IsTrue(string_t::compare_ci(u8"abc\u898Bxyz", u8"abc\u00c3xyz") > 0, L"A string with with a Kanji character is greater than one with a Latin character");
      }

      BEGIN_TEST_METHOD_ATTRIBUTE(CompareStringsNCharsNoCase)
         TEST_DESCRIPTION(L"Compare up to N characters of two strings case-insensitively")
         TEST_METHOD_ATTRIBUTE(L"Category", L"String Comparison")
      END_TEST_METHOD_ATTRIBUTE()

      TEST_METHOD(CompareStringsNCharsNoCase)
      {
         // same tests as for plain string comparisons, but with N greater than both lengths
         mstest::Assert::IsTrue(string_t::compare_ci("abc", "abc", 6) == 0, L"Equal strings are shorter than N");
         mstest::Assert::IsTrue(string_t::compare_ci("aBc", "abc", 6) == 0, L"A string with upper-case characters is equal the same lower-case string");
         mstest::Assert::IsTrue(string_t::compare_ci("abc", "aBc", 6) == 0, L"A lower-case string is to the same string with upper-case characters");

         mstest::Assert::IsTrue(string_t::compare("aXc", "abc", 6) < 0, L"A string with upper-case characters is less than all lower-case string");
         mstest::Assert::IsTrue(string_t::compare("abc", "aXc", 6) > 0, L"An all lower-case string is greater than a string with upper-case characters");

         mstest::Assert::IsTrue(string_t::compare_ci("abc", "ab", 6) > 0, L"A longer string is greater than a shorter one with the same initial characters");
         mstest::Assert::IsTrue(string_t::compare_ci("ab", "abs", 6) < 0, L"A shorter string is less than a longer one with the same initial characters string");

         mstest::Assert::IsTrue(string_t::compare_ci("abc", "", 6) > 0, L"Any string is greater than an empty one");
         mstest::Assert::IsTrue(string_t::compare_ci("", "abc", 6) < 0, L"An empty string is less than any other string");

         mstest::Assert::IsTrue(string_t::compare_ci(u8"abc\u898Bxyz", u8"abc\u898Bxyz", 6) == 0, L"Two equal strings with a Kanji character");
         mstest::Assert::IsTrue(string_t::compare_ci(u8"abc\u898Bxyz", u8"abc", 6) > 0, L"A string with a Kanji character is greater than a shorter one with same initial characters");
         mstest::Assert::IsTrue(string_t::compare_ci(u8"abc", u8"abc\u898Bxyz", 6) < 0, L"A shorter string with the same initial characters is less than one with a Kanji character");

         mstest::Assert::IsTrue(string_t::compare_ci(u8"abc\u00c3xyz", u8"abc\u898Bxyz", 6) < 0, L"A string with a Latin character is less than one with a Kanji character");
         mstest::Assert::IsTrue(string_t::compare_ci(u8"abc\u898Bxyz", u8"abc\u00c3xyz", 6) > 0, L"A string with with a Kanji character is greater than one with a Latin character");

         // tests with N shorter than one or both strings (Kanji is 3 code units, Latin is 2 code units)
         mstest::Assert::IsTrue(string_t::compare_ci("abcdef", "abcxyz", 3) == 0, L"Two strings with equal initial characters");

         mstest::Assert::IsTrue(string_t::compare_ci("abcdef", "abc", 4) > 0, L"The second string is shorter than the first and shorter than N");
         mstest::Assert::IsTrue(string_t::compare_ci("abc", "abcdef", 4) < 0, L"The first string is shorter than the second and shorter than N");

         mstest::Assert::IsTrue(string_t::compare_ci(u8"abc\u898Bxyz", u8"abc\u898Bxyz", 6) == 0, L"First six code units of two equal strings with a Kanji character");
         mstest::Assert::IsTrue(string_t::compare_ci(u8"abc\u898Bxyz", u8"abc", 6) > 0, L"First six code units of a string with a Kanji character is greater than a shorter one with same initial characters");
         mstest::Assert::IsTrue(string_t::compare_ci(u8"abc", u8"abc\u898Bxyz", 6) < 0, L"A shorter string with the same initial characters is less than one with a Kanji character");

         mstest::Assert::IsTrue(string_t::compare_ci(u8"abc\u00c3xyz", u8"abc\u898Bxyz", 6) < 0, L"A string with a Latin character is less than one with a Kanji character");
         mstest::Assert::IsTrue(string_t::compare_ci(u8"abc\u898Bxyz", u8"abc\u00c3xyz", 6) > 0, L"A string with with a Kanji character is greater than one with a Latin character");
      }

      BEGIN_TEST_METHOD_ATTRIBUTE(CompareStringsSortOrder)
         TEST_DESCRIPTION(L"Case-sensitive string sort order")
         TEST_METHOD_ATTRIBUTE(L"Category", L"String Comparison")
      END_TEST_METHOD_ATTRIBUTE()

      TEST_METHOD(CompareStringsSortOrder)
      {
         mstest::Assert::IsTrue(string_t::compare("A", "X") < string_t::compare("M", "X"), L"'A' is further from 'X' than 'M' is from 'X'");
         mstest::Assert::IsTrue(string_t::compare("X", "A") > string_t::compare("X", "M"), L"'X' is further from 'A' than 'X' is from 'M'");

         mstest::Assert::IsTrue(string_t::compare("A", "x") < string_t::compare("M", "x"), L"'A' is further from 'x' than 'M' is from 'x'");
         mstest::Assert::IsTrue(string_t::compare("x", "A") > string_t::compare("x", "M"), L"'x' is further from 'A' than 'x' is from 'M'");
      }

      BEGIN_TEST_METHOD_ATTRIBUTE(CompareStringsSortOrderNoCase)
         TEST_DESCRIPTION(L"Case-insensitive string sort order")
         TEST_METHOD_ATTRIBUTE(L"Category", L"String Comparison")
      END_TEST_METHOD_ATTRIBUTE()

      TEST_METHOD(CompareStringsSortOrderNoCase)
      {
         mstest::Assert::IsTrue(string_t::compare_ci("A", "X") < string_t::compare_ci("M", "X"), L"'A' is further from 'X' than 'M' is from 'X'");
         mstest::Assert::IsTrue(string_t::compare_ci("X", "A") > string_t::compare_ci("X", "M"), L"'X' is further from 'A' than 'X' is from 'M'");

         mstest::Assert::IsTrue(string_t::compare_ci("A", "X") == string_t::compare_ci("A", "x"), L"'A' is as far from 'X' as is 'a' from 'x'");
         mstest::Assert::IsTrue(string_t::compare_ci("X", "A") == string_t::compare_ci("x", "A"), L"'X' is as far from 'A' as is 'x' from 'a'");
      }

      BEGIN_TEST_METHOD_ATTRIBUTE(CompareStringsNullPtr)
         TEST_DESCRIPTION(L"Compare a NULL pointer to a string or two NULL pointers")
         TEST_METHOD_ATTRIBUTE(L"Category", L"String Comparison")
      END_TEST_METHOD_ATTRIBUTE()

      TEST_METHOD(CompareStringsNullPtr)
      {
         mstest::Assert::IsTrue(string_t::compare(nullptr, "ABC") < 0, L"A NULL pointer is less than any non-empty string");
         mstest::Assert::IsTrue(string_t::compare("ABC", nullptr) > 0, L"Any non-empty string is greater than a NULL pointer");
         mstest::Assert::IsTrue(string_t::compare(nullptr, nullptr) == 0, L"Two NULL pointers are equal");
         mstest::Assert::IsTrue(string_t::compare("", nullptr) == 0, L"An empty string is equal to a NULL pointer");
         mstest::Assert::IsTrue(string_t::compare(nullptr, "") == 0, L"A NULL pointer is equal to an empty string");

         mstest::Assert::IsTrue(string_t::compare_ci(nullptr, "ABC") < 0, L"A NULL pointer is less than any non-empty string");
         mstest::Assert::IsTrue(string_t::compare_ci("ABC", nullptr) > 0, L"Any non-empty string is greater than a NULL pointer");
         mstest::Assert::IsTrue(string_t::compare_ci(nullptr, nullptr) == 0, L"Two NULL pointers are equal");
         mstest::Assert::IsTrue(string_t::compare_ci("", nullptr) == 0, L"An empty string is equal to a NULL pointer");
         mstest::Assert::IsTrue(string_t::compare_ci(nullptr, "") == 0, L"A NULL pointer is equal to an empty string");
      }
};
}
