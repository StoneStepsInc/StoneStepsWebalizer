/*
   webalizer - a web server log analysis program

   Copyright (c) 2004-2022, Stone Steps Inc. (www.stonesteps.ca)

   See COPYING and Copyright files for additional licensing and copyright information 

   ut_unicode.cpp
*/
#include "pch.h"

#include "../unicode.h"
#include "../tstring.h"

///
/// @brief  Tests conversion of UCS-2 or UCS-4 characters to sequences of UTF-8 bytes.
///
TEST(Unicode, UCS2UTF8Test)
{
   char u8ch[5] = {};   // one extra byte for a null terminator

   // Scalar Value                  First Byte  Second Byte Third Byte  Fourth Byte
   // 00000000 0xxxxxxx             0xxxxxxx

   ASSERT_EQ(1, ucs2utf8(L'A', u8ch)) << "An ASCII UCS-2 character should convert to a one-byte UTF-8 character";
   EXPECT_EQ('A', *u8ch) << "L'A' is expected to convert to 'A'";

   // 00000yyy yyxxxxxx             110yyyyy    10xxxxxx
   ASSERT_EQ(2, ucs2utf8(L'\u00A3', u8ch)) << "A pound sign character should convert into a two-byte UTF-8 character";
   u8ch[2] = 0;
   EXPECT_STREQ(u8"\u00A3", u8ch) << "Wide pound sign character should convert into a two-byte UTF-8 character";

   // zzzzyyyy yyxxxxxx             1110zzzz    10yyyyyy    10xxxxxx
   ASSERT_EQ(3, ucs2utf8(L'\uF922', u8ch)) << "A Kanji character should convert into a three-byte UTF-8 character";
   u8ch[3] = 0;
   EXPECT_STREQ(u8"\uF922", u8ch) << "A wide Kanji character should convert into a three-byte UTF-8 character";

   // GCC implements wchar_t as a 4-byte entity on many platforms
#if WCHAR_MAX > 0xFFFF
   // 000uuuuu zzzzyyyy yyxxxxxx    11110uuu    10uuzzzz    10yyyyyy    10xxxxxx
   ASSERT_EQ(4, ucs2utf8(L'\U00010334', u8ch)) << "A Gothic character should convert into a four-byte UTF-8 character";
   u8ch[4] = 0;
   EXPECT_STREQ(u8"\U00010334", u8ch) << "Wide Gothic character should convert into a fout-byte UTF-8 character";
#endif

   // surrogate pairs should return the size of the replacement character
   EXPECT_EQ(3, ucs2utf8(L'\xD800', u8ch)) << "High surrogate pair code unit should not be converted to UTF-8";
   EXPECT_EQ(3, ucs2utf8(L'\xDBFF', u8ch)) << "High surrogate pair code unit should not be converted to UTF-8";

   EXPECT_EQ(3, ucs2utf8(L'\xDC00', u8ch)) << "Low surrogate pair code unit should not be converted to UTF-8";
   EXPECT_EQ(3, ucs2utf8(L'\xDFFF', u8ch)) << "Low surrogate pair code unit should not be converted to UTF-8";
}

///
/// @brief  Tests UTF-8 character sizes within each of possible code unit ranges.
///
TEST(Unicode, UTF8CharSizeTest)
{
   // Code Points          First Byte  Second Byte Third Byte  Fourth Byte
   // U+0000   .. U+007F   00..7F
   EXPECT_EQ(1, utf8size("A")) << "Character 'A' is a one-byte UTF-8 character";
   EXPECT_EQ(1, utf8size("z")) << "Character 'z' is a one-byte UTF-8 character";

   // U+0080   .. U+07FF   C2..DF      80..BF
   EXPECT_EQ(2, utf8size(u8"\u00A3")) << "A pound sign character is a two-byte UTF-8 character";
   EXPECT_EQ(2, utf8size(u8"\u0713")) << "A Syriac character is a two-byte UTF-8 character";

   // U+0800   .. U+0FFF   E0          A0..BF      80..BF
   EXPECT_EQ(3, utf8size(u8"\u0985")) << "A Bengali character is a three-byte UTF-8 character";
   EXPECT_EQ(3, utf8size(u8"\u0F13")) << "A Tibetan character is a three-byte UTF-8 character";

   // U+1000   .. U+CFFF   E1..EC      80..BF      80..BF
   EXPECT_EQ(3, utf8size(u8"\u1003")) << "A Myanmar character is a three-byte UTF-8 character";
   EXPECT_EQ(3, utf8size(u8"\uCF03")) << "A Hangul Syllable character is a three-byte UTF-8 character";

   // U+D000   .. U+D7FF   ED          80..9F      80..BF
   EXPECT_EQ(3, utf8size(u8"\uD003")) << "A Hangul Syllable character is a three-byte UTF-8 character";
   EXPECT_EQ(3, utf8size(u8"\uD78F")) << "A Hangul Syllable character is a three-byte UTF-8 character";

   // U+E000   .. U+FFFF   EE..EF      80..BF      80..BF
   EXPECT_EQ(3, utf8size(u8"\uE014")) << "A Private Use Area character is a three-byte UTF-8 character";
   EXPECT_EQ(3, utf8size(u8"\uF8FB")) << "A Private Use Area character is a three-byte UTF-8 character";

   // U+10000  .. U+3FFFF  F0          90..BF      80..BF      80..BF
   EXPECT_EQ(4, utf8size(u8"\U00010104")) << "An Aegean Numbers character is a four-byte UTF-8 character";
   EXPECT_EQ(4, utf8size(u8"\U0002F947")) << "A CJK Compatibility Ideographs Supplement character is a four-byte UTF-8 character";

   // U+40000  .. U+FFFFF  F1..F3      80..BF      80..BF      80..BF
   EXPECT_EQ(4, utf8size(u8"\U00040000")) << "A character in U+40000..U+FFFFF range is a four-byte UTF-8 character";
   EXPECT_EQ(4, utf8size(u8"\U000FFFFF")) << "A character in U+40000..U+FFFFF range is a four-byte UTF-8 character";

   // U+100000 .. U+10FFFF F4          80..8F      80..BF      80..BF   
   EXPECT_EQ(4, utf8size(u8"\U00100000")) << "A Supplementary Private Use Area-B character is a four-byte UTF-8 character";
   EXPECT_EQ(4, utf8size(u8"\U0010FFFD")) << "A Supplementary Private Use Area-B character is a four-byte UTF-8 character";
}

///
/// @brief  Tests converting of N characters of a UCS-2 or UCS-4 string to a UTF-8 string.
///
TEST(Unicode, UCS2toUTF8StringLengthTest)
{
   char u8out[16 * 4 + 1] = {};     // maximum 16 wide characters and a null character

   // ASCII conversion without a null character
   u8out[3] = 'X';
   ASSERT_EQ(3, ucs2utf8(L"ABCDEF", 3, u8out, 10)) << "Convert 3 wide ASCII characters";
   ASSERT_EQ('X', u8out[3]) << "ASCII output characters past allowed input should not change";
   u8out[3] = 0;
   EXPECT_STREQ(u8"ABC", u8out);

   // ASCII conversion with a null character
   u8out[3] = 'X';
   ASSERT_EQ(4, ucs2utf8(L"ABC\x0" L"EF", 4, u8out, 10)) << "Convert 3 wide ASCII characters and a null wide character to a UTF-8 string";
   ASSERT_EQ(0, u8out[3]) << "A null wide character in an ASCII string should convert to a null UTF-8 character";
   EXPECT_STREQ(u8"ABC", u8out);

   // Latin-1 conversion without a null character
   u8out[4] = 'X';
   ASSERT_EQ(4, ucs2utf8(L"A\u00A3CDEF", 3, u8out, 10)) << "Convert a wide string with a pound sign to UTF-8";
   ASSERT_EQ('X', u8out[4]) << "Latin-1 output characters past allowed input should not change";
   u8out[4] = 0;
   EXPECT_STREQ(u8"A\u00A3C", u8out);

   // Latin-1 conversion with a null character
   u8out[4] = 'X';
   ASSERT_EQ(5, ucs2utf8(L"A\u00A3C\x0" L"EF", 4, u8out, 10)) << "Convert a wide string with a pound sign to UTF-8";
   ASSERT_EQ(0, u8out[4]) << "A null wide character in a Latin-1 string should convert to a null UTF-8 character";
   EXPECT_STREQ(u8"A\u00A3C", u8out);

   // Kanji conversion without a null character
   u8out[5] = 'X';
   ASSERT_EQ(5, ucs2utf8(L"A\uF922CDEF", 3, u8out, 10)) << "Convert a wide Kanji string to UTF-8";
   ASSERT_EQ('X', u8out[5]) << "Kanji output characters past allowed input should not change";
   u8out[5] = 0;
   EXPECT_STREQ(u8"A\uF922C", u8out);

   // Kanji conversion with a null character
   u8out[5] = 'X';
   ASSERT_EQ(6, ucs2utf8(L"A\uF922C\x0" L"EF", 4, u8out, 10)) << "Convert a wide Kanji string with a pound sign to UTF-8";
   ASSERT_EQ(0, u8out[5]) << "A null wide character in a Kanji string should convert to a null UTF-8 character";
   EXPECT_STREQ(u8"A\uF922C", u8out);
}

///
/// @brief  Tests converting incomplete surrogate paits into the replacement character.
///
TEST(Unicode, UCS2toUTF8StrLenSurPairTest)
{
   char u8out[16 * 4 + 1] = {};     // maximum 16 wide characters and a null character

   // high surrogate pairs should be silently ignored and should not appear in the output
   u8out[5] = 'X';
   ASSERT_EQ(5, ucs2utf8(L"A\xD800" L"CEF", 3, u8out, 10)) << "Convert a wide string with a high surrogate pair as the second wide character to UTF-8";
   ASSERT_EQ('X', u8out[5]) << "The character past allowed input should not change";
   u8out[5] = 0;
   EXPECT_STREQ(u8"A\uFFFDC", u8out) << "Output should contain the replacement character instead the high surrogate pair";

   u8out[5] = 'X';
   ASSERT_EQ(6, ucs2utf8(L"A\xD800" L"C\x0" L"EF", 4, u8out, 10)) << "Convert a wide string with a high surrogate pair and a null wide character to UTF-8";
   ASSERT_EQ(0, u8out[5]) << "The character past allowed input should not change";
   EXPECT_STREQ(u8"A\uFFFDC", u8out) << "Output should contain the replacement character instead the high surrogate pair";

   // low surrogate pairs should be silently ignored and should not appear in the output
   u8out[5] = 'X';
   ASSERT_EQ(5, ucs2utf8(L"A\xDC00" L"CEF", 3, u8out, 10)) << "Convert a wide string with a low surrogate pair as the second wide character to UTF-8";
   ASSERT_EQ('X', u8out[5]) << "The character past allowed input should not change";
   u8out[5] = 0;
   EXPECT_STREQ(u8"A\uFFFDC", u8out) << "Output should contain the replacement character instead the low surrogate pair";

   u8out[5] = 'X';
   ASSERT_EQ(6, ucs2utf8(L"A\xDC00" L"C\x0" L"EF", 4, u8out, 10)) << "Convert a wide string with a low surrogate pair and a null wide character to UTF-8";
   ASSERT_EQ(0, u8out[5]) << "The character past allowed input should not change";
   EXPECT_STREQ(u8"A\uFFFDC", u8out) << "Output should contain the replacement character instead the low surrogate pair";
}

///
/// @brief  Tests lower case conversion for ASCII characters.
///
/// This test is not a generic string case conversion test, but rather to test the code
/// that checks UTF-8 characters being converted are in the ASCII range.
///
TEST(Unicode, CharToLowerCaseTest)
{
   for(char ch = 'A'; ch <= 'Z'; ch++)
      ASSERT_EQ('a' + (ch - 'A'), string_t::tolower(ch));

   EXPECT_EQ('x', string_t::tolower('x')) << "Lower case characters should not change";

   EXPECT_EQ('@', string_t::tolower('@')) << "Punctuation characters should not be converted to lower case";

   EXPECT_EQ('5', string_t::tolower('5')) << "Digits should not be converted to lower case";

   // test a byte that does not start a valid UTF-8 sequence
   EXPECT_EQ('\xA3', string_t::tolower('\xA3')) << "A non-UTF-8 character should not change";

   // test a byte that starts a valid UTF-8 two-byte sequence
   EXPECT_EQ('\xC2', string_t::tolower('\xC2')) << "A first byte of a UTF-8 character sequence should not change";
}

///
/// @brief  Tests upper case conversion for ASCII characters.
///
/// This test is not a generic string case conversion test, but rather to test the code
/// that checks UTF-8 characters being converted are in the ASCII range.
///
TEST(Unicode, CharToUpperCaseTest)
{
   for(char ch = 'a'; ch <= 'a'; ch++)
      ASSERT_EQ('A' + (ch - 'a'), string_t::toupper(ch));

   EXPECT_EQ('X', string_t::toupper('X')) << "Upper case characters should not change";

   EXPECT_EQ('@', string_t::toupper('@')) << "Punctuation characters should not be converted to upper case";

   EXPECT_EQ('5', string_t::toupper('5')) << "Digits should not be converted to upper case";

   // test a byte that does not start a valid UTF-8 sequence
   EXPECT_EQ('\xA3', string_t::toupper('\xA3')) << "A non-UTF-8 character should not change";

   // test a byte that starts a valid UTF-8 two-byte sequence
   EXPECT_EQ('\xC2', string_t::toupper('\xC2')) << "A first byte of a UTF-8 character sequence should not change";
}
