/*
   webalizer - a web server log analysis program

   Copyright (c) 2004-2022, Stone Steps Inc. (www.stonesteps.ca)

   See COPYING and Copyright files for additional licensing and copyright information 
   
   ut_normurl.cpp
*/
#include "pch.h"

#include "../util_url.h"

namespace sswtest {

///
/// @brief  Normalize ASCII strings without URL-encoded characters
///
TEST(URLNormalizerTest, NormalizeASCII)
{
   string_t str;
   string_t::char_buffer_t strbuf;

   str = "abc";
   norm_url_str(str, strbuf);
   EXPECT_STREQ("abc", str) << "Normalize an ASCII string without URL-encoded characters";
}

///
/// @brief  Normalize strings with reserved URL characters
///
TEST(URLNormalizerTest, NormalizeReserved)
{
   string_t str;
   string_t::char_buffer_t strbuf;

   str = "abc:/?#[]@!$&'()*+,;=xyz";
   norm_url_str(str, strbuf);
   EXPECT_STREQ("abc:/?#[]@!$&'()*+,;=xyz", str) << "Normalize a URL string with reserved characters";
}

///
/// @brief  Normalize malformed strings
///
TEST(URLNormalizerTest, NormalizeMalformed)
{
   string_t str;
   string_t::char_buffer_t strbuf;

   str = "abc%xyz";
   norm_url_str(str, strbuf);
   EXPECT_STREQ("abc%25xyz", str) << "Normalize an ASCII string with an out-of-place % character";

   str = "abc%";
   norm_url_str(str, strbuf);
   EXPECT_STREQ("abc%25", str) << "Normalize an ASCII string with an out-of-place % character at the end of the string";
}

///
/// @brief  Normalize strings containing CP1252 characters
///
TEST(URLNormalizerTest, NormalizeCP1252)
{
   string_t str;
   string_t::char_buffer_t strbuf;

   str = "abc\xA3xyz";
   norm_url_str(str, strbuf);
   EXPECT_STREQ("abc\xC2\xA3xyz", str) << "Normalize a string with a CP1252 character";
}

///
/// @brief  Normalize strings containing URL-encoded characters
///
TEST(URLNormalizerTest, NormalizeURLEncoded)
{
   string_t str;
   string_t::char_buffer_t strbuf;

   str = "abc%25xyz";
   norm_url_str(str, strbuf);
   EXPECT_STREQ("abc%25xyz", str) << "Normalize an ASCII string with a properly encoded % character";

   str = "abc%41%45xyz";
   norm_url_str(str, strbuf);
   EXPECT_STREQ("abcAExyz", str) << "Normalize an ASCII string with unnecessarily encoded A and E characters";

   str = "abc%41%3D%45xyz";
   norm_url_str(str, strbuf);
   EXPECT_STREQ("abcA%3DExyz", str) << "Normalize a string with a mix of properly and unnecessarily encoded characters";

   str = "abc%3a%3d%3fxyz";
   norm_url_str(str, strbuf);
   EXPECT_STREQ("abc%3A%3D%3Fxyz", str) << "Normalize a string with lower-case URL-encoded hex characters";
}

///
/// @brief  Normalize strings containing URL-encoded CP1252 characters
///
TEST(URLNormalizerTest, NormalizeURLEncodedCP1252)
{
   string_t str;
   string_t::char_buffer_t strbuf;

   str = "abc%A3xyz";
   norm_url_str(str, strbuf);
   EXPECT_STREQ("abc\xC2\xA3xyz", str) << "Normalize a string with a URL-encoded CP1252 character";
}

///
/// @brief  Normalize strings containing URL-encoded Kanji characters
///
TEST(URLNormalizerTest, NormalizeURLEncodedKanji)
{
   string_t str;
   string_t::char_buffer_t strbuf;

   str = "abc%E8%A8%98xyz";
   norm_url_str(str, strbuf);
   EXPECT_STREQ(u8"abc\u8A18xyz", str) << "Normalize a string with a URL-encoded Kanji character";

   str = "abcA=%E8%A8%98xyz";
   norm_url_str(str, strbuf);
   EXPECT_STREQ(u8"abcA=\u8A18xyz", str) << "Normalize a string with a URL-encoded Kanji character and an equal sign";

   str = "abcA=%26%E8%A8%98xyz";
   norm_url_str(str, strbuf);
   EXPECT_STREQ(u8"abcA=%26\u8A18xyz", str) << "Normalize a string with a URL-encoded Kanji character and a URL-encoded ampersand";
}

///
/// @brief  Normalize strings containing well-formed UTF-8 characters
///
TEST(URLNormalizerTest, NormalizeUTF8)
{
   string_t str;
   string_t::char_buffer_t strbuf;

   str = u8"abc\u68A1xyz\u6890";
   norm_url_str(str, strbuf);
   EXPECT_STREQ("abc\xE6\xA2\xA1xyz\xE6\xA2\x90", str) << "Normalize a string with a Kanji character encoded in UTF-8";

   str = u8"abc\u00A3xyz";
   norm_url_str(str, strbuf);
   EXPECT_STREQ("abc\xC2\xA3xyz", str) << "Normalize a string with a Latin1 character encoded in UTF-8";
}

///
/// @brief  Normalize strings containing control characters
///
TEST(URLNormalizerTest, NormalizeControl)
{
   string_t str;
   string_t::char_buffer_t strbuf;

   // control characters are URL-encoded to make them visible
   str = "x\x01.\x05.\x10.\x19.\x20\x7Fy";
   norm_url_str(str, strbuf);
   EXPECT_STREQ("x%01.%05.%10.%19. %7Fy", str) << "Normalize a string with control characters";

   // URL-encoded control characters are left unchanged
   str = "x%01.%05.%10.%19.%20%7Fy";
   norm_url_str(str, strbuf);
   EXPECT_STREQ("x%01.%05.%10.%19. %7Fy", str) << "Normalize a string with URL-encoded control characters";
}

///
/// @brief  Normalize strings containing characters to encode in different order///

TEST(URLNormalizerTest, NormalizeCharacterOrder)
{
   string_t str;
   string_t::char_buffer_t strbuf;

   str = "x\x01%25\xA3y";
   norm_url_str(str, strbuf);
   EXPECT_STREQ("x%01%25\xC2\xA3y", str) << "Normalize a string with control, encoded, Latin";

   str = "x\x7F%25\xA3y";
   norm_url_str(str, strbuf);
   EXPECT_STREQ("x%7F%25\xC2\xA3y", str) << "Normalize a string with control (2), encoded, Latin";

   str = "x%25\x01\xA3y";
   norm_url_str(str, strbuf);
   EXPECT_STREQ("x%25%01\xC2\xA3y", str) << "Normalize a string with encoded, control, Latin";

   str = "x\xA3%25\x01y";
   norm_url_str(str, strbuf);
   EXPECT_STREQ("x\xC2\xA3%25%01y", str) << "Normalize a string with Latin, encoded, control";
}

///
/// @brief  URL-encode a normalized URL string
///
TEST(URLNormalizerTest, URLEncodeNormString)
{
   string_t str, out;

   str = "x%25y";
   url_encode(str, out);
   EXPECT_STREQ("x%25y", out) << "One-byte characters are not URL-encoded";

   str = "x\xC2\xA3y";
   url_encode(str, out);
   EXPECT_STREQ("x%C2%A3y", out) << "Multi-byte CP1252 characters are URL-encoded";

   str = "x\xE8\xA8\x98y";
   url_encode(str, out);
   EXPECT_STREQ("x%E8%A8%98y", out) << "Multi-byte Kanji characters are URL-encoded";

   str = "x y";
   url_encode(str, out);
   EXPECT_STREQ("x%20y", out) << "Space is URL-encoded";

   str = "x\t\r\n\v\fy";
   url_encode(str, out);
   EXPECT_STREQ("x%09%0D%0A%0B%0Cy", out) << "Control characters are URL-encoded";

   str = "x\"<>y";
   url_encode(str, out);
   EXPECT_STREQ("x%22%3C%3Ey", out) << "Characters that are not explicitly listed in reserved and unreserved sets URL-encoded";
}

///
/// @brief  Test encoding a special character at the end of the buffer.
///
/// This test is specifically for a bug caused by a special URL character, like a
/// double quote, located at the end of the string that had only either one or two
/// bytes available at the end of the buffer. The output size for a a few special
/// characters tested here was calculated as if the character was not encoded and
/// then an encoded character was placed in the buffer, which triggered a runtime
/// exception.
///
TEST(URLNormalizerTest, URLEncodeBufEndSpecialCharacter)
{
   string_t in;
   const char spch[] = "\"<>";

   // test the case with two bytes remaining in the buffer (no room for the last pct-seq character and the null terminator)
   for(const char *cp = spch; *cp; cp++) {
      string_t out;

      // string buffer is allocated in multiples of 4, plus the null character -> 5 bytes
      out.reserve(3);
      in = "123";
      in += *cp;

      EXPECT_NO_THROW(url_encode(in, out)) << "A special character at the end of the string shouldn't overwrite one byte in the buffer";
      EXPECT_STREQ(string_t::_format("123%%%02X", (unsigned char) *cp).c_str(), out.c_str());
   }

   // test the case with three bytes remaining in the buffer (no room for the null character)
   for(const char *cp = spch; *cp; cp++) {
      string_t out;

      out.reserve(3);
      in = "12";
      in += *cp;

      EXPECT_NO_THROW(url_encode(in, out)) << "A special character at the end of the string should leave room for a null character";
      EXPECT_STREQ(string_t::_format("12%%%02X", (unsigned char) *cp).c_str(), out.c_str());
   }
}
}
