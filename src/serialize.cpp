/*
   webalizer - a web server log analysis program

   Copyright (c) 2004-2019, Stone Steps Inc. (www.stonesteps.ca)

   See COPYING and Copyright files for additional licensing and copyright information 
   
   serialize.cpp
*/
#include "pch.h"

#include "serialize.h"

#include <stdexcept>

serializer_t::serializer_t(const void *buffer, size_t bufsize) :
      buffer(buffer),
      bufsize(bufsize)
{
}

size_t serializer_t::data_size(const void *ptr) const
{
   std::ptrdiff_t ptrdiff = (const u_char*) ptr - (const u_char*) buffer;

   if(ptrdiff < 0)
      throw std::invalid_argument("Bad current buffer position (before buffer start)");

   if((size_t) ptrdiff > bufsize)
      throw std::invalid_argument("Bad current buffer position (past buffer end)");

   return (size_t) ptrdiff;
}

size_t serializer_t::buffer_space(const void *ptr) const
{
   return bufsize - data_size(ptr);
}

///
/// This method is specialized for `bool` and declared as deleted for `string_t` and
/// `tstamp_t` because these objects require variable amount of storage and their size
/// cannot be determined without a specific instance.
///
template <typename type_t>
size_t serializer_t::s_size_of(void)
{
   return sizeof(type_t);
}

template <>
size_t serializer_t::s_size_of<bool>(void)
{
   return sizeof(u_char);
}

template <typename type_t>
size_t serializer_t::s_size_of(type_t value)
{
   return sizeof(type_t);
}

size_t serializer_t::s_size_of(const string_t& value)
{
   return s_size_of<u_int>() + value.length();
}

size_t serializer_t::s_size_of(const tstamp_t& tstamp)
{
   size_t datasize = s_size_of<bool>();         // null
   
   if(tstamp.null)
      return datasize;
       
   datasize += s_size_of<bool>()       +        // utc
            s_size_of<u_short>()       +        // year
            s_size_of<u_char>() * 5;            // month, day, hour, min, sec

   return tstamp.utc ? datasize : 
               datasize + s_size_of<short>();  // offset
}

size_t serializer_t::s_size_of(bool value)
{
   return s_size_of<bool>();
}

template <> size_t serializer_t::s_size_of(const char (&chars)[2])
{
   return sizeof(char[2]);
}

template <typename type_t>
size_t serializer_t::s_size_of(const void *ptr) const
{
   // s_skip_field guarantees that both pointers have acceptable values, so we can just subtract
   return (const u_char*) s_skip_field<type_t>(ptr) - (const u_char*) ptr;
}

void *serializer_t::serialize(void *ptr, const char chars[], size_t length) const
{
   if(buffer_space(ptr) < length)
      throw std::invalid_argument(string_t::_format("Buffer is too small (%zd vs. %zd)", length, buffer_space(ptr)));

   memcpy(ptr, chars, length);

   return (u_char*) ptr + length;
}

const void *serializer_t::s_skip_field(const void *ptr, size_t length) const
{
   if(buffer_space(ptr) < length)
      throw std::invalid_argument(string_t::_format("Truncated data (%zd vs. %zd)", length, buffer_space(ptr)));

   return (const u_char*) ptr + length;
}

template <>
const void *serializer_t::s_skip_field<string_t>(const void *ptr) const
{
   u_int slen;

   ptr = deserialize(ptr, slen);

   return s_skip_field(ptr, (size_t) slen);
}

template <>
const void *serializer_t::s_skip_field<tstamp_t>(const void *ptr) const
{
   bool null, utc;

   ptr = deserialize(ptr, null);

   if(null)
      return ptr;

   ptr = deserialize(ptr, utc);

   ptr = s_skip_field<u_short>(ptr);
   ptr = s_skip_field<u_char>(ptr);
   ptr = s_skip_field<u_char>(ptr);

   ptr = s_skip_field<u_char>(ptr);
   ptr = s_skip_field<u_char>(ptr);
   ptr = s_skip_field<u_char>(ptr);

   if(!utc)
      ptr = s_skip_field<short>(ptr);

   return ptr;
}

template <>
const void *serializer_t::s_skip_field<bool>(const void *ptr) const
{
   return (u_char*) ptr + sizeof(u_char);
}

template <typename type_t>
const void *serializer_t::s_skip_field(const void *ptr) const
{
   if(buffer_space(ptr) < s_size_of<type_t>())
      throw std::invalid_argument(string_t::_format("Truncated data (%s)", typeid(type_t).name()));

   return (u_char*) ptr + s_size_of<type_t>();
}

template <>
void *serializer_t::serialize(void *ptr, const char (&chars)[2]) const
{
   return serialize(ptr, chars, 2);
}

template <typename type_t>
void *serializer_t::serialize(void *ptr, type_t value) const
{
   if(buffer_space(ptr) < s_size_of(value))
      throw std::invalid_argument(string_t::_format("Buffer is too small (%s)", typeid(value).name()));

   *((type_t*) ptr) = value;

   return (u_char*) ptr + s_size_of(value);
}

///
/// Unlike the deserialization counterpart for this method, the caller could just cast
/// the value to the storage type, but the code would be more consistent if the same
/// template function pattern was used. For example, a run time 64-bit integer can be
/// serialized and deserialized as follows:
///
/// ```
/// uint64_t ui64 = 123;
/// unsigned int ui = 0;
/// serializer_t sr(buffer, bufsize);
/// sr.serialize<unsigned int>(buffer, ui64);
/// sr.deserialize<unsigned int>(buffer, ui);
/// ```
///
template <typename storage_t, typename runtime_t>
void *serializer_t::serialize(void *ptr, runtime_t value) const
{
   return serialize(ptr, (storage_t) value);
}

void *serializer_t::serialize(void *ptr, const string_t& value) const
{
   ptr = serialize<u_int, size_t>(ptr, value.length());

   if(value.length())
      ptr = serialize(ptr, value.c_str(), value.length());

   return ptr;
}

void *serializer_t::serialize(void *ptr, const tstamp_t& tstamp) const
{
   //
   // Serialize null and UTC flags first, so we can figure out the size 
   // of the timestamp in the buffer after reading one or two bytes.
   //
   ptr = serialize(ptr, tstamp.null);

   if(tstamp.null)
      return ptr;

   ptr = serialize(ptr, tstamp.utc);

   // now serialize all fixed-size fields
   ptr = serialize<u_short>(ptr, tstamp.year);
   ptr = serialize<u_char>(ptr, tstamp.month);
   ptr = serialize<u_char>(ptr, tstamp.day);

   ptr = serialize<u_char>(ptr, tstamp.hour);
   ptr = serialize<u_char>(ptr, tstamp.min);
   ptr = serialize<u_char>(ptr, tstamp.sec);

   // finally, serialize the UTC offset for local times
   if(!tstamp.utc)
      ptr = serialize<short>(ptr, tstamp.offset);

   return ptr;
}

void *serializer_t::serialize(void *ptr, bool value) const
{
   return serialize<u_char, bool>(ptr, value);
}

const void *serializer_t::deserialize(const void *ptr, char chars[], size_t length) const
{
   if(buffer_space(ptr) < length)
      throw std::invalid_argument(string_t::_format("Truncated data (%zd vs. %zd)", length, buffer_space(ptr)));

   memcpy(chars, ptr, length);

   return (const u_char*) ptr + length;
}

template <>
const void *serializer_t::deserialize(const void *ptr, char (&chars)[2]) const
{
   return deserialize(ptr, chars, 2);
}

template <typename type_t>
const void *serializer_t::deserialize(const void *ptr, type_t& value) const
{
   if(buffer_space(ptr) < s_size_of(value))
      throw std::invalid_argument(string_t::_format("Truncated data (%s)", typeid(value).name()));

   value = *((type_t*) ptr);

   return (const u_char*)ptr + s_size_of(value);
}

template <typename storage_t, typename runtime_t>
const void *serializer_t::deserialize(const void *buf, runtime_t& value) const
{
   storage_t stored_value;

   const void *ptr = deserialize(buf, stored_value);

   value = (runtime_t) stored_value;

   return ptr;
}

const void *serializer_t::deserialize(const void *ptr, string_t& value) const
{
   size_t slen;

   // call explicit specialization because size_t is unsigned int on Win32
   ptr = deserialize<u_int, size_t>(ptr, slen);

   if(slen) {
      string_t::char_buffer_t cb(slen + 1);

      ptr = deserialize(ptr, cb.get_buffer(), slen);
      cb[slen] = 0;

      value.attach(std::move(cb), slen);
   }
   else
      value.reset();

   return ptr;
}

const void *serializer_t::deserialize(const void *ptr, tstamp_t& tstamp) const
{
   bool null, utc;
   u_int year, month, day, hour, min, sec;
   int offset;

   ptr = deserialize(ptr, null);

   if(null) {
      tstamp.reset();
      return ptr;
   }

   ptr = deserialize(ptr, utc);

   ptr = deserialize<u_short>(ptr, year);
   ptr = deserialize<u_char>(ptr, month);
   ptr = deserialize<u_char>(ptr, day);

   ptr = deserialize<u_char>(ptr, hour);
   ptr = deserialize<u_char>(ptr, min);
   ptr = deserialize<u_char>(ptr, sec);

   if(utc)
      tstamp.reset(year, month, day, hour, min, sec);
   else {
      ptr = deserialize<short>(ptr, offset);
      tstamp.reset(year, month, day, hour, min, sec, offset);
   }

   return ptr;
}

const void *serializer_t::deserialize(const void *ptr, bool& value) const
{
   return deserialize<u_char, bool>(ptr, value);
}

//
// Instantiate template functinos defined in this file
//

// TODO: This is temporary, to bring in nodetype_t
#include "hashtab.h"

template size_t serializer_t::s_size_of<u_short>(const void *ptr) const;
template size_t serializer_t::s_size_of<u_int>(const void *ptr) const;
template size_t serializer_t::s_size_of<uint64_t>(const void *ptr) const;
template size_t serializer_t::s_size_of<double>(const void *ptr) const;
template size_t serializer_t::s_size_of<string_t>(const void *ptr) const;
template size_t serializer_t::s_size_of<tstamp_t>(const void *ptr) const;

template size_t serializer_t::s_size_of<u_short>(void);
template size_t serializer_t::s_size_of<u_int>(void);
template size_t serializer_t::s_size_of<uint64_t>(void);
template size_t serializer_t::s_size_of<double>(void);

template size_t serializer_t::s_size_of(u_short value);
template size_t serializer_t::s_size_of(u_int value);
template size_t serializer_t::s_size_of(uint64_t value);
template size_t serializer_t::s_size_of(double value);

template const void *serializer_t::s_skip_field<char(&)[2]>(const void *ptr) const;
template const void *serializer_t::s_skip_field<u_char>(const void *ptr) const;
template const void *serializer_t::s_skip_field<u_short>(const void *ptr) const;
template const void *serializer_t::s_skip_field<u_int>(const void *ptr) const;
template const void *serializer_t::s_skip_field<uint64_t>(const void *ptr) const;

template void *serializer_t::serialize<u_char, nodetype_t>(void *ptr, nodetype_t value) const;
template void *serializer_t::serialize<short, int>(void *ptr, int value) const;
template void *serializer_t::serialize<u_short, u_int>(void *ptr, u_int value) const;
template void *serializer_t::serialize<u_int, uint64_t>(void *ptr, uint64_t value) const;

template void *serializer_t::serialize(void *ptr, const char (&chars)[2]) const;
template void *serializer_t::serialize(void *ptr, u_char value) const;
template void *serializer_t::serialize(void *ptr, u_short value) const;
template void *serializer_t::serialize(void *ptr, u_int value) const;
template void *serializer_t::serialize(void *ptr, uint64_t value) const;
template void *serializer_t::serialize(void *ptr, double value) const;

template const void *serializer_t::deserialize<u_char, nodetype_t>(const void *ptr, nodetype_t& value) const;
template const void *serializer_t::deserialize<short, int>(const void *ptr, int& value) const;
template const void *serializer_t::deserialize<u_short, u_int>(const void *ptr, u_int& value) const;
template const void *serializer_t::deserialize<u_int, uint64_t>(const void *ptr, uint64_t& value) const;

template const void *serializer_t::deserialize(const void *ptr, char (&chars)[2]) const;
template const void *serializer_t::deserialize(const void *ptr, u_char& value) const;
template const void *serializer_t::deserialize(const void *ptr, u_short& value) const;
template const void *serializer_t::deserialize(const void *ptr, u_int& value) const;
template const void *serializer_t::deserialize(const void *ptr, uint64_t& value) const;
template const void *serializer_t::deserialize(const void *ptr, double& value) const;

