/*
    webalizer - a web server log analysis program

    Copyright (c) 2004-2021, Stone Steps Inc. (www.stonesteps.ca)

    See COPYING and Copyright files for additional licensing and copyright information

    inode.h
*/
#ifndef INODE_H
#define INODE_H

#include "hashtab.h"
#include "basenode.h"
#include "tstamp.h"
#include "types.h"
#include "storable.h"

///
/// @brief  User node
///
struct inode_t : public base_node<inode_t> {
      uint64_t count;               ///< Request count
      uint64_t files;               ///< Files requested
      uint64_t visit;               ///< Visits started
      tstamp_t tstamp;              ///< Last request time
      uint64_t xfer;                ///< Transfer amount, in bytes
      double   avgtime;             ///< Average processing time (sec)
      double   maxtime;             ///< Maximum processing time (sec)

      public:
         template <typename ... param_t>
         using s_unpack_cb_t = void (*)(inode_t& rnode, param_t ... param);

      public:
         inode_t(void);
         inode_t(const string_t& ident, nodetype_t type);

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
// Users
//
class i_hash_table : public hash_table<storable_t<inode_t>> {
   public:
      i_hash_table(void) : hash_table<storable_t<inode_t>>(LMAXHASH) {}
};

#endif // INODE_H
