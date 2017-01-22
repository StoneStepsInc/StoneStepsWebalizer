/*
   webalizer - a web server log analysis program

   Copyright (c) 2004-2017, Stone Steps Inc. (www.stonesteps.ca)

   See COPYING and Copyright files for additional licensing and copyright information 
   
   ut_ipaddr.cpp
*/
#include "pchtest.h"

#include "../util.h"

namespace mstest = Microsoft::VisualStudio::CppUnitTestFramework;

namespace sswtest {
TEST_CLASS(IPAddress) {
   public:
      BEGIN_TEST_METHOD_ATTRIBUTE(GoodIPv4Addresses)
         TEST_DESCRIPTION(L"Parse proper IPv4 addresses")
         TEST_METHOD_ATTRIBUTE(L"Category", L"IP Address")
      END_TEST_METHOD_ATTRIBUTE()

      TEST_METHOD(GoodIPv4Addresses)
      {
         mstest::Assert::IsTrue(is_ipv4_address("0.0.0.0"), L"0.0.0.0");
         mstest::Assert::IsTrue(is_ipv4_address("12.34.56.78"), L"12.34.56.78");
         mstest::Assert::IsTrue(is_ipv4_address("123.123.123.123"), L"123.123.123.123");
         mstest::Assert::IsTrue(is_ipv4_address("0.123.123.123"), L"0.123.123.123");
         mstest::Assert::IsTrue(is_ipv4_address("123.0.123.123"), L"123.0.123.123");
         mstest::Assert::IsTrue(is_ipv4_address("123.123.0.123"), L"123.123.0.123");
         mstest::Assert::IsTrue(is_ipv4_address("123.123.123.0"), L"123.123.123.0");
         mstest::Assert::IsTrue(is_ipv4_address("127.0.0.1"), L"127.0.0.1");
      }

      BEGIN_TEST_METHOD_ATTRIBUTE(BadIPv4Addresses)
         TEST_DESCRIPTION(L"Parse malformed IPv4 addresses")
         TEST_METHOD_ATTRIBUTE(L"Category", L"IP Address")
      END_TEST_METHOD_ATTRIBUTE()

      TEST_METHOD(BadIPv4Addresses)
      {
         mstest::Assert::IsFalse(is_ipv4_address("0,0.0.0"), L"A non-dot character (comma)");
         mstest::Assert::IsFalse(is_ipv4_address("0.0.0.0.0"), L"Five groups of digits");
         mstest::Assert::IsFalse(is_ipv4_address("0.0.0.1234"), L"More than three digits in a group (1)");
         mstest::Assert::IsFalse(is_ipv4_address("0.0.1234.0"), L"More than three digits in a group (2)");
         mstest::Assert::IsFalse(is_ipv4_address("0.1234.0.0"), L"More than three digits in a group (3)");
         mstest::Assert::IsFalse(is_ipv4_address("1234.0.0.0"), L"More than three digits in a group (4)");
         mstest::Assert::IsFalse(is_ipv4_address("0.a.0.0"), L"A non-decimal character in a group");
         mstest::Assert::IsFalse(is_ipv4_address("0.1a.0.0"), L"A non-decimal character in a group");
         mstest::Assert::IsFalse(is_ipv4_address("abc"), L"abc");
         mstest::Assert::IsFalse(is_ipv4_address("a.b.c.d"), L"a.b.c.d");
      }

      BEGIN_TEST_METHOD_ATTRIBUTE(GoodIPv6Addresses)
         TEST_DESCRIPTION(L"Parse proper IPv6 addresses")
         TEST_METHOD_ATTRIBUTE(L"Category", L"IP Address")
      END_TEST_METHOD_ATTRIBUTE()

      TEST_METHOD(GoodIPv6Addresses)
      {
         mstest::Assert::IsTrue(is_ipv6_address("2017:0db8:85a3:0000:0000:8a2e:0370:7334"), L"2017:0db8:85a3:0000:0000:8a2e:0370:7334");
         mstest::Assert::IsTrue(is_ipv6_address("2017:0DB8:85A3:0000:0000:8A2E:0370:7334"), L"2017:0DB8:85A3:0000:0000:8A2E:0370:7334");
         mstest::Assert::IsTrue(is_ipv6_address("2017:db8:85a3:0:0:8a2e:370:7334"), L"2017:db8:85a3:0:0:8a2e:370:7334");
         mstest::Assert::IsTrue(is_ipv6_address("2017:db8:85a3::8a2e:370:7334"), L"2017:db8:85a3::8a2e:370:7334");
         mstest::Assert::IsTrue(is_ipv6_address("::"), L"::");
         mstest::Assert::IsTrue(is_ipv6_address("::123"), L"::123");
         mstest::Assert::IsTrue(is_ipv6_address("2017::123"), L"2017::123");
         mstest::Assert::IsTrue(is_ipv6_address("::ffff:c000:0280"), L"::ffff:c000:0280");
         mstest::Assert::IsTrue(is_ipv6_address("::ffff:192.0.2.128"), L"::ffff:192.0.2.128");
         mstest::Assert::IsTrue(is_ipv6_address("::192.0.2.128"), L"::192.0.2.128");
      }

      BEGIN_TEST_METHOD_ATTRIBUTE(BadIPv6Addresses)
         TEST_DESCRIPTION(L"Parse malformed IPv6 addresses")
         TEST_METHOD_ATTRIBUTE(L"Category", L"IP Address")
      END_TEST_METHOD_ATTRIBUTE()

      TEST_METHOD(BadIPv6Addresses)
      {
         mstest::Assert::IsFalse(is_ipv6_address("2017:00db8:85a3:0000:0000:8a2e:0370:7334"), L"More than four characters in a hex group");
         mstest::Assert::IsFalse(is_ipv6_address("2017:0Xb8:85a3:0000:0000:8a2e:0370:7334"), L"A non-hex character (X) in a group");
         mstest::Assert::IsFalse(is_ipv6_address("2017::2017:0db8:85a3:0000:0000:8a2e:0370:7334"), L"Nine hex groups instead of eight");
         mstest::Assert::IsFalse(is_ipv6_address("2017:0db8::0000:8a2e::7334"), L"Two double-colon sequences");
         mstest::Assert::IsFalse(is_ipv6_address("::ffff:192.0.2.128.1"), L"Five groups of the IPv4 part of the address");
         mstest::Assert::IsFalse(is_ipv6_address("::ffff:192,0.2.128"), L"A non-dot character (comma) in the IPv4 part of the address");
         mstest::Assert::IsFalse(is_ipv6_address("::ffff:192.X.2.128"), L"A non-hex digit in the IPv4 part of the address");
         mstest::Assert::IsFalse(is_ipv6_address("::ffff:192.0:2.128"), L"A colon in the IPv4 part of the address");
         mstest::Assert::IsFalse(is_ipv6_address("::ffff:1234.0.2.128"), L"Four digits in a group in the IPv4 part of the address (1st group)");
         mstest::Assert::IsFalse(is_ipv6_address("::ffff:192.0.1234.128"), L"Four digits in a group in the IPv4 part of the address (3rd group)");
      }
};
}