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

///
/// @typedef   s_compare_cb_t
///
/// @brief  Compares two serialized fields
///
/// `buf1` and `buf2` should point to the beginning of two values being compared, 
/// as returned by one of the field extractor/pointer functions.
///
typedef int64_t (*s_compare_cb_t)(const void *buf1, const void *buf2);

///
/// @typedef   s_field_cb_t
///
/// @brief  Returns a pointer to a serialized field.
///
/// `buffer` should point to a data record containing the requested field. The 
/// return value points to a field within the record and datasize is set to 
/// reflect the size of the field.
///
typedef const void *(*s_field_cb_t)(const void *buffer, size_t bufsize, size_t& datasize);

///
/// @typedef   s_mp_compare_cb_t
///
/// @brief  Compares the specified part of a serialized multi-part field.
///
/// `buf1` and `buf2` should point to the first part of two multipart values being 
/// compared. Zero-based `partid` indicates which part should be evaluated. Function 
/// behavior is undefined if `partid` is greater than the maximum valid value. When 
/// the last part of the value is evaluated, `lastpart` will be set by the function 
/// to `true`.
///
typedef int64_t (*s_mp_compare_cb_t)(const void *buf1, const void *buf2, u_int partid, bool& lastpart);

///
/// @typedef   s_mp_field_cb_t
///
/// @brief  Returns a pointer to a serialized part of a multi-part field
///
/// `buffer` should point to a data record containing the requested field. The return 
/// value points to the specified part of a multipart value. When the last part of the 
/// value is evaluated, `lastpart` will be set by the function to `true`.
///
typedef const void *(*s_mp_field_cb_t)(const void *buffer, size_t bufsize, size_t& datasize, u_int partid, bool& lastpart);

// -----------------------------------------------------------------------
//
// serialization
//
// -----------------------------------------------------------------------

///
/// @brief  Serializes a buffer of `u_char` characters.
///
inline void *serialize(void *ptr, const u_char bytes[], size_t length)
{
   memcpy(ptr, bytes, length);
   return (u_char*) ptr + length;
}

///
/// @brief  Serializes a buffer of `char` characters.
///
inline void *serialize(void *ptr, const char chars[], size_t length)
{
   memcpy(ptr, chars, length);
   return &((u_char*)ptr)[length];
}

///
/// @brief  Serializes strings as a character count followed by string characters.
///
void *serialize(void *ptr, const string_t& value);

///
/// @brief  Serializes time stamps as individual components.
///
void *serialize(void *ptr, const tstamp_t& tstamp);

///
/// @brief  Serializes a numeric value.
///
/// @tparam type_t   Numeric value data type
///
template <typename type_t>
inline void *serialize(void *ptr, type_t value)
{
   *((type_t*) ptr) = value;
   return (u_char*) ptr + sizeof(type_t);
}

///
/// @brief  Serializes `bool` as a `u_char` value.
///
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

///
/// @brief  Deseriaizes a series of `u_char` characters.
///
inline const void *deserialize(const void *ptr, u_char bytes[], size_t length)
{
   memcpy(bytes, ptr, length);
   return &((u_char*)ptr)[length];
}

///
/// @brief  Deseriaizes a series of `char` characters.
///
inline const void *deserialize(const void *ptr, char chars[], size_t length)
{
   memcpy(chars, ptr, length);
   return &((u_char*)ptr)[length];
}

///
/// @brief  Reads strings as a character count followed by string characters.
///
const void *deserialize(const void *ptr, string_t& value);

///
/// @brief  Reads time stamp components one at a time.
///
const void *deserialize(const void *ptr, tstamp_t& tstamp);

///
/// @brief  Reads a serialized value whose storage type is the same as the 
///         run time type.
///
/// @tparam type_t   Field data type
///
template <typename type_t>
inline const void *deserialize(const void *ptr, type_t& value)
{
   value = *((type_t*) ptr);
   return (u_char*)ptr + sizeof(type_t);
}

///
/// @brief  Reads a serialized value whose storage type differs from the run 
///         time type.
///
/// @tparam storage_t   Field storage data type
/// @tparam runtime_t   Field value data type
///
/// Runtime type may be `size_t` or `int`, but the range of values stored may
/// be limited to a smaller subset than the runtime type would allow and may
/// be stored in a smaller storage type, such as `unsigned short`.
///
template <typename storage_t, typename runtime_t>
const void *deserialize(const void *buf, runtime_t& value)
{
   storage_t stored_value;

   const void *ptr = deserialize(buf, stored_value);
   value = (runtime_t) stored_value;

   return ptr;
}

///
/// @brif   Reads `bool` as `u_char`.
///
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

///
/// @brief  Returns the storage size of a value.
///
/// @tparam type_t   Value data type.
///
template <typename type_t>
inline size_t s_size_of(type_t value)
{
   return sizeof(type_t);
}

///
/// @brief  Returns the storage size of a `bool` value.
///
inline size_t s_size_of(bool value)
{
   return sizeof(u_char);
}

///
/// @brief  Returns the storage size of a string value.
///
inline size_t s_size_of(const string_t& value)
{
   return sizeof(u_int) + value.length();
}

///
/// @brief  Returns the storage size of a time stamp value.
///
size_t s_size_of(const tstamp_t& tstamp);

// -----------------------------------------------------------------------
//
// size-of functions (buffer)
//
// -----------------------------------------------------------------------

///
/// @brief  Returns the size of a `type_t` value in a buffer.
///
/// @tparam type_t   Field data type
///
template <typename type_t>
inline size_t s_size_of(const void *ptr)
{
   return sizeof(type_t);
}

///
/// @brief  Returns the size of a `bool` value in a buffer.
///
template <>
inline size_t s_size_of<bool>(const void *ptr)
{
   return sizeof(u_char);
}

///
/// @brief  Returns the size of a string value in a buffer.
///
template <>
inline size_t s_size_of<string_t>(const void *ptr)
{
   return sizeof(u_int) + (*(u_int*)ptr);
}

///
/// @brief  Returns the size of a time stamp value in a buffer.
///
template <> 
size_t s_size_of<tstamp_t>(const void *ptr);

// -----------------------------------------------------------------------
//
// skip functions
//
// -----------------------------------------------------------------------

///
/// @brief  Skips a field of type `type_t` and returns a pointer to the first
///         byte past the skippped field.
///
/// @tparam type_t   Field data type
///
template <typename type_t>
inline void *s_skip_field(const void *ptr)
{
   return (u_char*) ptr + sizeof(type_t);
}

///
/// @brief  Skips a `bool` field and returns a pointer to the first
///         byte past the skippped field.
///
template <>
inline void *s_skip_field<bool>(const void *ptr)
{
   return (u_char*) ptr + sizeof(u_char);
}

///
/// @brief  Skips a string field and returns a pointer to the first
///         byte past the skippped field. 
///
template <>
inline void *s_skip_field<string_t>(const void *ptr)
{
   return (u_char*) ptr + sizeof(u_int) + *(u_int*)ptr;
}

///
/// @brief  Skips a time stamp field and returns a pointer to the first
///         byte past the skippped field.
///
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

///
/// @brief  Compares two values of `type_t` in their buffers.
///
/// @tparam type_t   Field value data type.
///
template <typename type_t>
inline int64_t s_compare(const void *buf1, const void *buf2)
{
   return *(type_t*) buf1 == *(type_t*) buf2 ? 0 : *(type_t*) buf1 > *(type_t*) buf2 ? 1 : -1;
}

///
/// @brief  Compares two strings in their buffers.
///
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
