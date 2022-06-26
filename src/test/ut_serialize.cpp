/*
   webalizer - a web server log analysis program

   Copyright (c) 2004-2022, Stone Steps Inc. (www.stonesteps.ca)

   See COPYING and Copyright files for additional licensing and copyright information 

   ut_serialize.cpp
*/
#include "pch.h"

#include "../serialize.h"
#include "../tstring.h"
#include "../tstamp.h"
#include "..//types.h"

#include <ctime>

///
/// @brief  Tests sizes of fields stored in the buffer.
///
TEST(Serialization, FieldSizeTest)
{
   // without data values
   EXPECT_EQ(sizeof(u_char), serializer_t::s_size_of<bool>());
   EXPECT_EQ(sizeof(u_short), serializer_t::s_size_of<u_short>());
   EXPECT_EQ(sizeof(u_int), serializer_t::s_size_of<u_int>());
   EXPECT_EQ(sizeof(uint64_t), serializer_t::s_size_of<uint64_t>());

   // with data values
   EXPECT_EQ(sizeof(u_char), serializer_t::s_size_of(true));
   EXPECT_EQ(sizeof(u_short), serializer_t::s_size_of((u_short) 123));
   EXPECT_EQ(sizeof(u_int), serializer_t::s_size_of(123u));
   EXPECT_EQ(sizeof(uint64_t), serializer_t::s_size_of((uint64_t) 123));

   // non-empty string
   string_t str("ABCDEF");
   EXPECT_EQ(sizeof(u_int) + str.length(), serializer_t::s_size_of(str));

   // empty string
   string_t empty;
   EXPECT_EQ(sizeof(u_int) + empty.length(), serializer_t::s_size_of(empty));

   // UTC time stamp
   tstamp_t utc_ts(time(nullptr));
   EXPECT_EQ(sizeof(u_char) +       // null
      sizeof(u_char) +              // utc
      sizeof(u_short) +             // year
      sizeof(u_char) * 5,           // month, day, hour, minute, second
      serializer_t::s_size_of(utc_ts));

   // local time stamp
   tstamp_t lcl_ts(time(nullptr), -300);
   EXPECT_EQ(sizeof(u_char) +       // null
      sizeof(u_char) +              // utc
      sizeof(u_short) +             // year
      sizeof(u_char) * 5 +          // month, day, hour, minute, second
      sizeof(short),                // local time offset
      serializer_t::s_size_of(lcl_ts));

   // null time stamp
   tstamp_t null_ts;
   EXPECT_EQ(sizeof(u_char), serializer_t::s_size_of(null_ts));
}

///
/// @brief  Tests fields written to and read from the buffer.
///
TEST(Serialization, FieldWriteReadTest)
{
   // large enough to detect overflows for data less than 64 bytes
   string_t::char_buffer_t buffer(128);

   // set to 0x5A to detect overflows
   memset(buffer.get_buffer(), 0x5A, buffer.memsize());

   string_t str("ABCDEF");
   string_t empty;

   tstamp_t utc_ts(time(nullptr));
   tstamp_t lcl_ts(time(nullptr), -300);
   tstamp_t null_ts;

   u_short ushv = 12345;
   uint64_t ui64v = 1234567890ull; // convertable to u_int
   char ccode[2] = {'c', 'a'};

   serializer_t sr(buffer, 64);
   void *wptr = buffer.get_buffer();

   wptr = sr.serialize(wptr, ushv);
   wptr = sr.serialize(wptr, null_ts);
   wptr = sr.serialize(wptr, str);
   wptr = sr.serialize(wptr, utc_ts);
   wptr = sr.serialize(wptr, ui64v);
   wptr = sr.serialize(wptr, empty);
   wptr = sr.serialize(wptr, true);
   wptr = sr.serialize(wptr, ccode);
   wptr = sr.serialize(wptr, lcl_ts);
   wptr = sr.serialize<u_int>(wptr, ui64v);

   ASSERT_LE(sr.data_size(wptr), buffer.memsize()) << "Write pointer must be within the buffer when we finish serialization";
   ASSERT_EQ('\x5A', *(char*) wptr);

   const void *rptr = buffer.get_buffer();

   string_t v_str, v_empty;
   const char *v_sptr = nullptr;
   size_t v_slen = 0;
   tstamp_t v_utc_ts, v_lcl_ts, v_null_ts;
   u_short v_shv;
   uint64_t v_ui64v, v_ui64v_i;
   bool v_bool = false;
   char v_ccode[3] = {'\xAA', '\xAA', '\xAA'};

   rptr = sr.deserialize(rptr, v_shv);
   ASSERT_EQ(ushv, v_shv);

   rptr = sr.deserialize(rptr, v_null_ts);
   ASSERT_EQ(null_ts, v_null_ts);

   // do not advance the read pointer, so we can read the string again
   sr.deserialize(rptr, v_sptr, v_slen);
   ASSERT_STREQ(str.c_str(), string_t::hold(v_sptr, v_slen));

   rptr = sr.deserialize(rptr, v_str);
   ASSERT_STREQ(str.c_str(), v_str.c_str());

   rptr = sr.deserialize(rptr, v_utc_ts);
   ASSERT_EQ(utc_ts, v_utc_ts);

   rptr = sr.deserialize(rptr, v_ui64v);
   ASSERT_EQ(ui64v, v_ui64v);

   // do not advance the read pointer, so we can read the string again
   sr.deserialize(rptr, v_sptr, v_slen);
   ASSERT_EQ(0, v_slen);

   rptr = sr.deserialize(rptr, v_empty);
   ASSERT_TRUE(v_empty.isempty());

   rptr = sr.deserialize(rptr, v_bool);
   ASSERT_EQ(true, v_bool);

   // read into a larger array, so we can check the character following those read from the buffer
   rptr = sr.deserialize(rptr, reinterpret_cast<char(&)[2]>(v_ccode));
   ASSERT_EQ(ccode[0], v_ccode[0]);
   ASSERT_EQ(ccode[1], v_ccode[1]);
   ASSERT_EQ('\xAA', v_ccode[2]);

   rptr = sr.deserialize(rptr, v_lcl_ts);
   ASSERT_EQ(lcl_ts, v_lcl_ts);

   rptr = sr.deserialize<u_int>(rptr, v_ui64v_i);
   ASSERT_EQ(ui64v, v_ui64v_i);

   ASSERT_EQ(wptr, rptr) << "Serialization write and read pointers should compare equal for same data";

   ASSERT_EQ('\x5A', *(char*) rptr) << "Read pointer should point to the first unused byte in the buffer";
}

///
/// @brief  Tests skipping fields when reading.
///
TEST(Serialization, FieldSkipTest)
{
   // large enough to detect overflows for data less than 64 bytes
   string_t::char_buffer_t buffer(128);

   // set to 0x5A to detect overflows
   memset(buffer.get_buffer(), 0x5A, buffer.memsize());

   string_t str("ABCDEF");
   string_t empty;

   tstamp_t utc_ts(time(nullptr));
   tstamp_t lcl_ts(time(nullptr), -300);
   tstamp_t null_ts;

   u_short ushv = 12345;
   uint64_t ui64v = 1234567890ull; // convertable to u_int

   char ccode[2] = {'c', 'a'};

   serializer_t sr(buffer, 64);
   void *wptr = buffer.get_buffer();

   wptr = sr.serialize(wptr, ushv);
   wptr = sr.serialize(wptr, null_ts);
   wptr = sr.serialize(wptr, str);
   wptr = sr.serialize(wptr, utc_ts);
   wptr = sr.serialize(wptr, ui64v);
   wptr = sr.serialize(wptr, empty);
   wptr = sr.serialize(wptr, true);
   wptr = sr.serialize(wptr, ccode);
   wptr = sr.serialize(wptr, lcl_ts);
   wptr = sr.serialize<u_int>(wptr, ui64v);

   const void *rptr = buffer.get_buffer();

   rptr = sr.s_skip_field<u_short>(rptr);
   rptr = sr.s_skip_field<tstamp_t>(rptr);
   rptr = sr.s_skip_field<string_t>(rptr);
   rptr = sr.s_skip_field<tstamp_t>(rptr);
   rptr = sr.s_skip_field<uint64_t>(rptr);
   rptr = sr.s_skip_field<string_t>(rptr);
   rptr = sr.s_skip_field<bool>(rptr);
   rptr = sr.s_skip_field<char(&)[2]>(rptr);
   rptr = sr.s_skip_field<tstamp_t>(rptr);
   rptr = sr.s_skip_field<u_int>(rptr);

   ASSERT_EQ(wptr, rptr) << "Serialization write and read pointers should compare equal for same data";

   ASSERT_EQ('\x5A', *(char*) rptr) << "Read pointer should point to the first unused byte in the buffer";
}

TEST(Serialization, BufferBoundstest)
{
   // large enough for a 10-byte serialized string and 10-byte padding on both ends
   string_t::char_buffer_t buffer(30);

   // set to 0x5A to detect overflows
   memset(buffer.get_buffer(), 0x5A, buffer.memsize());

   string_t str("ABCDEF");

   // 4-byte length. plus 6 bytes for string characters
   serializer_t sr(buffer.get_buffer() + 10, 10);

   // serialize 10 bytes of data (will not read it as a string) starting from a 10-byte offset
   ASSERT_NO_THROW(sr.serialize(buffer.get_buffer() + 10, str));

   //
   // Probe one byte at a time inside and outside the buffer.
   //

   // we can read the first byte of the buffer
   ASSERT_NO_THROW(sr.s_skip_field<u_char>(buffer.get_buffer() + 10));
   ASSERT_NE('\x5A', *(buffer.get_buffer() + 10));

   // cannot read the byte right before the buffer start byte
   ASSERT_THROW(sr.s_skip_field<u_char>(buffer.get_buffer() + 9), std::invalid_argument);
   ASSERT_EQ('\x5A', *(buffer.get_buffer() + 9));

   // cannot read the byte a few bytes before the buffer start byte
   ASSERT_THROW(sr.s_skip_field<u_char>(buffer.get_buffer()), std::invalid_argument);
   ASSERT_EQ('\x5A', *buffer.get_buffer());

   // we can read the last byte of the buffer
   ASSERT_NO_THROW(sr.s_skip_field<u_char>(buffer.get_buffer() + 19));
   ASSERT_NE('\x5A', *(buffer.get_buffer() + 19));

   // cannot read the byte right past the buffer end byte
   ASSERT_THROW(sr.s_skip_field<u_char>(buffer.get_buffer() + 20), std::invalid_argument);
   ASSERT_EQ('\x5A', *(buffer.get_buffer() + 20));

   // cannot read the byte a few bytes past the buffer end byte
   ASSERT_THROW(sr.s_skip_field<u_char>(buffer.get_buffer() + 29), std::invalid_argument);
   ASSERT_EQ('\x5A', *(buffer.get_buffer() + 29));

   //
   // Check correct data size returned when pointing inside and outside the buffer.
   //

   // pointing at the first byte of the buffer yields zero-length data
   ASSERT_NO_THROW(sr.data_size(buffer.get_buffer() + 10));
   ASSERT_EQ(0, sr.data_size(buffer.get_buffer() + 10));

   // pointing one byte past the buffer end should return data size as 10 bytes
   ASSERT_NO_THROW(sr.data_size(buffer.get_buffer() + 20));
   ASSERT_EQ(10, sr.data_size(buffer.get_buffer() + 20));

   // cannot get data length when pointing at the byte right before the buffer
   ASSERT_THROW(sr.data_size(buffer.get_buffer() + 9), std::invalid_argument);

   // cannot get data length when pointing to a few bytes before the buffer
   ASSERT_THROW(sr.data_size(buffer.get_buffer()), std::invalid_argument);

   // cannot get data length when pointing to two bytes past the buffer
   ASSERT_THROW(sr.data_size(buffer.get_buffer() + 21), std::invalid_argument);

   // cannot get data length when pointing to a few bytes past the buffer
   ASSERT_THROW(sr.data_size(buffer.get_buffer() + 29), std::invalid_argument);

   //
   // Attempt to read a few bytes overlapping buffer boundaries (value is irrelevant).
   //

   u_short ushv = 0;
   u_int uiv = 0;

   // skip or read 2 bytes right before the end of the buffer
   ASSERT_NO_THROW(sr.s_skip_field<u_short>(buffer.get_buffer() + 20 - serializer_t::s_size_of<u_short>()));
   ASSERT_NO_THROW(sr.deserialize(buffer.get_buffer() + 20 - serializer_t::s_size_of<u_short>(), ushv));

   // we should be able to get the size of a u_short field at the very end of the buffer
   ASSERT_EQ(serializer_t::s_size_of<u_short>(), sr.s_size_of<u_short>(buffer.get_buffer() + 20 - serializer_t::s_size_of<u_short>()));

   // the second byte of a 2-byte skip/read is past the end of the buffer
   ASSERT_THROW(sr.s_skip_field<u_short>(buffer.get_buffer() + 21 - serializer_t::s_size_of<u_short>()), std::invalid_argument);
   ASSERT_THROW(sr.deserialize(buffer.get_buffer() + 21 - serializer_t::s_size_of<u_short>(), ushv), std::invalid_argument);

   // cannot get a size of a field that would extend beyond the end of the buffer
   ASSERT_THROW(sr.s_size_of<u_short>(buffer.get_buffer() + 21 - serializer_t::s_size_of<u_short>()), std::invalid_argument);

   // read storage a 2-byte u_short value at the end of the buffer and return as a 4-byte u_int value
   ASSERT_NO_THROW(sr.deserialize<u_short>(buffer.get_buffer() + 20 - serializer_t::s_size_of<u_short>(), uiv));

   // simulate a bad string in the buffer by writing a bad 7-character length (one past buffer size)
   sr.serialize(buffer.get_buffer() + 10, 7u);

   // attempt to read a bad string from the buffer
   ASSERT_THROW(sr.deserialize(buffer.get_buffer() + 10, str), std::invalid_argument);

   // content of the existing string should not change
   ASSERT_STREQ("ABCDEF", str.c_str());

   // cannot get the size of a string that would extend beyond the end of the buffer by one byte
   ASSERT_THROW(sr.s_size_of<string_t>(buffer.get_buffer() + 10), std::invalid_argument);

   char ccode[2] = {};

   // read a bogus 2-character country code at the very end of the buffer (last two characters of the string)
   ASSERT_NO_THROW(sr.deserialize(buffer.get_buffer() + 20 - sizeof(ccode), ccode));
   ASSERT_EQ('E', ccode[0]);
   ASSERT_EQ('F', ccode[1]);

   // attempt to read a 2-character country code with one byte outside the buffer
   ASSERT_THROW(sr.deserialize(buffer.get_buffer() + 21 - sizeof(ccode), ccode), std::invalid_argument);

   //
   // Attempt to write a few bytes overlapping buffer boundaries.
   //

   string_t bad_str("1234567");

   // attempt to write a string that is one byte longer than the buffer size
   ASSERT_THROW(sr.serialize(buffer.get_buffer() + 10, bad_str), std::invalid_argument);

   tstamp_t utc_ts(time(nullptr));
   tstamp_t lcl_ts(time(nullptr), -300);

   // a UTC time stamp is 9 bytes long and will fit in a 10-byte buffer
   ASSERT_NO_THROW(sr.serialize(buffer.get_buffer() + 10, utc_ts));

   // cpmputed and actual serialized time stamp size should be the same
   ASSERT_EQ(serializer_t::s_size_of(utc_ts), sr.s_size_of<tstamp_t>(buffer.get_buffer() + 10));

   // a local time stamp is 11 bytes long and will not fit in the 10-byte buffer
   ASSERT_THROW(sr.serialize(buffer.get_buffer() + 10, lcl_ts), std::invalid_argument);
}
