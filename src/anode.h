/*
    webalizer - a web server log analysis program

    Copyright (c) 2004-2017, Stone Steps Inc. (www.stonesteps.ca)

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
/// @brief  User agent node
///
struct anode_t : public base_node<anode_t> {
      uint64_t count;                  // request count
      uint64_t visits;                 // visits started
      
      uint64_t xfer;                   // transfer amount in bytes
      
      bool     robot : 1;              // matches the robot pattern?

      public:
         typedef void (*s_unpack_cb_t)(anode_t& anode, void *arg);

      public:
         anode_t(void);
         anode_t(const anode_t& anode);
         anode_t(const string_t& agent, bool robot);

         //
         // serialization
         //
         size_t s_data_size(void) const;
         size_t s_pack_data(void *buffer, size_t bufsize) const;
         size_t s_unpack_data(const void *buffer, size_t bufsize, s_unpack_cb_t upcb, void *arg);

         static size_t s_data_size(const void *buffer);

         static const void *s_field_value_hash(const void *buffer, size_t bufsize, size_t& datasize);
         static const void *s_field_hits(const void *buffer, size_t bufsize, size_t& datasize);
         static const void *s_field_visits(const void *buffer, size_t bufsize, size_t& datasize);

         static int64_t s_compare_hits(const void *buf1, const void *buf2);
         static int64_t s_compare_visits(const void *buf1, const void *buf2);
};

//
// User Agents
//
class a_hash_table : public hash_table<storable_t<anode_t>> {
};

#endif // ANODE_H
