/*
    webalizer - a web server log analysis program

    Copyright (c) 2004-2021, Stone Steps Inc. (www.stonesteps.ca)

    See COPYING and Copyright files for additional licensing and copyright information

    rnode.h
*/
#ifndef RNODE_H
#define RNODE_H

#include "hashtab.h"
#include "types.h"
#include "basenode.h"
#include "storable.h"

///
/// @brief  Referrer node
///
struct rnode_t : public base_node<rnode_t> {   
      uint64_t count;               ///< Request count
      uint64_t visits;              ///< Visits started

      public:
         template <typename ... param_t>
         using s_unpack_cb_t = void (*)(rnode_t& rnode, param_t ... param);

      public:
         rnode_t(void) : count(0), visits(0) {}
         rnode_t(const string_t& ref, nodetype_t type);

         //
         // serialization
         //
         size_t s_data_size(void) const;
         size_t s_pack_data(void *buffer, size_t bufsize) const;

         template <typename ... param_t>
         size_t s_unpack_data(const void *buffer, size_t bufsize, s_unpack_cb_t<param_t ...> upcb, param_t ... param);

         static const void *s_field_value_hash(const void *buffer, size_t bufsize, size_t& datasize);
         static const void *s_field_hits(const void *buffer, size_t bufsize, size_t& datasize);

         static int64_t s_compare_hits(const void *buf1, size_t buf1size, const void *buf2, size_t buf2size);
};

//
// Referrers
//
class r_hash_table : public hash_table<storable_t<rnode_t>> {
};

#endif // RNODE_H
