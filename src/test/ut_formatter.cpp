/*
   webalizer - a web server log analysis program

   Copyright (c) 2004-2019, Stone Steps Inc. (www.stonesteps.ca)

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

   //
   // The encoder expects to have enough room at the end of the buffer for the maximum encoded
   // sequence length for the last character, which is 6 for HTML encoding, plus the null character.
   // For a 32-byte buffer that means we can output 26 characters (i.e. 25 characters, plus the last
   // one that will be checked for 6 bytes, plus the null character: 25 + 6 + 1 = 32), even though
   // it leaves 5 bytes empty.
   //

   // try without special characters first (this leaves 5 empty bytes at the end of the buffer)
   ASSERT_NO_THROW(formatter.format(encode_string<encode_char_html>, "12345678901234567890123456")) << "26 characters should fit into a 32-byte buffer";

   ASSERT_THROW(formatter.format(encode_string<encode_char_html>, "123456789012345678901234567"), exception_t) << "27 characters should not fit into a 32-byte buffer";

   // a single quote will expand to the end of the buffer, minus the null character
   ASSERT_NO_THROW(formatter.format(encode_string<encode_char_html>, "1234567890123456789012345'")) << "26 characters, with one 6-byte special character, should fit into a 32-byte buffer";

   // one more character leaves no room for the null character
   ASSERT_THROW(formatter.format(encode_string<encode_char_html>, "12345678901234567890123456'"), exception_t) << "26 characters, with one 6-byte special character, should not fit into a 32-byte buffer";
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

   //
   // See the comment in the HTML formatter
   //

   // try without special characters first (this leaves 5 empty bytes at the end of the buffer)
   ASSERT_NO_THROW(formatter.format(encode_string<encode_char_xml>, "12345678901234567890123456")) << "26 characters should fit into a 32-byte buffer";

   ASSERT_THROW(formatter.format(encode_string<encode_char_xml>, "123456789012345678901234567"), exception_t) << "27 characters should not fit into a 32-byte buffer";

   // a single quote will expand to the end of the buffer, minus the null character
   ASSERT_NO_THROW(formatter.format(encode_string<encode_char_xml>, "1234567890123456789012345'")) << "26 characters, with one 6-byte special character, should fit into a 32-byte buffer";

   // one more character leaves no room for the null character
   ASSERT_THROW(formatter.format(encode_string<encode_char_xml>, "12345678901234567890123456'"), exception_t) << "26 characters, with one 6-byte special character, should not fit into a 32-byte buffer";
}

///
/// @brief  Tests a JavaScript encoder formatter.
///
TEST_F(FormatterTest, FormatterEncodeJavaScript)
{
   // use hex representation for UTF-8 bytes to make the transformation more visible
   formatter.format(encode_string<encode_char_js>, "\"'\r\n\xE2\x80\xA8\xE2\x80\xA9");

   EXPECT_STREQ(R"==(\"\'\r\n\u2028\u2029)==", buffer) << "All JavaScript special characters must be encoded";

   formatter.format(encode_string<encode_char_js>, R"==(123"abc"456)==");

   EXPECT_STREQ(R"==(123\"abc\"456)==", buffer) << "Characters that are not JavaScript-special should not be changed";

   //
   // See the comment in the HTML formatter
   //

   // try without special characters first (this leaves 5 empty bytes at the end of the buffer)
   ASSERT_NO_THROW(formatter.format(encode_string<encode_char_js>, "12345678901234567890123456")) << "26 characters should fit into a 32-byte buffer";

   ASSERT_THROW(formatter.format(encode_string<encode_char_js>, "123456789012345678901234567"), exception_t) << "27 characters should not fit into a 32-byte buffer";

   // a single quote will expand to the end of the buffer, minus the null character
   ASSERT_NO_THROW(formatter.format(encode_string<encode_char_js>, "1234567890123456789012345\xE2\x80\xA8")) << "26 characters, with one 6-byte special character, should fit into a 32-byte buffer";

   // one more character leaves no room for the null character
   ASSERT_THROW(formatter.format(encode_string<encode_char_js>, "12345678901234567890123456\xE2\x80\xA8"), exception_t) << "26 characters, with one 6-byte special character, should not fit into a 32-byte buffer";
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
