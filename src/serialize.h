/*
    webalizer - a web server log analysis program

    Copyright (c) 2004-2019, Stone Steps Inc. (www.stonesteps.ca)

    See COPYING and Copyright files for additional licensing and copyright information

    serialize.h
*/
#ifndef SERIALIZE_H
#define SERIALIZE_H

#include "util_string.h"
#include "tstring.h"
#include "tstamp.h"
#include "types.h"

#include <cstddef>

///
/// @brief  A class that handles all data serialization and deserialization operations.
///
/// Serializer objects operate within a fixed buffer size and do not change throughout
/// their life span. The caller maintains a serialization state by keeping track of the
/// current buffer reading or writing position and the serializer ensures that no
/// operation ever crosses either of the buffer boundaries.
///
/// Serializer throws an exception whenever any operation would reach outside the buffer,
/// with the exception of a read/write pointer being one byte past the buffer size after
/// reading or writing, which is expected. Any subsequent attempt to read from or write
/// into this buffer location will throw an exception.
///
/// Only `std::invalid_argument` are thrown because even bad data, such as a stored
/// string length exceeding the buffer size, is likely caused by incorrect positioning
/// of the read pointer within the buffer. Without some checksum mechanism it is impossible
/// to distinguish buffer corruption from a bad read position.
///
class serializer_t {
   private:
      const void     *buffer;          ///< Serialization buffer. 
      const size_t   bufsize;          ///< Buffer size in bytes.

   private:
      /// Returns available buffer space, in bytes, that can store more data.
      size_t buffer_space(const void *ptr) const;

      /// Serializes an arbitrary sequence of bytes.
      void *serialize(void *ptr, const char chars[], size_t length) const;

      /// Deserializes an arbitrary sequence of bytes.
      const void *deserialize(const void *ptr, char chars[], size_t length) const;

      /// Skips an arbitrary sequence of bytes.
      const void *s_skip_field(const void *ptr, size_t length) const;

   public:
      /// Constructs a serializer for the specified buffer of a fixed size.
      serializer_t(const void *buffer, size_t bufsize);

      /// Returns the number of bytes in the buffer up to, but not including the specified pointer.
      size_t data_size(const void *ptr) const;

      ///
      /// @name   Instance data size methods
      ///
      /// All methods in this group return the buffer space required to store serialized
      /// instance data.
      ///
      /// @{

      /// Returns required storage size for numeric data types.
      template <typename type_t>
      static size_t s_size_of(void);

      /// Returns required storage size for numeric data types.
      template <typename type_t>
      static size_t s_size_of(type_t value);

      /// Returns required storage size for a `string_t` instance.
      static size_t s_size_of(const string_t& value);

      /// Returns required storage size for a `tstamp_t` instance.
      static size_t s_size_of(const tstamp_t& tstamp);

      /// Returns required storage size for a `bool` value.
      static size_t s_size_of(bool value);

      /// Cannot serialize arbitrary arrays of bytes. A two-character specialization is provided.
      template <size_t N>
      static size_t s_size_of(const char (&chars)[N]) = delete;

      /// Cannot serialize arbitrary strings of characters.
      static size_t s_size_of(char*) = delete;

      /// Cannot serialize arbitrary strings of `const` characters.
      static size_t s_size_of(const char*) = delete;

      /// @}

      ///
      /// @name   Serialized data size methods
      ///
      /// These methods, including their specializations, check if the buffer has enough data
      /// to accommodate a field of `type_t` type.
      ///
      /// @{

      /// Returns the size of `type_t` stored in the buffer.
      template <typename type_t>
      size_t s_size_of(const void *ptr) const;

      /// @}

      ///
      /// @name   Skip field methods
      ///
      /// @{

      /// Skips a numeric field and returns the position of the next field.
      template <typename type_t>
      const void *s_skip_field(const void *ptr) const;

      /// @}
      ///
      /// @name   Serialization methods
      ///
      /// @{

      /// Cannot serialize arbitrary arrays of bytes. A two-character specialization is provided.
      template <size_t N>
      void *serialize(void *ptr, const char (&chars)[N]) const = delete;

      /// Serializes a numeric value and returns the position after the serialized field.
      template <typename type_t>
      void *serialize(void *ptr, type_t value) const;

      /// Serializes a numeric value with different storage and runtime types and returns the position after the serialized field.
      template <typename storage_t, typename runtime_t>
      void *serialize(void *ptr, runtime_t value) const;

      /// Serializes a `string_t` value and returns the position after the serialized field.
      void *serialize(void *ptr, const string_t& value) const;

      /// Serializes a `tstamp_t` value and returns the position after the serialized field.
      void *serialize(void *ptr, const tstamp_t& tstamp) const;

      /// Serializes a `bool` value and returns the position after the serialized field.
      void *serialize(void *ptr, bool value) const;

      /// @}

      ///
      /// @name   Deserialization methods
      ///
      /// @{

      /// Cannot deserialize arbitrary arrays of bytes. A two-character specialization is provided.
      template <size_t N>
      const void *deserialize(const void *ptr, char (&chars)[N]) const = delete;

      /// Deserializes a numeric value and returns the position after the field that was just read.
      template <typename type_t>
      const void *deserialize(const void *ptr, type_t& value) const;

      /// Deserializes a numeric value with different storage and runtime types and returns the position after the field that was just read.
      template <typename storage_t, typename runtime_t>
      const void *deserialize(const void *buf, runtime_t& value) const;

      /// Deserializes a string pointer and length. The pointer is within the buffer for non-zero length values.
      const void *deserialize(const void *ptr, const char *&value, size_t& slen) const;

      /// Deserializes a `string_t` value and returns the position after the field that was just read.
      const void *deserialize(const void *ptr, string_t& value) const;

      /// Deserializes a `tstamp_t` value and returns the position after the field that was just read.
      const void *deserialize(const void *ptr, tstamp_t& tstamp) const;

      /// Deserializes a `bool` value and returns the position after the field that was just read.
      const void *deserialize(const void *ptr, bool& value) const;

      /// @}
};

///
/// @name   Instance data size methods specializations
/// @{
template <> size_t serializer_t::s_size_of<bool>(void);
template <> size_t serializer_t::s_size_of<string_t>(void) = delete;
template <> size_t serializer_t::s_size_of<tstamp_t>(void) = delete;

template <> size_t serializer_t::s_size_of(const char (&chars)[2]);
/// @}

///
/// @name   Skip field methods specializations
///
/// @{
template <> const void *serializer_t::s_skip_field<string_t>(const void *ptr) const;
template <> const void *serializer_t::s_skip_field<tstamp_t>(const void *ptr) const;
template <> const void *serializer_t::s_skip_field<bool>(const void *ptr) const;
/// @}

///
/// @name   Serialization methods specializations
///
/// @{
template <> void *serializer_t::serialize(void *ptr, const char (&chars)[2]) const;
/// @}

///
/// @name   Deserialization methods specializations
///
/// @{
template <> const void *serializer_t::deserialize(const void *ptr, char (&chars)[2]) const;
/// @}

///
/// @typedef   s_compare_cb_t
///
/// @brief  Compares two serialized fields
///
/// `buf1` and `buf2` should point to the beginning of two values being compared, 
/// as returned by one of the field extractor/pointer functions.
///
typedef int64_t (*s_compare_cb_t)(const void *buf1, size_t buf1size, const void *buf2, size_t buf2size);

///
/// @typedef   s_field_cb_t
///
/// @brief  Returns a pointer to a serialized field.
///
/// `buffer` should point to a data record containing the requested field. The 
/// return value points to a field within the record and `datasize` is set to 
/// reflect the size of the field.
///
typedef const void *(*s_field_cb_t)(const void *buffer, size_t bufsize, size_t& datasize);

///
/// @name   Comparison functions (buffer)
///
/// @{

///
/// @brief  Compares two values of `type_t` in their buffers.
///
/// @tparam type_t   Field value data type.
///
template <typename type_t>
inline int64_t s_compare(const void *buf1, size_t buf1size, const void *buf2, size_t buf2size)
{
   serializer_t sr1(buf1, buf1size);
   serializer_t sr2(buf2, buf2size);

   type_t val1, val2;

   sr1.deserialize(buf1, val1);
   sr2.deserialize(buf2, val2);

   return val1 == val2 ? 0 : val1 > val2 ? 1 : -1;
}

///
/// @brief  Compares two strings in their buffers.
///
template <>
inline int64_t s_compare<string_t>(const void *buf1, size_t buf1size, const void *buf2, size_t buf2size)
{
   serializer_t sr1(buf1, buf1size);
   serializer_t sr2(buf2, buf2size);

   // make sure both strings are within their buffer sizes
   sr1.s_skip_field<string_t>(buf1);
   sr2.s_skip_field<string_t>(buf2);

   const char *cp1, *cp2;
   u_int slen1, slen2;

   // avoid copying strings and instead get their lengths
   cp1 = (const char*) sr1.deserialize(buf1, slen1);
   cp2 = (const char*) sr2.deserialize(buf2, slen2);

   // and compare strings right in the buffer
   return strncmp_ex(cp1, slen1, cp2, slen2);
}

/// @}

#endif // SERIALIZE_H
