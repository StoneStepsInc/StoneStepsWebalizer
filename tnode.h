/*
    webalizer - a web server log analysis program

    Copyright (c) 2004-2014, Stone Steps Inc. (www.stonesteps.ca)

    See COPYING and Copyright files for additional licensing and copyright information

    tnode.h
*/
#ifndef __TNODE_H
#define __TNODE_H

#include "hashtab.h"
#include "basenode.h"
#include "types.h"

/* daily host struct */
struct tnode_t : public base_node<tnode_t> {
      public:
         typedef void (*s_unpack_cb_t)(tnode_t& tnode, void *arg);

      public:
         tnode_t(void) : base_node<tnode_t>() {}
         tnode_t(const tnode_t& tnode) : base_node<tnode_t>(tnode) {}
         tnode_t(const char *host) : base_node<tnode_t>(host) {}

         //
         // serialization
         //
         size_t s_data_size(void) const;
         size_t s_pack_data(void *buffer, size_t bufsize) const;
         size_t s_unpack_data(const void *buffer, size_t bufsize, s_unpack_cb_t upcb, void *arg);

         static size_t s_data_size(const void *buffer);

         static const void *s_field_value_hash(const void *buffer, size_t bufsize, size_t& datasize);
};

//
// Hosts (daily)
//
class t_hash_table : public hash_table<tnode_t> {
   public:
      t_hash_table(void) : hash_table<tnode_t>(LMAXHASH) {}
};

#endif // __TNODE_H
