/*
    webalizer - a web server log analysis program

    Copyright (c) 2004-2013, Stone Steps Inc. (www.stonesteps.ca)

    See COPYING and Copyright files for additional licensing and copyright information

    tnode.h
*/
#ifndef __TNODE_H
#define __TNODE_H

#include "hashtab.h"
#include "basenode.h"
#include "types.h"

/* daily host struct */
typedef struct tnode_t : public base_node<tnode_t> {
      public:
         typedef void (*s_unpack_cb_t)(tnode_t& tnode, void *arg);

      public:
         tnode_t(void) : base_node<tnode_t>() {}
         tnode_t(const tnode_t& tnode) : base_node<tnode_t>(tnode) {}
         tnode_t(const char *host) : base_node<tnode_t>(host) {}

         //
         // serialization
         //
         u_int s_data_size(void) const;
         u_int s_pack_data(void *buffer, u_int bufsize) const;
         u_int s_unpack_data(const void *buffer, u_int bufsize, s_unpack_cb_t upcb, void *arg);

         static u_int s_data_size(const void *buffer);

         static const void *s_field_value_hash(const void *buffer, u_int bufsize, u_int& datasize);
} *TNODEPTR;

//
// Hosts (daily)
//
class t_hash_table : public hash_table<tnode_t> {
   public:
      t_hash_table(void) : hash_table<tnode_t>(LMAXHASH) {}
};

#endif // __TNODE_H
