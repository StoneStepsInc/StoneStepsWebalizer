/*
   webalizer - a web server log analysis program

   Copyright (c) 2004-2018, Stone Steps Inc. (www.stonesteps.ca)

   See COPYING and Copyright files for additional licensing and copyright information 
   
   ut_lang.cpp
*/
#include "pchtest.h"

#include "../lang.h"

namespace sswtest {

///
/// @brief  Match language codes
///
TEST(LangTest, MatchExactLangeCode)
{
   EXPECT_TRUE(lang_t::check_language("en", "en")) << "Same language codes should match";
   EXPECT_TRUE(lang_t::check_language("en", "EN")) << "Same language codes should match in different case";

   EXPECT_FALSE(lang_t::check_language("en", "de")) << "Different language codes should not match";
   EXPECT_FALSE(lang_t::check_language("en", "DE")) << "Different language codes should not match in different case";

   EXPECT_FALSE(lang_t::check_language("en", "en-us")) << "Language codes having different lengths should not match";

   EXPECT_TRUE(lang_t::check_language("en-us", "en_us")) << "Same language codes should match with different separators (1)";
   EXPECT_TRUE(lang_t::check_language("en_us", "en-us")) << "Same language codes should match with different separators (2)";
}

///
/// @brief  Match language codes
///
TEST(LangTest, MatchPartialLangeCode)
{
   EXPECT_TRUE(lang_t::check_language("en", "en", 2)) << "Same language codes should match";
   EXPECT_TRUE(lang_t::check_language("en", "en", 8)) << "Same language codes should match even if the length exceeds either string";

   EXPECT_FALSE(lang_t::check_language("en", "de", 2)) << "Different language codes should not match";

   EXPECT_TRUE(lang_t::check_language("en", "en-us", 2)) << "Language codes with the same language component should match (1)";
   EXPECT_TRUE(lang_t::check_language("en", "EN-US", 2)) << "Language codes with the same language component should match in different case (1)";

   EXPECT_TRUE(lang_t::check_language("en-US", "en", 2)) << "Language codes with the same language component should match (2)";
   EXPECT_TRUE(lang_t::check_language("en-us", "EN", 2)) << "Language codes with the same language component should match in different case (2)";
}
}