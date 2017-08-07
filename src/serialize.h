/*
    webalizer - a web server log analysis program

    Copyright (c) 2004-2017, Stone Steps Inc. (www.stonesteps.ca)

    See COPYING and Copyright files for additional licensing and copyright information

    serialize.h
*/
#ifndef SERIALIZE_H
#define SERIALIZE_H

#include "util_string.h"
#include "tstring.h"
#include "tstamp.h"
#include "types.h"

// -----------------------------------------------------------------------
//
// serialized field or record comparison function
//
// -----------------------------------------------------------------------
//
// buf1 and buf2 point to the beginning of two values being compared, as 
// returned by one of the field extractor/pointer functions.
//
typedef int64_t (*s_compare_cb_t)(const void *buf1, const void *buf2);

// -----------------------------------------------------------------------
//
// serialized field pointer function
//
// -----------------------------------------------------------------------
//
// buffer points to the data record containing the requested field. The 
// return value points to the field within the record and datasize is set 
// to reflect the size of the field.
//
typedef const void *(*s_field_cb_t)(const void *buffer, size_t bufsize, size_t& datasize);

// -----------------------------------------------------------------------
//
// serialized multi-part field or record comparison function
//
// -----------------------------------------------------------------------
//
// buf1 and buf2 point to the first part of two multipart values being 
// compared. Zero-based partid indicates which part should be evaluated.
// Function behavior is undefined if partid is greater than the maximum
// valid value. When the last part of the value is evaluated, lastpart
// will be set by the function to true.
//
typedef int64_t (*s_mp_compare_cb_t)(const void *buf1, const void *buf2, u_int partid, bool& lastpart);

// -----------------------------------------------------------------------
//
// serialized multi-part field pointer function
//
// -----------------------------------------------------------------------
//
// buffer points to the data record containing the requested field. The 
// return value points to the specified part of a multipart value. When
// the last part of the value is evaluated, lastpart will be set by the
// function to true.
//
typedef const void *(*s_mp_field_cb_t)(const void *buffer, size_t bufsize, size_t& datasize, u_int partid, bool& lastpart);

// -----------------------------------------------------------------------
//
// serialization
//
// -----------------------------------------------------------------------

inline void *serialize(void *ptr, const u_char bytes[], size_t length)
{
   memcpy(ptr, bytes, length);
   return (u_char*) ptr + length;
}

inline void *serialize(void *ptr, const char chars[], size_t length)
{
   memcpy(ptr, chars, length);
   return &((u_char*)ptr)[length];
}

// serialize strings as a character count followed by string characters
void *serialize(void *ptr, const string_t& value);

// serialize time stamp components one by one
void *serialize(void *ptr, const tstamp_t& tstamp);

template <typename type_t>
inline void *serialize(void *ptr, type_t value)
{
   *((type_t*) ptr) = value;
   return (u_char*) ptr + sizeof(type_t);
}

// serialize bool as u_char because bool is defined differently on many platforms
inline void *serialize(void *ptr, bool value)
{
   *((u_char*) ptr) = (u_char) value;
   return (u_char*) ptr + sizeof(u_char);
}

// -----------------------------------------------------------------------
//
// de-serialization
//
// -----------------------------------------------------------------------

inline const void *deserialize(const void *ptr, u_char bytes[], size_t length)
{
   memcpy(bytes, ptr, length);
   return &((u_char*)ptr)[length];
}

inline const void *deserialize(const void *ptr, char chars[], size_t length)
{
   memcpy(chars, ptr, length);
   return &((u_char*)ptr)[length];
}

// read strings as a character count followed by string characters
const void *deserialize(const void *ptr, string_t& value);

// read time stamp components one at a time
const void *deserialize(const void *ptr, tstamp_t& tstamp);

// read a serialized value whose storage type is the same as the run time type
template <typename type_t>
inline const void *deserialize(const void *ptr, type_t& value)
{
   value = *((type_t*) ptr);
   return (u_char*)ptr + sizeof(type_t);
}

// read a serialized value whose storage type differs from the run time type
template <typename storage_t, typename runtime_t>
const void *deserialize(const void *buf, runtime_t& value)
{
   storage_t stored_value;

   const void *ptr = deserialize(buf, stored_value);
   value = (runtime_t) stored_value;

   return ptr;
}

// read bool as u_char because bool is defined differently on many platforms
inline const void *deserialize(const void *ptr, bool& value)
{
   value = *((u_char*) ptr) ? true : false;
   return (u_char*) ptr + sizeof(u_char);
}

// -----------------------------------------------------------------------
//
// size-of functions (value)
//
// -----------------------------------------------------------------------
template <typename type_t>
inline size_t s_size_of(type_t value)
{
   return sizeof(type_t);
}

// make sure bool stored consistently, as sizeof(bool) varies
inline size_t s_size_of(bool value)
{
   return sizeof(u_char);
}

// define a non-template function so we can use a reference
inline size_t s_size_of(const string_t& value)
{
   return sizeof(u_int) + value.length();
}

size_t s_size_of(const tstamp_t& tstamp);

// -----------------------------------------------------------------------
//
// size-of functions (buffer)
//
// -----------------------------------------------------------------------
template <typename type_t>
inline size_t s_size_of(const void *ptr)
{
   return sizeof(type_t);
}

template <>
inline size_t s_size_of<bool>(const void *ptr)
{
   return sizeof(u_char);
}

template <>
inline size_t s_size_of<string_t>(const void *ptr)
{
   return sizeof(u_int) + (*(u_int*)ptr);
}

template <> 
size_t s_size_of<tstamp_t>(const void *ptr);

// -----------------------------------------------------------------------
//
// size-of functions that don't depend on instance or storage data
//
// -----------------------------------------------------------------------
template <typename type_t>
inline size_t s_size_of(void)
{
   return sizeof(type_t);
}

template <>
inline size_t s_size_of<bool>(void)
{
   return sizeof(u_char);
}

// -----------------------------------------------------------------------
//
// skip functions
//
// -----------------------------------------------------------------------

template <typename type_t>
inline void *s_skip_field(const void *ptr)
{
   return (u_char*) ptr + sizeof(type_t);
}

template <>
inline void *s_skip_field<bool>(const void *ptr)
{
   return (u_char*) ptr + sizeof(u_char);
}

template <>
inline void *s_skip_field<string_t>(const void *ptr)
{
   return (u_char*) ptr + sizeof(u_int) + *(u_int*)ptr;
}

template <>
inline void *s_skip_field<tstamp_t>(const void *ptr)
{
   return (u_char*) ptr + s_size_of<tstamp_t>(ptr);
}

// -----------------------------------------------------------------------
//
// comparison functions (buffer)
//
// -----------------------------------------------------------------------
template <typename type_t>
inline int64_t s_compare(const void *buf1, const void *buf2)
{
   return *(type_t*) buf1 == *(type_t*) buf2 ? 0 : *(type_t*) buf1 > *(type_t*) buf2 ? 1 : -1;
}

template <>
inline int64_t s_compare<string_t>(const void *buf1, const void *buf2)
{
   const char *cp1, *cp2;
   u_int slen1, slen2;

   cp1 = (const char*) deserialize(buf1, slen1);
   cp2 = (const char*) deserialize(buf2, slen2);

   return strncmp_ex(cp1, slen1, cp2, slen2);
}

#endif // SERIALIZE_H
