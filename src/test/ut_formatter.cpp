/*
   webalizer - a web server log analysis program

   Copyright (c) 2004-2017, Stone Steps Inc. (www.stonesteps.ca)

   See COPYING and Copyright files for additional licensing and copyright information 
   
   ut_formatter.cpp
*/
#include "pchtest.h"

#include "../fmt_impl.h"
#include "../formatter.h"

namespace mstest = Microsoft::VisualStudio::CppUnitTestFramework;

namespace sswtest {
TEST_CLASS(Formatter) {
   const size_t BUFSIZE = 32;
   char *buffer;
   buffer_formatter_t formatter;

   public:
      Formatter(void) : 
         buffer(new char[BUFSIZE]), 
         formatter(buffer, BUFSIZE, buffer_formatter_t::overwrite)
      {
      }

      ~Formatter(void)
      {
         delete [] buffer;
      }

       BEGIN_TEST_METHOD_ATTRIBUTE(HumanReadable1000)
         TEST_DESCRIPTION(L"Print numbers in a human-readable format with decimal K suffix")
         TEST_METHOD_ATTRIBUTE(L"Category", L"Formatter")
       END_TEST_METHOD_ATTRIBUTE()

       TEST_METHOD(HumanReadable1000)
       {
         const char *suffix[] = {"K", "M", "G", "T", "P", "E", "Z"};

         mstest::Assert::AreEqual("0", formatter.format(fmt_hr_num, 0, nullptr, suffix, "B", true));
         mstest::Assert::AreEqual("10", formatter.format(fmt_hr_num, 10, nullptr, suffix, "B", true));
         mstest::Assert::AreEqual("1KB", formatter.format(fmt_hr_num, 1'000, nullptr, suffix, "B", true));
         mstest::Assert::AreEqual("1.2KB", formatter.format(fmt_hr_num, 1'234, nullptr, suffix, "B", true));
         mstest::Assert::AreEqual("10.5MB", formatter.format(fmt_hr_num, 10'500'000, nullptr, suffix, "B", true));
         mstest::Assert::AreEqual("12.5GB", formatter.format(fmt_hr_num, 12'456'567'890, nullptr, suffix, "B", true));
      }

       BEGIN_TEST_METHOD_ATTRIBUTE(HumanReadable1024)
         TEST_DESCRIPTION(L"Print numbers in a human-readable format with a binary K suffix")
         TEST_METHOD_ATTRIBUTE(L"Category", L"Formatter")
       END_TEST_METHOD_ATTRIBUTE()

       TEST_METHOD(HumanReadable1024)
       {
         const char *suffix[] = {"K", "M", "G", "T", "P", "E", "Z"};

         mstest::Assert::AreEqual("0", formatter.format(fmt_hr_num, 0, nullptr, suffix, "B", false));
         mstest::Assert::AreEqual("10", formatter.format(fmt_hr_num, 10, nullptr, suffix, "B", false));
         mstest::Assert::AreEqual("1KB", formatter.format(fmt_hr_num, 1'024, nullptr, suffix, "B", false));
         mstest::Assert::AreEqual("1.2KB", formatter.format(fmt_hr_num, 1'234, nullptr, suffix, "B", false));
         mstest::Assert::AreEqual("10MB", formatter.format(fmt_hr_num, 10'500'000, nullptr, suffix, "B", false));
         mstest::Assert::AreEqual("11.6GB", formatter.format(fmt_hr_num, 12'456'567'890, nullptr, suffix, "B", false));
      }

       BEGIN_TEST_METHOD_ATTRIBUTE(HumanReadableSep)
         TEST_DESCRIPTION(L"Print numbers in a human-readable format with a different separator")
         TEST_METHOD_ATTRIBUTE(L"Category", L"Formatter")
       END_TEST_METHOD_ATTRIBUTE()

       TEST_METHOD(HumanReadableSep)
       {
         const char *suffix[] = {"K", "M", "G", "T", "P", "E", "Z"};

         mstest::Assert::AreEqual("0", formatter.format(fmt_hr_num, 0, " ", suffix, "B", true));
         mstest::Assert::AreEqual("10", formatter.format(fmt_hr_num, 10, " ", suffix, "B", true));
         mstest::Assert::AreEqual("1.2 KB", formatter.format(fmt_hr_num, 1234, " ", suffix, "B", true));
         mstest::Assert::AreEqual("1.2&nbsp;KB", formatter.format(fmt_hr_num, 1234, "&nbsp;", suffix, "B", true));

         mstest::Assert::AreEqual("0", formatter.format(fmt_hr_num, 0, nullptr, suffix, "B", true));
         mstest::Assert::AreEqual("10", formatter.format(fmt_hr_num, 10, nullptr, suffix, "B", true));
         mstest::Assert::AreEqual("1.2KB", formatter.format(fmt_hr_num, 1234, nullptr, suffix, "B", true));
      }

       BEGIN_TEST_METHOD_ATTRIBUTE(HumanReadableUnit)
         TEST_DESCRIPTION(L"Print numbers in a human-readable format with a different unit")
         TEST_METHOD_ATTRIBUTE(L"Category", L"Formatter")
       END_TEST_METHOD_ATTRIBUTE()

       TEST_METHOD(HumanReadableUnit)
       {
         const char *suffix[] = {"K", "M", "G", "T", "P", "E", "Z"};

         mstest::Assert::AreEqual("0", formatter.format(fmt_hr_num, 0, " ", suffix, "B", true));
         mstest::Assert::AreEqual("0", formatter.format(fmt_hr_num, 0, " ", suffix, nullptr, true));

         mstest::Assert::AreEqual("10", formatter.format(fmt_hr_num, 10, " ", suffix, "B", true));
         mstest::Assert::AreEqual("10", formatter.format(fmt_hr_num, 10, " ", suffix, nullptr, true));

         mstest::Assert::AreEqual("1.2 KB", formatter.format(fmt_hr_num, 1234, " ", suffix, "B", true));
         mstest::Assert::AreEqual("1.2 K", formatter.format(fmt_hr_num, 1234, " ", suffix, nullptr, true));
      }

       BEGIN_TEST_METHOD_ATTRIBUTE(VSPrintF)
         TEST_DESCRIPTION(L"Print using printf-style formats")
         TEST_METHOD_ATTRIBUTE(L"Category", L"Formatter")
       END_TEST_METHOD_ATTRIBUTE()

       TEST_METHOD(VSPrintF)
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

         mstest::Assert::AreEqual("(0)", fmt_printf("(%d)", 0));
         mstest::Assert::AreEqual("10", fmt_printf("%d", 10));
         mstest::Assert::AreEqual("x=1.235", fmt_printf("x=%.3f", 1.23456));
         mstest::Assert::AreEqual("x=1235", fmt_printf("x=%.0f", 1234.5678));
         mstest::Assert::AreEqual("abc=xyz", fmt_printf("abc=%s", "xyz"));
      }
};
}

#include "../formatter_tmpl.cpp"
