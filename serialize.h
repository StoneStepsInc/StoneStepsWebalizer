/*
    webalizer - a web server log analysis program

    Copyright (c) 2004-2014, Stone Steps Inc. (www.stonesteps.ca)

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

// serialize strings as a character count followed by string characters
void *serialize(void *ptr, const string_t& value);

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

// read strings as a character count followed by string characters
const void *deserialize(const void *ptr, string_t& value);

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

//
// Compatibility deserializer reads values whose data type changed at the specified 
// version. The primary template defines three type parameters:
//
//    org_storage_t  - original storage data type used prior to the switch version
//    new_storage_t  - new storage data type used on or after the switch version
//    runtime_t      - run time data type
//
// For example, a time_t value, which was often defined as a 32-bit value in the past, 
// was stored in the database as a u_long value, which also is a 32-bit value on many 
// platforms. Now time_t is defined as a 64-bit value by many compilers, so we need to 
// read a u_long value, convert it to time_t so we can use it at run time, save as a 
// 64-bit value in the updated database and then read this 64-bit value in subsequent 
// runs.
//
template <u_short switch_version, typename org_storage_t, typename new_storage_t, typename runtime_t = new_storage_t>
class compatibility_deserializer {
   u_short  version;          // actual value version

   public:
      compatibility_deserializer(u_short version) : version(version) {}

      const void *operator () (const void *buf, runtime_t& value)
      {
         // read values in their original format prior to the switch version
         if(version < switch_version)
            return deserialize<org_storage_t, runtime_t>(buf, value); 
         
         // otherwise read in the new format
         return deserialize<new_storage_t, runtime_t>(buf, value);
      }
};

//
// This specialized template is used when variable run time type is the same as the 
// storage type, so new format values will be read directly into the destination
// variable.
//
template <u_short switch_version, typename org_storage_t, typename new_storage_t>
class compatibility_deserializer<switch_version, org_storage_t, new_storage_t, new_storage_t> {
   u_short  version;          // actual value version

   public:
      compatibility_deserializer(u_short version) : version(version) {}

      const void *operator () (const void *buf, new_storage_t& value)
      {
         // read values in their original format prior to the switch version
         if(version < switch_version)
            return deserialize<org_storage_t, new_storage_t>(buf, value);
         
         // otherwise read in the new format    
         return deserialize(buf, value);
      }
};

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
inline size_t s_size_of(bool value)
{
   return sizeof(u_char);
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
