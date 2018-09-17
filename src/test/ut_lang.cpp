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

///
/// @brief  Assign language variables with varying spacing around the equal sign.
///
TEST(LangTest, AssignCharLangVarSpace)
{
   char buffer[] = u8"msg_processed=A1\n"
                     "msg_records= A2\n"
                     "msg_addresses =A3\n"
                     "msg_ignored = A4\n"
                     "msg_bad\t=\tA5";

   lang_t lang;
   std::vector<string_t> errors;

   lang.parse_lang_file("LangTest", buffer, errors);

   ASSERT_EQ(0, errors.size()) << "Language file should be parsed without any errors";

   EXPECT_STREQ("A1", lang.msg_processed) << "Variable assignment should work without any space around the equal sign";
   EXPECT_STREQ("A2", lang.msg_records) << "Variable assignment should work space following the equal sign";
   EXPECT_STREQ("A3", lang.msg_addresses) << "Variable assignment should work space preceeding the equal sign";
   EXPECT_STREQ("A4", lang.msg_ignored) << "Variable assignment should work with space on both sides of the equal sign";
   EXPECT_STREQ("A5", lang.msg_bad) << "Variable assignment should work with tabs on both sides of the equal sign";
}

///
/// @brief  Assign non-array language variables with different line endings.
///
TEST(LangTest, AssignCharLangVarLineEnding)
{
   char buffer[] = u8"msg_processed = ABC1\n"
                  u8"msg_records = ABC2\r\n"
                  u8"msg_addresses = ABC3\r"
                  u8"msg_ignored = ABC4";

   lang_t lang;
   std::vector<string_t> errors;

   lang.parse_lang_file("LangTest", buffer, errors);

   ASSERT_EQ(0, errors.size()) << "Language file should be parsed without any errors";

   EXPECT_STREQ("ABC1", lang.msg_processed) << "Variable assignment should work with an LF line ending";
   EXPECT_STREQ("ABC2", lang.msg_records) << "Variable assignment should work with a CRLF line ending";
   EXPECT_STREQ("ABC3", lang.msg_addresses) << "Variable assignment should work with a CR line ending";
   EXPECT_STREQ("ABC4", lang.msg_ignored) << "Variable assignment of the last variable in the file should work without any line ending";
}

///
/// @brief  Assign array language variables using comma-separated values on one line.
///
TEST(LangTest, AssignCharArrLangVarOneLine)
{
   char buffer[] = u8"s_month = A1,A2,A3,A4,A5,A6,A7,A8,A9,A10,A11,A12\n"
                   u8"l_month = X1, X2, X3, X4, X5, X6, X7, X8, X9, X10, X11, X12";

   lang_t lang;
   std::vector<string_t> errors;

   lang.parse_lang_file("LangTest", buffer, errors);

   ASSERT_EQ(0, errors.size()) << "Language file should be parsed without any errors";

   ASSERT_EQ(12, lang.s_month.size()) << "Character array elements that appear on one line must be properly assigned (s_month)";
   ASSERT_EQ(12, lang.l_month.size()) << "Character array elements that appear on one line must be properly assigned (l_month)";

   for(size_t i = 0; i < lang.s_month.size(); i++)
      EXPECT_STREQ(("A" + std::to_string(i+1)).c_str(), lang.s_month[i]) << "Array elements on one line without spaces must be properly assigned (s_month)";

   for(size_t i = 0; i < lang.l_month.size(); i++)
      EXPECT_STREQ(("X" + std::to_string(i+1)).c_str(), lang.l_month[i]) << "Array elements on one line with spaces must be properly assigned (s_month)";
}

///
/// @brief  Assign array language variables using comma-separated values on multiple lines.
///
TEST(LangTest, AssignCharArrLangVarMultiLine)
{
   char buffer[] = u8"s_month = A1,A2,A3,A4,\n"
                               "A5,A6,A7,A8,\n"
                               "A9,A10,A11,A12\n"
                   u8"l_month = X1, X2, X3, X4, \n"
                               "X5, X6, X7, X8, \n"
                               "X9, X10, X11, X12";

   lang_t lang;
   std::vector<string_t> errors;

   lang.parse_lang_file("LangTest", buffer, errors);

   ASSERT_EQ(0, errors.size()) << "Language file should be parsed without any errors";

   ASSERT_EQ(12, lang.s_month.size()) << "Character array elements that appear on multiple lines must be properly assigned (s_month)";
   ASSERT_EQ(12, lang.l_month.size()) << "Character array elements that appear on one multiple lines must be properly assigned (l_month)";

   for(size_t i = 0; i < lang.s_month.size(); i++)
      EXPECT_STREQ(("A" + std::to_string(i+1)).c_str(), lang.s_month[i]) << "Array elements on multiple lines without spaces must be properly assigned (s_month)";

   for(size_t i = 0; i < lang.l_month.size(); i++)
      EXPECT_STREQ(("X" + std::to_string(i+1)).c_str(), lang.l_month[i]) << "Array elements on multiple lines with spaces must be properly assigned (s_month)";
}

///
/// @brief  Assign array language variables with commas using quoted values.
///
TEST(LangTest, AssignCharArrLangVarQuoted)
{
   char buffer[] = u8"h_msg = A, \"B, C\", D,\n"
                              "\"E, F\", G\n";

   lang_t lang;
   std::vector<string_t> errors;

   lang.parse_lang_file("LangTest", buffer, errors);

   ASSERT_EQ(0, errors.size()) << "Language file should be parsed without any errors";

   ASSERT_EQ(5, lang.h_msg.size()) << "Quoted values may contain commas and should be properly assigned";

   EXPECT_STREQ("A", lang.h_msg[0]) << "An unquoted value should be properly assigned (0)";
   EXPECT_STREQ("B, C", lang.h_msg[1]) << "A quoted value should be properly assigned (1)";
   EXPECT_STREQ("D", lang.h_msg[2]) << "An unquoted value should be properly assigned (2)";
   EXPECT_STREQ("E, F", lang.h_msg[3]) << "A quoted value should be properly assigned (3)";
   EXPECT_STREQ("G", lang.h_msg[4]) << "An unquoted value should be properly assigned (4)";
}

///
/// @brief  Test that unbalanced quotes around array language variable values are 
///         reported as errors.
///
TEST(LangTest, AssignCharArrLangVarUnbalancedQuotes)
{
   char buffer[] = u8"h_msg = A, \"B, C, D,\n"
                              "\"E, F\", G\n";

   lang_t lang;
   std::vector<string_t> errors;

   lang.parse_lang_file("LangTest", buffer, errors);

   ASSERT_EQ(1, errors.size()) << "Language file should be parsed with an error";

   EXPECT_STREQ("Unbalanced quotes for language variable h_msg", errors[0].c_str()) << "Unbalanced quotes should be reported as errors";
}

///
/// @brief  Assign array language variables with the last entry ending with a comma 
///         and an empty line.
///
TEST(LangTest, AssignCharArrTrailingComma)
{
   char buffer[] = u8"h_msg = A, B,\n"
                              "C, D,\n"
                              "\n"
                              "msg_processed = X";

   lang_t lang;
   std::vector<string_t> errors;

   lang.parse_lang_file("LangTest", buffer, errors);

   ASSERT_EQ(0, errors.size()) << "Language file should be parsed without any errors";

   ASSERT_EQ(4, lang.h_msg.size()) << "All elements of a language variable array ending in a trailing comma should be assigned properly";

   EXPECT_STREQ("A", lang.h_msg[0]);
   EXPECT_STREQ("B", lang.h_msg[1]);
   EXPECT_STREQ("C", lang.h_msg[2]);
   EXPECT_STREQ("D", lang.h_msg[3]);

   EXPECT_STREQ("X", lang.msg_processed) << "The language variable following the trailing comma should be assigned properly";
}

///
/// @brief  Assign array language variables without a line ending after the last entry. 
///
TEST(LangTest, AssignCharArrTrailingCommaNoLineEnding)
{
   char buffer[] = u8"h_msg = A, B,\n"
                              "C, D,";

   lang_t lang;
   std::vector<string_t> errors;

   lang.parse_lang_file("LangTest", buffer, errors);

   ASSERT_EQ(0, errors.size()) << "Language file should be parsed without any errors";

   ASSERT_EQ(4, lang.h_msg.size()) << "All elements of a language variable array ending in a trailing comma should be assigned properly";

   EXPECT_STREQ("A", lang.h_msg[0]);
   EXPECT_STREQ("B", lang.h_msg[1]);
   EXPECT_STREQ("C", lang.h_msg[2]);
   EXPECT_STREQ("D", lang.h_msg[3]);
}

///
/// @brief  Assign HTTP response code array language variables using comma-separated values
///         on one line.
///
TEST(LangTest, AssignRespArrLangVarOneLine)
{
   char buffer[] = u8"response = X, C-123 - A, Code 456 - B\n";

   lang_t lang;
   std::vector<string_t> errors;

   lang.parse_lang_file("LangTest", buffer, errors);

   ASSERT_EQ(0, errors.size()) << "Language file should be parsed without any errors";

   ASSERT_EQ(12, lang.s_month.size()) << "Response code elements that appear on one line must be properly assigned";

   EXPECT_EQ(0, lang.response[0].code) << "Response code element without a numeric code should be zero (0)";
   EXPECT_STREQ("X", lang.response[0].desc) << "Response element on one line must be properly assigned (0)";

   EXPECT_EQ(123, lang.response[1].code) << "Response code element with a numeric code should be properly assigned (1)";
   EXPECT_STREQ("C-123 - A", lang.response[1].desc) << "Response element on one line must be properly assigned (1)";

   EXPECT_EQ(456, lang.response[2].code) << "Response code element with a numeric code should be properly assigned (2)";
   EXPECT_STREQ("Code 456 - B", lang.response[2].desc) << "Response element on one line must be properly assigned (2)";
}

///
/// @brief  Assign HTTP response code array language variables using comma-separated values
///         on multiple lines.
///
TEST(LangTest, AssignRespArrLangVarMultiLine)
{
   char buffer[] = u8"response = X, \n"
                                "C-123 - A, \n"
                                "Code 456 - B\n";

   lang_t lang;
   std::vector<string_t> errors;

   lang.parse_lang_file("LangTest", buffer, errors);

   ASSERT_EQ(0, errors.size()) << "Language file should be parsed without any errors";

   ASSERT_EQ(12, lang.s_month.size()) << "Response code elements that appear on multiple lines must be properly assigned";

   EXPECT_EQ(0, lang.response[0].code) << "Response code element without a numeric code should be zero (0)";
   EXPECT_STREQ("X", lang.response[0].desc) << "Response element on one line must be properly assigned (0)";

   EXPECT_EQ(123, lang.response[1].code) << "Response code element with a numeric code should be properly assigned (1)";
   EXPECT_STREQ("C-123 - A", lang.response[1].desc) << "Response element on one line must be properly assigned (1)";

   EXPECT_EQ(456, lang.response[2].code) << "Response code element with a numeric code should be properly assigned (2)";
   EXPECT_STREQ("Code 456 - B", lang.response[2].desc) << "Response element on one line must be properly assigned (2)";
}

///
/// @brief  Assign country code array language variables using comma-separated values
///         on multiple lines.
///
TEST(LangTest, AssignCountryArrLangVarMultiLine)
{
   char buffer[] = u8"ctry = *, \tX,\r"
                            "ab,\tAB, \r\n"
                            "cd,   CD,\n"
                            "ef,\t EF";

   lang_t lang;
   std::vector<string_t> errors;

   lang.parse_lang_file("LangTest", buffer, errors);

   ASSERT_EQ(0, errors.size()) << "Language file should be parsed without any errors";

   ASSERT_EQ(4, lang.ctry.size()) << "Country elements that appear on multiple lines must be properly assigned";

   EXPECT_STREQ("*", lang.ctry[0].ccode);
   EXPECT_STREQ("X", lang.ctry[0].desc);

   EXPECT_STREQ("ab", lang.ctry[1].ccode);
   EXPECT_STREQ("AB", lang.ctry[1].desc);

   EXPECT_STREQ("cd", lang.ctry[2].ccode);
   EXPECT_STREQ("CD", lang.ctry[2].desc);

   EXPECT_STREQ("ef", lang.ctry[3].ccode);
   EXPECT_STREQ("EF", lang.ctry[3].desc);
}

}