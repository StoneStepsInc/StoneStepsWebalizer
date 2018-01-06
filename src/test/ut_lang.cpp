/*
   webalizer - a web server log analysis program

   Copyright (c) 2004-2018, Stone Steps Inc. (www.stonesteps.ca)

   See COPYING and Copyright files for additional licensing and copyright information 
   
   ut_lang.cpp
*/
#include "pchtest.h"

#include "../lang.h"

namespace mstest = Microsoft::VisualStudio::CppUnitTestFramework;

//
// Lang
//
namespace sswtest {
TEST_CLASS(Lang) {
   public:
      BEGIN_TEST_METHOD_ATTRIBUTE(MatchExactLangeCode)
         TEST_DESCRIPTION(L"Match language codes")
         TEST_METHOD_ATTRIBUTE(L"Category", L"Language")
      END_TEST_METHOD_ATTRIBUTE()

      TEST_METHOD(MatchExactLangeCode)
      {
         mstest::Assert::IsTrue(lang_t::check_language("en", "en"), L"Same language codes should match");
         mstest::Assert::IsTrue(lang_t::check_language("en", "EN"), L"Same language codes should match in different case");

         mstest::Assert::IsFalse(lang_t::check_language("en", "de"), L"Different language codes should not match");
         mstest::Assert::IsFalse(lang_t::check_language("en", "DE"), L"Different language codes should not match in different case");

         mstest::Assert::IsFalse(lang_t::check_language("en", "en-us"), L"Language codes having different lengths should not match");

         mstest::Assert::IsTrue(lang_t::check_language("en-us", "en_us"), L"Same language codes should match with different separators (1)");
         mstest::Assert::IsTrue(lang_t::check_language("en_us", "en-us"), L"Same language codes should match with different separators (2)");
      }

      BEGIN_TEST_METHOD_ATTRIBUTE(MatchPartialLangeCode)
         TEST_DESCRIPTION(L"Match language codes")
         TEST_METHOD_ATTRIBUTE(L"Category", L"Language")
      END_TEST_METHOD_ATTRIBUTE()

      TEST_METHOD(MatchPartialLangeCode)
      {
         mstest::Assert::IsTrue(lang_t::check_language("en", "en", 2), L"Same language codes should match");
         mstest::Assert::IsTrue(lang_t::check_language("en", "en", 8), L"Same language codes should match even if the length exceeds either string");

         mstest::Assert::IsFalse(lang_t::check_language("en", "de", 2), L"Different language codes should not match");

         mstest::Assert::IsTrue(lang_t::check_language("en", "en-us", 2), L"Language codes with the same language component should match (1)");
         mstest::Assert::IsTrue(lang_t::check_language("en", "EN-US", 2), L"Language codes with the same language component should match in different case (1)");

         mstest::Assert::IsTrue(lang_t::check_language("en-US", "en", 2), L"Language codes with the same language component should match (2)");
         mstest::Assert::IsTrue(lang_t::check_language("en-us", "EN", 2), L"Language codes with the same language component should match in different case (2)");
      }
};
}