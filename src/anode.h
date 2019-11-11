/*
    webalizer - a web server log analysis program

    Copyright (c) 2004-2019, Stone Steps Inc. (www.stonesteps.ca)

    See COPYING and Copyright files for additional licensing and copyright information

    anode.h
*/
#ifndef ANODE_H
#define ANODE_H

#include "hashtab.h"
#include "basenode.h"
#include "types.h"
#include "storable.h"

///
/// @struct anode_t
///
/// @brief  A user agent node
///
struct anode_t : public base_node<anode_t> {
      uint64_t count;                  ///< Request count
      uint64_t visits;                 ///< Visits started
      
      uint64_t xfer;                   ///< Transfer amount in bytes
      
      bool     robot : 1;              ///< Matches the robot pattern?

      public:
         ///
         /// @brief  An alias template for a callback function that will be called
         ///         after the node has been deserialized.
         ///
         template <typename ... param_t>
         using s_unpack_cb_t = void (*)(anode_t& anode, param_t ... param);

      public:
         /// Constructs an empty instance of a user agent node.
         anode_t(void);

         /// Constructs an instance of a user agent node with the `agent` string.
         anode_t(const string_t& agent, bool robot);

         ///
         /// @name   Serialization
         ///
         /// @{

         /// Returns a serialized size of this user agent node instance.
         size_t s_data_size(void) const;

         /// Serializes this user agent node into the specified buffer and returns a serialized size.
         size_t s_pack_data(void *buffer, size_t bufsize) const;

         /// Deserializes a user agent node instance from the specified buffer and returns a deserialized size.
         template <typename ... param_t>
         size_t s_unpack_data(const void *buffer, size_t bufsize, s_unpack_cb_t<param_t ...> upcb, param_t ... param);

         /// Returns a pointer to the value hash within a serialized user agent node data.
         static const void *s_field_value_hash(const void *buffer, size_t bufsize, size_t& datasize);

         /// Returns a pointer to the hit count value within a serialized user agent node data.
         static const void *s_field_hits(const void *buffer, size_t bufsize, size_t& datasize);

         /// Returns a pointer to the visit count value within a serialized user agent node data.
         static const void *s_field_visits(const void *buffer, size_t bufsize, size_t& datasize);

         /// Compares two serialized hit count values.
         static int64_t s_compare_hits(const void *buf1, size_t buf1size, const void *buf2, size_t buf2size);

         /// Compares two serialized visit count values.
         static int64_t s_compare_visits(const void *buf1, size_t buf1size, const void *buf2, size_t buf2size);
         /// @}
};

///
/// @brief  A hash table to store user agent nodes.
///
class a_hash_table : public hash_table<storable_t<anode_t>> {
};

#endif // ANODE_H
