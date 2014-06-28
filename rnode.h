/*
    webalizer - a web server log analysis program

    Copyright (c) 2004-2014, Stone Steps Inc. (www.stonesteps.ca)

    See COPYING and Copyright files for additional licensing and copyright information

    rnode.h
*/
#ifndef __RNODE_H
#define __RNODE_H

#include "hashtab.h"
#include "types.h"
#include "basenode.h"

/* referrer hash table struct   */
typedef struct rnode_t : public base_node<rnode_t> {   
      bool     hexenc;               // any %xx sequences?
      u_long   count;
      u_long   visits;

      public:
         typedef void (*s_unpack_cb_t)(rnode_t& rnode, void *arg);

      public:
         rnode_t(void) : base_node<rnode_t>() {hexenc = false; count = 0;}
         rnode_t(const rnode_t& rnode);
         rnode_t(const char *ref);

         //
         // serialization
         //
         u_int s_data_size(void) const;
         u_int s_pack_data(void *buffer, u_int bufsize) const;
         u_int s_unpack_data(const void *buffer, u_int bufsize, s_unpack_cb_t upcb, void *arg);

         static u_int s_data_size(const void *buffer);

         static const void *s_field_value_hash(const void *buffer, u_int bufsize, u_int& datasize);
         static const void *s_field_hits(const void *buffer, u_int bufsize, u_int& datasize);

         static int s_compare_hits(const void *buf1, const void *buf2);
} *RNODEPTR;

//
// Referrers
//
class r_hash_table : public hash_table<rnode_t> {
};

#endif // __RNODE_H
