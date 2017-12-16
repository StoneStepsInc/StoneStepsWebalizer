/*
    webalizer - a web server log analysis program

    Copyright (c) 2004-2017, Stone Steps Inc. (www.stonesteps.ca)

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
/// @struct inode_t
///
/// @brief  User node
///
struct inode_t : public base_node<inode_t> {
      uint64_t count;               // request count
      uint64_t files;               // files requested
      uint64_t visit;               // visits started
      tstamp_t tstamp;              // last request time
      uint64_t xfer;                // transfer amount, in bytes
      double   avgtime;             // average processing time (sec)
      double   maxtime;             // maximum processing time (sec)

      public:
         typedef void (*s_unpack_cb_t)(inode_t& rnode, void *arg);

      public:
         inode_t(void);
         inode_t(const inode_t& inode);
         inode_t(const string_t& ident);

         //
         // serialization
         //
         size_t s_data_size(void) const;
         size_t s_pack_data(void *buffer, size_t bufsize) const;
         size_t s_unpack_data(const void *buffer, size_t bufsize, s_unpack_cb_t upcb, void *arg);

         static size_t s_data_size(const void *buffer);

         static const void *s_field_value_hash(const void *buffer, size_t bufsize, size_t& datasize);
         static const void *s_field_hits(const void *buffer, size_t bufsize, size_t& datasize);

         static int64_t s_compare_hits(const void *buf1, const void *buf2);
};

//
// Users
//
class i_hash_table : public hash_table<storable_t<inode_t>> {
   public:
      i_hash_table(void) : hash_table<storable_t<inode_t>>(LMAXHASH) {}
};

#endif // INODE_H
