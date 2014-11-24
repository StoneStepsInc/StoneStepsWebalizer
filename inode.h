/*
    webalizer - a web server log analysis program

    Copyright (c) 2004-2014, Stone Steps Inc. (www.stonesteps.ca)

    See COPYING and Copyright files for additional licensing and copyright information

    inode.h
*/
#ifndef __INODE_H
#define __INODE_H

#include "hashtab.h"
#include "basenode.h"
#include "types.h"

/* host hash table struct    */
typedef struct inode_t : public base_node<inode_t> {
      uint64_t   count;
      uint64_t   files;
      uint64_t   visit;
      tstamp_t tstamp;               // last request time
      uint64_t xfer;
      double   avgtime;				    // average processing time (sec)
      double   maxtime;              // maximum processing time (sec)

      public:
         typedef void (*s_unpack_cb_t)(inode_t& rnode, void *arg);

      public:
         inode_t(void);
         inode_t(const inode_t& inode);
         inode_t(const char *ident);

         //
         // serialization
         //
         u_int s_data_size(void) const;
         u_int s_pack_data(void *buffer, u_int bufsize) const;
         u_int s_unpack_data(const void *buffer, u_int bufsize, s_unpack_cb_t upcb, void *arg);

         static u_int s_data_size(const void *buffer);

         static const void *s_field_value_hash(const void *buffer, u_int bufsize, u_int& datasize);
         static const void *s_field_hits(const void *buffer, u_int bufsize, u_int& datasize);

         static int64_t s_compare_hits(const void *buf1, const void *buf2);
} *INODEPTR;

//
// Users
//
class i_hash_table : public hash_table<inode_t> {
   public:
      i_hash_table(void) : hash_table<inode_t>(LMAXHASH) {}
};

#endif // __INODE_H
