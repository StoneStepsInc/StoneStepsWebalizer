/*
   webalizer - a web server log analysis program

   Copyright (c) 2004-2017, Stone Steps Inc. (www.stonesteps.ca)

   See COPYING and Copyright files for additional licensing and copyright information 
   
   ut_normurl.cpp
*/
#include "pchtest.h"

#include "../util_url.h"

namespace mstest = Microsoft::VisualStudio::CppUnitTestFramework;

//
// 
//
namespace sswtest {
TEST_CLASS(URLNormalizer) {
   public:
      BEGIN_TEST_METHOD_ATTRIBUTE(NormalizeASCII)
         TEST_DESCRIPTION(L"Normalize ASCII strings without URL-encoded characters")
         TEST_METHOD_ATTRIBUTE(L"Category", L"URL")
      END_TEST_METHOD_ATTRIBUTE()

      TEST_METHOD(NormalizeASCII)
      {
         string_t str;
         string_t::char_buffer_t strbuf;

         str = "abc";
         norm_url_str(str, strbuf);
         mstest::Assert::AreEqual("abc", str, L"Normalize an ASCII string without URL-encoded characters");
      }

      BEGIN_TEST_METHOD_ATTRIBUTE(NormalizeReserved)
         TEST_DESCRIPTION(L"Normalize strings with reserved URL characters")
         TEST_METHOD_ATTRIBUTE(L"Category", L"URL")
      END_TEST_METHOD_ATTRIBUTE()

      TEST_METHOD(NormalizeReserved)
      {
         string_t str;
         string_t::char_buffer_t strbuf;

         str = "abc:/?#[]@!$&'()*+,;=xyz";
         norm_url_str(str, strbuf);
         mstest::Assert::AreEqual("abc:/?#[]@!$&'()*+,;=xyz", str, L"Normalize a URL string with reserved characters");
      }

      BEGIN_TEST_METHOD_ATTRIBUTE(NormalizeMalformed)
         TEST_DESCRIPTION(L"Normalize malformed strings")
         TEST_METHOD_ATTRIBUTE(L"Category", L"URL")
      END_TEST_METHOD_ATTRIBUTE()

      TEST_METHOD(NormalizeMalformed)
      {
         string_t str;
         string_t::char_buffer_t strbuf;

         str = "abc%xyz";
         norm_url_str(str, strbuf);
         mstest::Assert::AreEqual("abc%25xyz", str, L"Normalize an ASCII string with an out-of-place % character");

         str = "abc%";
         norm_url_str(str, strbuf);
         mstest::Assert::AreEqual("abc%25", str, L"Normalize an ASCII string with an out-of-place % character at the end of the string");
      }

      BEGIN_TEST_METHOD_ATTRIBUTE(NormalizeCP1252)
         TEST_DESCRIPTION(L"Normalize strings containing CP1252 characters")
         TEST_METHOD_ATTRIBUTE(L"Category", L"URL")
      END_TEST_METHOD_ATTRIBUTE()

      TEST_METHOD(NormalizeCP1252)
      {
         string_t str;
         string_t::char_buffer_t strbuf;

         str = "abc\xA3xyz";
         norm_url_str(str, strbuf);
         mstest::Assert::AreEqual("abc\xC2\xA3xyz", str, L"Normalize a string with a CP1252 character");
      }

      BEGIN_TEST_METHOD_ATTRIBUTE(NormalizeURLEncoded)
         TEST_DESCRIPTION(L"Normalize strings containing URL-encoded characters")
         TEST_METHOD_ATTRIBUTE(L"Category", L"URL")
      END_TEST_METHOD_ATTRIBUTE()

      TEST_METHOD(NormalizeURLEncoded)
      {
         string_t str;
         string_t::char_buffer_t strbuf;

         str = "abc%25xyz";
         norm_url_str(str, strbuf);
         mstest::Assert::AreEqual("abc%25xyz", str, L"Normalize an ASCII string with a properly encoded % character");

         str = "abc%41%45xyz";
         norm_url_str(str, strbuf);
         mstest::Assert::AreEqual("abcAExyz", str, L"Normalize an ASCII string with unnecessarily encoded A and E characters");

         str = "abc%41%3D%45xyz";
         norm_url_str(str, strbuf);
         mstest::Assert::AreEqual("abcA%3DExyz", str, L"Normalize a string with a mix of properly and unnecessarily encoded characters");

         str = "abc%3a%3d%3fxyz";
         norm_url_str(str, strbuf);
         mstest::Assert::AreEqual("abc%3A%3D%3Fxyz", str, L"Normalize a string with lower-case URL-encoded hex characters");
      }

      BEGIN_TEST_METHOD_ATTRIBUTE(NormalizeURLEncodedCP1252)
         TEST_DESCRIPTION(L"Normalize strings containing URL-encoded CP1252 characters")
         TEST_METHOD_ATTRIBUTE(L"Category", L"URL")
      END_TEST_METHOD_ATTRIBUTE()

      TEST_METHOD(NormalizeURLEncodedCP1252)
      {
         string_t str;
         string_t::char_buffer_t strbuf;

         str = "abc%A3xyz";
         norm_url_str(str, strbuf);
         mstest::Assert::AreEqual("abc\xC2\xA3xyz", str, L"Normalize a string with a URL-encoded CP1252 character");
      }

      BEGIN_TEST_METHOD_ATTRIBUTE(NormalizeURLEncodedKanji)
         TEST_DESCRIPTION(L"Normalize strings containing URL-encoded Kanji characters")
         TEST_METHOD_ATTRIBUTE(L"Category", L"URL")
      END_TEST_METHOD_ATTRIBUTE()

      TEST_METHOD(NormalizeURLEncodedKanji)
      {
         string_t str;
         string_t::char_buffer_t strbuf;

         str = "abc%E8%A8%98xyz";
         norm_url_str(str, strbuf);
         mstest::Assert::AreEqual(u8"abc\u8A18xyz", str, L"Normalize a string with a URL-encoded Kanji character");

         str = "abcA=%E8%A8%98xyz";
         norm_url_str(str, strbuf);
         mstest::Assert::AreEqual(u8"abcA=\u8A18xyz", str, L"Normalize a string with a URL-encoded Kanji character and an equal sign");

         str = "abcA=%26%E8%A8%98xyz";
         norm_url_str(str, strbuf);
         mstest::Assert::AreEqual(u8"abcA=%26\u8A18xyz", str, L"Normalize a string with a URL-encoded Kanji character and a URL-encoded ampersand");
      }

      BEGIN_TEST_METHOD_ATTRIBUTE(NormalizeUTF8)
         TEST_DESCRIPTION(L"Normalize strings containing well-formed UTF-8 characters")
         TEST_METHOD_ATTRIBUTE(L"Category", L"URL")
      END_TEST_METHOD_ATTRIBUTE()

      TEST_METHOD(NormalizeUTF8)
      {
         string_t str;
         string_t::char_buffer_t strbuf;

         str = u8"abc\u68A1xyz\u6890";
         norm_url_str(str, strbuf);
         mstest::Assert::AreEqual("abc\xE6\xA2\xA1xyz\xE6\xA2\x90", str, L"Normalize a string with a Kanji character encoded in UTF-8");

         str = u8"abc\u00A3xyz";
         norm_url_str(str, strbuf);
         mstest::Assert::AreEqual("abc\xC2\xA3xyz", str, L"Normalize a string with a Latin1 character encoded in UTF-8");
      }

      BEGIN_TEST_METHOD_ATTRIBUTE(NormalizeControl)
         TEST_DESCRIPTION(L"Normalize strings containing control characters")
         TEST_METHOD_ATTRIBUTE(L"Category", L"URL")
      END_TEST_METHOD_ATTRIBUTE()

      TEST_METHOD(NormalizeControl)
      {
         string_t str;
         string_t::char_buffer_t strbuf;

         // control characters are URL-encoded to make them visible
         str = "x\x01.\x05.\x10.\x19.\x20y";
         norm_url_str(str, strbuf);
         mstest::Assert::AreEqual("x%01.%05.%10.%19. y", str, L"Normalize a string with control characters");

         // URL-encoded control characters are left unchanged
         str = "x%01.%05.%10.%19.%20y";
         norm_url_str(str, strbuf);
         mstest::Assert::AreEqual("x%01.%05.%10.%19. y", str, L"Normalize a string with URL-encoded control characters");
      }

      BEGIN_TEST_METHOD_ATTRIBUTE(URLEncodeNormString)
         TEST_DESCRIPTION(L"URL-encode a normalized URL string")
         TEST_METHOD_ATTRIBUTE(L"Category", L"URL")
      END_TEST_METHOD_ATTRIBUTE()

      TEST_METHOD(URLEncodeNormString)
      {
         string_t str, out;

         str = "x%25y";
         url_encode(str, out);
         mstest::Assert::AreEqual("x%25y", out, L"One-byte characters are not URL-encoded");

         str = "x\xC2\xA3y";
         url_encode(str, out);
         mstest::Assert::AreEqual("x%C2%A3y", out, L"Multi-byte CP1252 characters are URL-encoded");

         str = "x\xE8\xA8\x98y";
         url_encode(str, out);
         mstest::Assert::AreEqual("x%E8%A8%98y", out, L"Multi-byte Kanji characters are URL-encoded");

         str = "x y";
         url_encode(str, out);
         mstest::Assert::AreEqual("x%20y", out, L"Space is URL-encoded");

         str = "x\t\r\n\v\fy";
         url_encode(str, out);
         mstest::Assert::AreEqual("x%09%0D%0A%0B%0Cy", out, L"Control characters are URL-encoded");
      }
};
}
