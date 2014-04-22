/*
    webalizer - a web server log analysis program

    Copyright (c) 2004-2013, Stone Steps Inc. (www.stonesteps.ca)

    See COPYING and Copyright files for additional licensing and copyright information

    snode.h
*/
#ifndef __SNODE_H
#define __SNODE_H

#include "hashtab.h"
#include "basenode.h"
#include "types.h"

/* search string struct      */
typedef struct snode_t : public base_node<snode_t> {     
      u_short        termcnt;
      u_long         count;
      u_long         visits;

      public:
         typedef void (*s_unpack_cb_t)(snode_t& snode, void *arg);

      public:
         snode_t(void) : base_node<snode_t>() {count = 0; termcnt = 0; visits = 0;}
         snode_t(const snode_t& snode);
         snode_t(const char *srch) : base_node<snode_t>(srch) {count = 0; termcnt = 0; visits = 0;}

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
} *SNODEPTR;

//
// Search Strings
//
class s_hash_table : public hash_table<snode_t> {
};

#endif // __SNODE_H
