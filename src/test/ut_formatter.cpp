/*
   webalizer - a web server log analysis program

   Copyright (c) 2004-2021, Stone Steps Inc. (www.stonesteps.ca)

   See COPYING and Copyright files for additional licensing and copyright information 
   
   ut_formatter.cpp
*/
#include "pch.h"

#include "../fmt_impl.h"
#include "../formatter.h"
#include "../encoder.h"
#include "../exception.h"

#include <locale>

namespace sswtest {
class FormatterTest : public testing::Test {
   protected:
      const size_t BUFSIZE = 32;
      char *buffer;
      buffer_formatter_t formatter;

   public:
      FormatterTest(void) : 
         buffer(new char[BUFSIZE]), 
         formatter(buffer, BUFSIZE, buffer_formatter_t::overwrite)
      {
      }

      ~FormatterTest(void)
      {
         delete [] buffer;
      }
};

///
/// @brief  Print numbers in a human-readable format with decimal K suffix.
///
TEST_F(FormatterTest, HumanReadable1000)
{
   const char *suffix[] = {"K", "M", "G", "T", "P", "E", "Z"};

   EXPECT_STREQ("0", formatter.format(fmt_hr_num, 0, nullptr, suffix, "B", true));
   EXPECT_STREQ("10", formatter.format(fmt_hr_num, 10, nullptr, suffix, "B", true));
   EXPECT_STREQ("1KB", formatter.format(fmt_hr_num, 1'000, nullptr, suffix, "B", true));
   EXPECT_STREQ("1.2KB", formatter.format(fmt_hr_num, 1'234, nullptr, suffix, "B", true));
   EXPECT_STREQ("10.5MB", formatter.format(fmt_hr_num, 10'500'000, nullptr, suffix, "B", true));
   EXPECT_STREQ("12.5GB", formatter.format(fmt_hr_num, 12'456'567'890, nullptr, suffix, "B", true));
}

///
/// @brief  Print numbers in a human-readable format with a binary K suffix.
///
TEST_F(FormatterTest, HumanReadable1024)
{
   const char *suffix[] = {"K", "M", "G", "T", "P", "E", "Z"};

   EXPECT_STREQ("0", formatter.format(fmt_hr_num, 0, nullptr, suffix, "B", false));
   EXPECT_STREQ("10", formatter.format(fmt_hr_num, 10, nullptr, suffix, "B", false));
   EXPECT_STREQ("1KB", formatter.format(fmt_hr_num, 1'024, nullptr, suffix, "B", false));
   EXPECT_STREQ("1.2KB", formatter.format(fmt_hr_num, 1'234, nullptr, suffix, "B", false));
   EXPECT_STREQ("10MB", formatter.format(fmt_hr_num, 10'500'000, nullptr, suffix, "B", false));
   EXPECT_STREQ("11.6GB", formatter.format(fmt_hr_num, 12'456'567'890, nullptr, suffix, "B", false));
}

///
/// @brief  Print numbers in a human-readable format with a different separator.
///
TEST_F(FormatterTest, HumanReadableSep)
{
   const char *suffix[] = {"K", "M", "G", "T", "P", "E", "Z"};

   EXPECT_STREQ("0", formatter.format(fmt_hr_num, 0, " ", suffix, "B", true));
   EXPECT_STREQ("10", formatter.format(fmt_hr_num, 10, " ", suffix, "B", true));
   EXPECT_STREQ("1.2 KB", formatter.format(fmt_hr_num, 1234, " ", suffix, "B", true));
   EXPECT_STREQ("1.2&nbsp;KB", formatter.format(fmt_hr_num, 1234, "&nbsp;", suffix, "B", true));

   EXPECT_STREQ("0", formatter.format(fmt_hr_num, 0, nullptr, suffix, "B", true));
   EXPECT_STREQ("10", formatter.format(fmt_hr_num, 10, nullptr, suffix, "B", true));
   EXPECT_STREQ("1.2KB", formatter.format(fmt_hr_num, 1234, nullptr, suffix, "B", true));
}

///
/// @brief  Print numbers in a human-readable format with a different unit.
///
TEST_F(FormatterTest, HumanReadableUnit)
{
   const char *suffix[] = {"K", "M", "G", "T", "P", "E", "Z"};

   EXPECT_STREQ("0", formatter.format(fmt_hr_num, 0, " ", suffix, "B", true));
   EXPECT_STREQ("0", formatter.format(fmt_hr_num, 0, " ", suffix, nullptr, true));

   EXPECT_STREQ("10", formatter.format(fmt_hr_num, 10, " ", suffix, "B", true));
   EXPECT_STREQ("10", formatter.format(fmt_hr_num, 10, " ", suffix, nullptr, true));

   EXPECT_STREQ("1.2 KB", formatter.format(fmt_hr_num, 1234, " ", suffix, "B", true));
   EXPECT_STREQ("1.2 K", formatter.format(fmt_hr_num, 1234, " ", suffix, nullptr, true));
}

///
/// @brief  Print using printf-style formats.
///
TEST_F(FormatterTest, VSPrintF)
{
   buffer_formatter_t& formatter = this->formatter;
   auto fmt_printf = [&formatter](const char *fmt, ...) -> const char *
   {
      va_list args;
      va_start(args, fmt);

      struct va_list_wrapper_t {
         va_list& args;
         ~va_list_wrapper_t() {va_end(args);}
      } va_list_wrapper = {args};

      return formatter.format(fmt_vprintf, fmt, args);
   };

   EXPECT_STREQ("(0)", fmt_printf("(%d)", 0));
   EXPECT_STREQ("10", fmt_printf("%d", 10));
   EXPECT_STREQ("x=1.235", fmt_printf("x=%.3f", 1.23456));
   EXPECT_STREQ("x=1235", fmt_printf("x=%.0f", 1234.5678));
   EXPECT_STREQ("abc=xyz", fmt_printf("abc=%s", "xyz"));
}

///
/// @brief  Tests a formatter scope with an append mode.
///
TEST_F(FormatterTest, FormatterScope)
{
   // a formatting function that converts two first characters of the input to upper case and discards the rest
   auto to_upper = [] (string_t::char_buffer_t& buffer, const char *cptr) -> size_t
   {
      if(buffer.capacity() < 3)
         throw std::logic_error("Buffer is too small");

      buffer[0] = std::toupper(*cptr, std::locale::classic());
      buffer[1] = std::toupper(*++cptr, std::locale::classic());
      buffer[2] = '\x0';

      return 3;
   };

   formatter.format(to_upper, "xyz");

   EXPECT_STREQ(buffer, "XY") << "Buffer should contain first two characters converted to upper case";

   formatter.format(to_upper, "abc");

   EXPECT_STREQ(buffer, "AB") << "In the overrite mode each formatting call should overwrite the buffer";

   {
      //
      // The compiler may optimize out the move constructor call, so get a reference
      // first and then explicitly move it to make sure to test how scopes are moved.
      //
      buffer_formatter_t::scope_t&& scope_ref = formatter.set_scope_mode(buffer_formatter_t::append);
      buffer_formatter_t::scope_t scope(std::move(scope_ref));

      formatter.format(to_upper, "def");
      formatter.format(to_upper, "ghi");
      formatter.format(to_upper, "jkl");

      EXPECT_STREQ(buffer, "DE") << "First formatted sequence should be at the start of the buffer";
      EXPECT_STREQ(buffer + 3, "GH") << "Second appended sequence should be at the offset 3";
      EXPECT_STREQ(buffer + 6, "JK") << "Third appended sequence should be at the offset 6";
   }

   formatter.format(to_upper, "xyz");

   EXPECT_STREQ(buffer, "XY") << "The original overwrite mode should be restored after the scoped formatter is destroyed";
}

///
/// @brief  Tests an HTML encoder formatter.
///
TEST_F(FormatterTest, FormatterEncodeHTML)
{
   formatter.format(encode_string<encode_char_html>, "<>&\"'");

   EXPECT_STREQ("&lt;&gt;&amp;&quot;&#x27;", buffer) << "All HTML special characters must be encoded";

   formatter.format(encode_string<encode_char_html>, "123<abc>456");

   EXPECT_STREQ("123&lt;abc&gt;456", buffer) << "Characters that are not HTML-special should not be changed";

   formatter.format(encode_string<encode_char_html>, "\xC2\xA7\xE4\xB8\x81\xE4\xB8\x82\xF0\x9D\x85\xA0");

   EXPECT_STREQ(u8"\u00A7\u4E01\u4E02\U0001D160", buffer) << "Multibyte UTF-8 sequences should not be encoded";

   //
   // The encoder expects to have enough room at the end of the buffer for the maximum encoded
   // sequence length for the last character, which is 6 for HTML encoding, plus the null character.
   // For a 32-byte buffer that means we can output 26 characters (i.e. 25 characters, plus the last
   // one that will be checked for 6 bytes, plus the null character: 25 + 6 + 1 = 32), even though
   // it leaves 5 bytes empty.
   //

   // check that the maximum encoded size is 6 bytes
   size_t mbenc_size = 0;
   encode_char_html(nullptr, 0, nullptr, mbenc_size);
   ASSERT_EQ(6, mbenc_size);

   // try without special characters first (this leaves 5 empty bytes at the end of the buffer)
   ASSERT_NO_THROW(formatter.format(encode_string<encode_char_html>, "12345678901234567890123456")) << "26 characters should fit into a 32-byte buffer";

   ASSERT_THROW(formatter.format(encode_string<encode_char_html>, "123456789012345678901234567"), exception_t) << "27 characters should not fit into a 32-byte buffer";

   // a single quote will expand to the end of the buffer, minus the null character
   ASSERT_NO_THROW(formatter.format(encode_string<encode_char_html>, "1234567890123456789012345'")) << "25 + 6-byte encoded character + null character should fit into a 32-byte buffer";

   // one more character leaves no room for the null character
   ASSERT_THROW(formatter.format(encode_string<encode_char_html>, "12345678901234567890123456'"), exception_t) << "26 + 6-byte encoded character + null character should not fit into a 32-byte buffer";
}

///
/// @brief  Tests an XML encoder formatter.
///
TEST_F(FormatterTest, FormatterEncodeXML)
{
   formatter.format(encode_string<encode_char_xml>, "<>&\"'");

   EXPECT_STREQ("&lt;&gt;&amp;&quot;&apos;", buffer) << "All XML special characters must be encoded";

   formatter.format(encode_string<encode_char_xml>, "123<abc>456");

   EXPECT_STREQ("123&lt;abc&gt;456", buffer) << "Characters that are not XML-special should not be changed";

   formatter.format(encode_string<encode_char_xml>, "\xC2\xA7\xE4\xB8\x81\xE4\xB8\x82\xF0\x9D\x85\xA0");

   EXPECT_STREQ(u8"\u00A7\u4E01\u4E02\U0001D160", buffer) << "Multibyte UTF-8 sequences should not be encoded";

   //
   // See the comment in the HTML formatter
   //

   // check that the maximum encoded size is 6 bytes
   size_t mbenc_size = 0;
   encode_char_xml(nullptr, 0, nullptr, mbenc_size);
   ASSERT_EQ(6, mbenc_size);

   // try without special characters first (this leaves 5 empty bytes at the end of the buffer)
   ASSERT_NO_THROW(formatter.format(encode_string<encode_char_xml>, "12345678901234567890123456")) << "26 characters should fit into a 32-byte buffer";

   ASSERT_THROW(formatter.format(encode_string<encode_char_xml>, "123456789012345678901234567"), exception_t) << "27 characters should not fit into a 32-byte buffer";

   // a single quote will expand to the end of the buffer, minus the null character
   ASSERT_NO_THROW(formatter.format(encode_string<encode_char_xml>, "1234567890123456789012345'")) << "25 + 6-byte encoded character + null character should fit into a 32-byte buffer";

   // one more character leaves no room for the null character
   ASSERT_THROW(formatter.format(encode_string<encode_char_xml>, "12345678901234567890123456'"), exception_t) << "26 + 6-byte encoded character + null character should not fit into a 32-byte buffer";
}

///
/// @brief  Tests a JavaScript encoder formatter.
///
TEST_F(FormatterTest, FormatterEncodeJavaScript)
{
   // use hex representation for UTF-8 bytes to make the transformation more visible
   formatter.format(encode_string<encode_char_js>, "\"'\\\r\n\xE2\x80\xA8\xE2\x80\xA9");

   EXPECT_STREQ(R"==(\"\'\\\r\n\u2028\u2029)==", buffer) << "All JavaScript special characters must be encoded";

   formatter.format(encode_string<encode_char_js>, "\xC2\xA7\xE4\xB8\x81\xE4\xB8\x82\xF0\x9D\x85\xA0");

   EXPECT_STREQ(u8"\u00A7\u4E01\u4E02\U0001D160", buffer) << "Multibyte UTF-8 sequences should not be encoded";

   formatter.format(encode_string<encode_char_js>, R"==(123"abc"456)==");

   EXPECT_STREQ(R"==(123\"abc\"456)==", buffer) << "Characters that are not JavaScript-special should not be changed";

   //
   // See the comment in the HTML formatter
   //

   // check that the maximum encoded sequence size is 6 bytes
   size_t mbenc_size = 0;
   encode_char_js(nullptr, 0, nullptr, mbenc_size);
   ASSERT_EQ(6, mbenc_size);

   // try without special characters first (this leaves 5 empty bytes at the end of the buffer)
   ASSERT_NO_THROW(formatter.format(encode_string<encode_char_js>, "12345678901234567890123456")) << "26 characters should fit into a 32-byte buffer";

   ASSERT_THROW(formatter.format(encode_string<encode_char_js>, "123456789012345678901234567"), exception_t) << "27 characters should not fit into a 32-byte buffer";

   // encoded \u2028 expands to the end of the buffer, minus the null character
   ASSERT_NO_THROW(formatter.format(encode_string<encode_char_js>, "1234567890123456789012345\xE2\x80\xA8")) << "25 + 6-byte encoded character + null character should fit into a 32-byte buffer";

   // one more character leaves no room for the null character
   ASSERT_THROW(formatter.format(encode_string<encode_char_js>, "12345678901234567890123456\xE2\x80\xA8"), exception_t) << "26 + 6-byte encoded character + null character should not fit into a 32-byte buffer";
}

///
/// @brief  Tests a JSON encoder formatter.
///
TEST_F(FormatterTest, FormatterEncodeJSON)
{
   formatter.format(encode_string<encode_char_json>, "\"\\\r\n\t");
   EXPECT_STREQ(R"==(\"\\\r\n\t)==", buffer) << "Special JSON characters must be escaped";

   formatter.format(encode_string<encode_char_json>, "\b\f");
   EXPECT_STREQ(u8"\uE008\uE00C", buffer) << "\b and \f should be treated as control characters and encoded as private-use code points";

   // use hex representation for UTF-8 bytes to make the transformation more visible.
   formatter.format(encode_string<encode_char_json>, "\xC2\xA7\xE4\xB8\x81\xE4\xB8\x82\xF0\x9D\x85\xA0");
   EXPECT_STREQ(u8"\u00A7\u4E01\u4E02\U0001D160", buffer) << "Multibyte UTF-8 sequences should not be escaped";

   // single quote and forward slash should not be escaped
   formatter.format(encode_string<encode_char_json>, R"==(abc'123/XYZ)==");
   EXPECT_STREQ(R"==(abc'123/XYZ)==", buffer) << "Printable ASCII characters, expect those above, should not be escaped";

   //
   // See the comment in the HTML formatter
   //
   // Unlike other encoders, the longest encoded JSON sequence is shorter
   // than the longest UTF-8 character, which changes the bounds in tests
   // below. For 1 and 2 byte characters there must be at least 2 (max
   // encoded sequence) + 1 (null character) bytes at the end. For 3 and
   // 4 byte characters there should be 3 + 1 and 4 + 1 bytes at the end,
   // respectively.
   //

   // check that the maximum encoded sequence size is 2 bytes (not output)
   size_t mbenc_size = 0;
   encode_char_json(nullptr, 0, nullptr, mbenc_size);
   ASSERT_EQ(2, mbenc_size);

   // 1-byte character at the end
   ASSERT_NO_THROW(formatter.format(encode_string<encode_char_json>, "123456789012345678901234567890")) << "30 + null character should fit into a 32-byte buffer";
   ASSERT_THROW(formatter.format(encode_string<encode_char_json>, "1234567890123456789012345678901"), exception_t) << "29 characters should not fit into a 32-byte buffer";

   // 3-byte character at the end
   ASSERT_NO_THROW(formatter.format(encode_string<encode_char_json>, "1234567890123456789012345678\xE4\xB8\x82")) << "28 + 3-byte character + null character should fit into a 32-byte buffer";
   ASSERT_THROW(formatter.format(encode_string<encode_char_json>, "12345678901234567890123456789\xE4\xB8\x82"), exception_t) << "29 + 3-byte character + null character, should not fit into a 32-byte buffer";
}

///
/// @brief  Tests that control characters are converted into Unicode private-range characters.
///
TEST_F(FormatterTest, FormatterEncodeCtrlChars)
{
   char ctlchr[2] = {};
   char utf8[5] = {};

   for(char chr = '\x1'; chr < '\x20'; chr++) {
      // skip acceptable characters
      if(!strchr("\t\r\n", chr)) {
         *ctlchr = chr;

         // all control characters are encoded the same way, so we can use any encoding function
         formatter.format(encode_string<encode_char_html>, ctlchr);

         ucs2utf8(u'\uE000' + (char16_t) chr, utf8);

         ASSERT_STREQ(utf8, buffer) << "A control character should be converted to Unicode private range character";
      }
   }

   *ctlchr = '\x7f';
   formatter.format(encode_string<encode_char_html>, ctlchr);
   ASSERT_STREQ(u8"\uE07F", buffer);

}

}

#include "../formatter_tmpl.cpp"
