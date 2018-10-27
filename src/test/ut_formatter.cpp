/*
   webalizer - a web server log analysis program

   Copyright (c) 2004-2018, Stone Steps Inc. (www.stonesteps.ca)

   See COPYING and Copyright files for additional licensing and copyright information 
   
   ut_formatter.cpp
*/
#include "pchtest.h"

#include "../fmt_impl.h"
#include "../formatter.h"

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

}

#include "../formatter_tmpl.cpp"
