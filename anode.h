/*
    webalizer - a web server log analysis program

    Copyright (c) 2004-2014, Stone Steps Inc. (www.stonesteps.ca)

    See COPYING and Copyright files for additional licensing and copyright information

    anode.h
*/
#ifndef __ANODE_H
#define __ANODE_H

#include "hashtab.h"
#include "basenode.h"
#include "types.h"

typedef struct anode_t : public base_node<anode_t> {
      u_long   count;
      u_long   visits;
      
      double   xfer;
      
      bool     robot : 1;

      public:
         typedef void (*s_unpack_cb_t)(anode_t& anode, void *arg);

      public:
         anode_t(void);
         anode_t(const anode_t& anode);
         anode_t(const string_t& agent, bool robot);

         //
         // serialization
         //
         u_int s_data_size(void) const;
         u_int s_pack_data(void *buffer, u_int bufsize) const;
         u_int s_unpack_data(const void *buffer, u_int bufsize, s_unpack_cb_t upcb, void *arg);

         static u_int s_data_size(const void *buffer);

         static const void *s_field_value_hash(const void *buffer, u_int bufsize, u_int& datasize);
         static const void *s_field_hits(const void *buffer, u_int bufsize, u_int& datasize);
         static const void *s_field_visits(const void *buffer, u_int bufsize, u_int& datasize);

         static int s_compare_hits(const void *buf1, const void *buf2);
         static int s_compare_visits(const void *buf1, const void *buf2);
} *ANODEPTR;

//
// User Agents
//
class a_hash_table : public hash_table<anode_t> {
};

#endif // __ANODE_H
