/*
    webalizer - a web server log analysis program

    Copyright (c) 2004-2014, Stone Steps Inc. (www.stonesteps.ca)

    See COPYING and Copyright files for additional licensing and copyright information

    ccnode.h
*/
#ifndef __CCNODE_H
#define __CCNODE_H

#include "hashtab.h"
#include "types.h"
#include "keynode.h"
#include "datanode.h"

// -----------------------------------------------------------------------
// ccnode_t
//
// 1. Country code nodes are identified by an index value returned by
// ctry_idx. 
//
// 2. Country code and description strings are not stored in the database,
// but instead are set up from the language file on start-up.
// -----------------------------------------------------------------------
struct ccnode_t : public htab_node_t<ccnode_t>, public keynode_t<uint64_t>, public datanode_t<ccnode_t> {
   string_t    ccode;
   string_t    cdesc;
   uint64_t    count;
   uint64_t    files;
   uint64_t    visits;
   uint64_t    xfer;

   public:
      typedef void (*s_unpack_cb_t)(ccnode_t& vnode, void *arg);

   public:
      ccnode_t(void);

      ccnode_t(const char *cc, const char *desc);

      const string_t& key(void) const {return ccode;}

      nodetype_t get_type(void) const {return OBJ_REG;}

      void reset(void) {count = 0; files = 0; xfer = 0; visits = 0;}
      
      void update(const ccnode_t& ccnode);

      //
      // serialization
      //
      size_t s_data_size(void) const;
      size_t s_pack_data(void *buffer, size_t bufsize) const;
      size_t s_unpack_data(const void *buffer, size_t bufsize, s_unpack_cb_t upcb, void *arg);

      static size_t s_data_size(const void *buffer);
};

class cc_hash_table : public hash_table<ccnode_t> {
   ccnode_t empty;

   public:
      cc_hash_table(void) : hash_table<ccnode_t>(SMAXHASH) {}

      void reset(void);
      
      void update_ccnode(const ccnode_t& ccnode) {get_ccnode(ccnode.ccode).update(ccnode);}

      void put_ccnode(const char *ccode, const char *cdesc) {put_node(hash_ex(0, ccode), new ccnode_t(ccode, cdesc));}

      const ccnode_t& get_ccnode(const string_t& ccode) const;

      ccnode_t& get_ccnode(const string_t& ccode);
};

#endif // __CCNODE_H
