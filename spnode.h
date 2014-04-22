/*
    webalizer - a web server log analysis program

    Copyright (c) 2004-2013, Stone Steps Inc. (www.stonesteps.ca)

    See COPYING and Copyright files for additional licensing and copyright information

    spnode.h
*/
#ifndef __SPNODE_H
#define __SPNODE_H

#include "hashtab.h"
#include "basenode.h"
#include "types.h"

// spammer
struct spnode_t : public base_node<spnode_t> {
      public:
         typedef void (*s_unpack_cb_t)(spnode_t& spnode, void *arg);

      public:
         spnode_t(void) : base_node<spnode_t>() {}
         spnode_t(const char *host) : base_node<spnode_t>(host) {}
};

//
// Spammers
//
class sp_hash_table : public hash_table<spnode_t> {
};

#endif // __SPNODE_H

