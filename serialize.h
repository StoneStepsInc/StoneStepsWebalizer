/*
    webalizer - a web server log analysis program

    Copyright (c) 2004-2013, Stone Steps Inc. (www.stonesteps.ca)

    See COPYING and Copyright files for additional licensing and copyright information

    serialize.h
*/
#ifndef __SERIALIZE_H
#define __SERIALIZE_H

#include "tstring.h"
#include "types.h"
#include "util.h"

// -----------------------------------------------------------------------
//
// serialized field or record comparison function
//
// -----------------------------------------------------------------------
//
// buf1 and buf2 point to the beginning of two values being compared, as 
// returned by one of the field extractor/pointer functions.
//
typedef int (*s_compare_cb_t)(const void *buf1, const void *buf2);

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
typedef const void *(*s_field_cb_t)(const void *buffer, u_int bufsize, u_int& datasize);

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
typedef int (*s_mp_compare_cb_t)(const void *buf1, const void *buf2, u_int partid, bool& lastpart);

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
typedef const void *(*s_mp_field_cb_t)(const void *buffer, u_int bufsize, u_int& datasize, u_int partid, bool& lastpart);

// -----------------------------------------------------------------------
//
// serialization
//
// -----------------------------------------------------------------------

inline void *serialize(void *ptr, const u_char bytes[], u_int length)
{
   memcpy(ptr, bytes, length);
   return (u_char*) ptr + length;
}

inline void *serialize(void *ptr, const char chars[], u_int length)
{
   memcpy(ptr, chars, length);
   return &((u_char*)ptr)[length];
}

void *serialize(void *ptr, const string_t& value);

template <typename type_t>
inline void *serialize(void *ptr, type_t value)
{
   *((type_t*) ptr) = value;
   return (u_char*) ptr + sizeof(type_t);
}

template <>
inline void *serialize<bool>(void *ptr, bool value)
{
   *((u_char*) ptr) = (u_char) value;
   return (u_char*) ptr + sizeof(u_char);
}

// see the comment above s_size_of<string_t>
template <>
inline void *serialize<string_t>(void *ptr, string_t value)
{
   return serialize(ptr, value);
}

// -----------------------------------------------------------------------
//
// de-serialization
//
// -----------------------------------------------------------------------

inline const void *deserialize(const void *ptr, u_char bytes[], u_int length)
{
   memcpy(bytes, ptr, length);
   return &((u_char*)ptr)[length];
}

inline const void *deserialize(const void *ptr, char chars[], u_int length)
{
   memcpy(chars, ptr, length);
   return &((u_char*)ptr)[length];
}

const void *deserialize(const void *ptr, string_t& value);

template <typename type_t>
inline const void *deserialize(const void *ptr, type_t& value)
{
   value = *((type_t*) ptr);
   return (u_char*)ptr + sizeof(type_t);
}

template <>
inline const void *deserialize<bool>(const void *ptr, bool& value)
{
   value = *((u_char*) ptr) ? true : false;
   return (u_char*) ptr + sizeof(u_char);
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
template <>
inline size_t s_size_of(bool value)
{
   return sizeof(u_char);
}

//
// Define this function so that the primary template function isn't called
// for string_t, which would return size of the string_t object, not the
// serialized length. This function shouldn't be ever called because the
// one below will be.
//
template <>
inline size_t s_size_of<string_t>(string_t value)
{
   return sizeof(u_int) + value.length();
}

// define a non-template function so we can use a reference
inline size_t s_size_of(const string_t& value)
{
   return sizeof(u_int) + value.length();
}

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

// -----------------------------------------------------------------------
//
// comparison functions (buffer)
//
// -----------------------------------------------------------------------
template <typename type_t>
inline int s_compare(const void *buf1, const void *buf2)
{
   return *(type_t*) buf1 == *(type_t*) buf2 ? 0 : *(type_t*) buf1 > *(type_t*) buf2 ? 1 : -1;
}

template <>
inline int s_compare<string_t>(const void *buf1, const void *buf2)
{
   const char *cp1, *cp2;
   u_int slen1, slen2;

   cp1 = (const char*) deserialize(buf1, slen1);
   cp2 = (const char*) deserialize(buf2, slen2);

   return strncmp_ex(cp1, slen1, cp2, slen2);
}

#endif // __SERIALIZE_H
