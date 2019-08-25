/*
   webalizer - a web server log analysis program

   Copyright (c) 2004-2019, Stone Steps Inc. (www.stonesteps.ca)

   See COPYING and Copyright files for additional licensing and copyright information 
   
   ut_string.cpp
*/
#include "pchtest.h"

#include "../tstring.h"

namespace sswtest {

///
/// @brief  Tests constructing a read-only string holding a string literal.
///
TEST(StringConstruct, HoldString)
{
   const char *slit = "ABC";

   // construct a read-only string
   string_t str1(string_t::hold(slit));

   EXPECT_EQ(slit, str1.c_str()) << "A read-only string should contain a character pointer it was constructed from";

   // move to a const string
   const string_t str2(std::move(str1));

   EXPECT_EQ(slit, str2.c_str()) << "A move-constructed read-only string should contain a character pointer the original string was constructed from";

   EXPECT_THROW(string_t::hold(slit, 2), std::runtime_error) << "A read-only string must have a null terminator at the specified length";

   EXPECT_NO_THROW(string_t::hold(slit, 3)) << "A read-only string may be constructed with its length specified explicitly";
}

///
/// @brief  Tests constructing a default string.
///
TEST(StringConstruct, DefaultConstruct)
{
   EXPECT_STREQ("", string_t().c_str()) << "A default-constructed string is equal to an empty string";
}

///
/// @brief  Tests constructing a string from a C-string.
///
TEST(StringConstruct, FromCString)
{
   EXPECT_STREQ("ABC", string_t("ABC").c_str()) << "A string constructed from a C-string should be equal to that string";
}

///
/// @brief  Tests constructing a string from a sub-string.
///
TEST(StringConstruct, FromSubString)
{
   EXPECT_STREQ("XYZ", string_t("XYZ123", 3).c_str()) << "A string constructed from a substring should compare equal to that substring";
}

///
/// @brief  Test constructing a string by moving the contents of another string.
///
TEST(StringConstruct, MoveConstruct)
{
   string_t str5("ABC123");
   string_t str6(std::move(str5));

   EXPECT_STREQ("", str5.c_str()) << "Source string should be empty after a move";
   EXPECT_STREQ("ABC123", str6.c_str()) << "Target of a move should compare equal to the original string";
}

///
/// @brief  Tests constructing a string from a character buffer and a dynamically-allocated 
///         block of memory.
///
TEST(StringConstruct, AttachString)
{
   // allocate a block of memory large enough to hold 7 characters
   char *mb = string_t::char_buffer_t::alloc(10);
   strcpy(mb, "123ABC");

   // attach the memory block to the buffer
   string_t::char_buffer_t cb(mb, 10);
   const char *cbptr = cb;

   EXPECT_EQ(cb, mb) << "Attached memory block should be the same within the buffer";

   // move buffer memory into the string and truncate it to 3 characters
   string_t str7(std::move(cb), 3);

   EXPECT_STREQ("123", str7.c_str()) << "Attached string should be truncated to three characters";
   EXPECT_EQ(cbptr, str7.c_str()) << "Attached string pointer should be equal to the original string pointer";

   // add characters to exceed the 10-character buffer size, so a new buffer is allocated
   EXPECT_NO_THROW(str7.append("4567890ABCDEF"));

   EXPECT_STREQ("1234567890ABCDEF", str7.c_str()) << "Appended string should be combined with the initial string";
}

///
/// @brief  Tests constrcting a read-only string from a const character buffer
///         and a string literal.
///
TEST(StringConstruct, ReadOnlyString)
{
   const char *slit = "ABC456";

   // create a const holder buffer
   string_t::const_char_buffer_t ccb(slit, 7, true);
   const char *ccbptr = ccb;

   EXPECT_EQ(ccb, slit) << "Attached string literal should be the same within the buffer";

   // a string literal cannot be truncated
   string_t str8(std::move(ccb), 6);

   EXPECT_STREQ("ABC456", str8.c_str()) << "Attached read-only string should compare equal to the same string literal";
   EXPECT_EQ(ccbptr, str8.c_str()) << "Attached read-only string pointer should be equal to the original string pointer";

   EXPECT_THROW(str8.append("X"), std::runtime_error) << "A read-only string cannot be modified";
}

///
/// @brief  Tests the size of the allocated string buffer.
///
TEST(StringConstruct, StringBufferSize)
{
   // string buffer should be allocated in multiples of 4 (plus 1 for the null character)
   EXPECT_EQ(3, string_t("1").capacity());
   EXPECT_EQ(3, string_t("12").capacity());
   EXPECT_EQ(3, string_t("123").capacity());
   EXPECT_EQ(7, string_t("1234").capacity());
   EXPECT_EQ(7, string_t("12345").capacity());
   EXPECT_EQ(7, string_t("123456").capacity());
   EXPECT_EQ(7, string_t("1234567").capacity());
   EXPECT_EQ(11, string_t("12345678").capacity());
   EXPECT_EQ(31, string_t("1234567890123456789012345678901").capacity());
   EXPECT_EQ(35, string_t("12345678901234567890123456789012").capacity());
}

///
/// @brief  Tests a copy-constructed string.
///
TEST(StringConstruct, StringCopy)
{
   string_t src("12345678901234567890123456789012");

   EXPECT_EQ(35, src.capacity());

   src = "ABC";

   EXPECT_EQ(35, src.capacity()) << "Assigning a smaller string should not change string capacity";

   EXPECT_EQ(3, string_t(src).capacity()) << "Buffer capacity is not copied for copy-constructed strings";
}

///
/// @brief  Tests a string copy-constructed from a read-only string.
///
TEST(StringConstruct, ReadOnlyStringCopy)
{
   const string_t& ro_src = string_t::hold("1234567");

   EXPECT_EQ(0, ro_src.capacity()) << "A read-only string has no capacity";

   EXPECT_EQ(7, string_t(ro_src).capacity()) << "A copy of a read-only string maintains its own buffer";
}

}
