/*
   webalizer - a web server log analysis program

   Copyright (c) 2004-2019, Stone Steps Inc. (www.stonesteps.ca)

   See COPYING and Copyright files for additional licensing and copyright information 
   
   ut_ipaddr.cpp
*/
#include "pch.h"

#include "../util_ipaddr.h"

namespace sswtest {
///
/// @brief  Parse proper IPv4 addresses
///
TEST(IPAddressTest, GoodIPv4Addresses)
{
   EXPECT_TRUE(is_ipv4_address("0.0.0.0")) << "0.0.0.0";
   EXPECT_TRUE(is_ipv4_address("12.34.56.78")) << "12.34.56.78";
   EXPECT_TRUE(is_ipv4_address("123.123.123.123")) << "123.123.123.123";
   EXPECT_TRUE(is_ipv4_address("0.123.123.123")) << "0.123.123.123";
   EXPECT_TRUE(is_ipv4_address("123.0.123.123")) << "123.0.123.123";
   EXPECT_TRUE(is_ipv4_address("123.123.0.123")) << "123.123.0.123";
   EXPECT_TRUE(is_ipv4_address("123.123.123.0")) << "123.123.123.0";
   EXPECT_TRUE(is_ipv4_address("127.0.0.1")) << "127.0.0.1";
}

///
/// @brief  Parse malformed IPv4 addresses
///
TEST(IPAddressTest, BadIPv4Addresses)
{
   EXPECT_FALSE(is_ipv4_address("0,0.0.0")) << "A non-dot character (comma)";
   EXPECT_FALSE(is_ipv4_address("0.0.0.0.0")) << "Five groups of digits";
   EXPECT_FALSE(is_ipv4_address("0.0.0.1234")) << "More than three digits in a group (1)";
   EXPECT_FALSE(is_ipv4_address("0.0.1234.0")) << "More than three digits in a group (2)";
   EXPECT_FALSE(is_ipv4_address("0.1234.0.0")) << "More than three digits in a group (3)";
   EXPECT_FALSE(is_ipv4_address("1234.0.0.0")) << "More than three digits in a group (4)";
   EXPECT_FALSE(is_ipv4_address("0.a.0.0")) << "A non-decimal character in a group";
   EXPECT_FALSE(is_ipv4_address("0.1a.0.0")) << "A non-decimal character in a group";
   EXPECT_FALSE(is_ipv4_address("abc")) << "abc";
   EXPECT_FALSE(is_ipv4_address("a.b.c.d")) << "a.b.c.d";
}

///
/// @brief  Parse proper IPv6 addresses
///
TEST(IPAddressTest, GoodIPv6Addresses)
{
   EXPECT_TRUE(is_ipv6_address("2017:0db8:85a3:0000:0000:8a2e:0370:7334")) << "2017:0db8:85a3:0000:0000:8a2e:0370:7334";
   EXPECT_TRUE(is_ipv6_address("2017:0DB8:85A3:0000:0000:8A2E:0370:7334")) << "2017:0DB8:85A3:0000:0000:8A2E:0370:7334";
   EXPECT_TRUE(is_ipv6_address("2017:db8:85a3:0:0:8a2e:370:7334")) << "2017:db8:85a3:0:0:8a2e:370:7334";
   EXPECT_TRUE(is_ipv6_address("2017:db8:85a3::8a2e:370:7334")) << "2017:db8:85a3::8a2e:370:7334";
   EXPECT_TRUE(is_ipv6_address("::")) << "::";
   EXPECT_TRUE(is_ipv6_address("::123")) << "::123";
   EXPECT_TRUE(is_ipv6_address("2017::123")) << "2017::123";
   EXPECT_TRUE(is_ipv6_address("::ffff:c000:0280")) << "::ffff:c000:0280";
   EXPECT_TRUE(is_ipv6_address("::ffff:192.0.2.128")) << "::ffff:192.0.2.128";
   EXPECT_TRUE(is_ipv6_address("::192.0.2.128")) << "::192.0.2.128";
}

///
/// @brief  Parse malformed IPv6 addresses
///
TEST(IPAddressTest, BadIPv6Addresses)
{
   EXPECT_FALSE(is_ipv6_address("2017:00db8:85a3:0000:0000:8a2e:0370:7334")) << "More than four characters in a hex group";
   EXPECT_FALSE(is_ipv6_address("2017:0Xb8:85a3:0000:0000:8a2e:0370:7334")) << "A non-hex character (X) in a group";
   EXPECT_FALSE(is_ipv6_address("2017::2017:0db8:85a3:0000:0000:8a2e:0370:7334")) << "Nine hex groups instead of eight";
   EXPECT_FALSE(is_ipv6_address("2017:0db8::0000:8a2e::7334")) << "Two double-colon sequences";
   EXPECT_FALSE(is_ipv6_address("::ffff:192.0.2.128.1")) << "Five groups of the IPv4 part of the address";
   EXPECT_FALSE(is_ipv6_address("::ffff:192,0.2.128")) << "A non-dot character (comma) in the IPv4 part of the address";
   EXPECT_FALSE(is_ipv6_address("::ffff:192.X.2.128")) << "A non-hex digit in the IPv4 part of the address";
   EXPECT_FALSE(is_ipv6_address("::ffff:192.0:2.128")) << "A colon in the IPv4 part of the address";
   EXPECT_FALSE(is_ipv6_address("::ffff:1234.0.2.128")) << "Four digits in a group in the IPv4 part of the address (1st group)";
   EXPECT_FALSE(is_ipv6_address("::ffff:192.0.1234.128")) << "Four digits in a group in the IPv4 part of the address (3rd group)";
}
}