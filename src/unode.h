/*
    webalizer - a web server log analysis program

    Copyright (c) 2004-2017, Stone Steps Inc. (www.stonesteps.ca)

    See COPYING and Copyright files for additional licensing and copyright information

    unode.h
*/
#ifndef UNODE_H
#define UNODE_H

#include "hashtab.h"
#include "basenode.h"
#include "types.h"

// -----------------------------------------------------------------------
//
// URL
//
// -----------------------------------------------------------------------
// 1. vstref is not a generic reference count, but rather an app-level
// indicator of how many visit nodes refer to this URL. That is, if 
// vstref is zero, unode_t may still be valid (although may be swapped
// out of memory at any time). 
//
struct unode_t : public base_node<unode_t> {
      bool     target : 1;          // Target URL?
      u_char   urltype;             // URL type (e.g. URL_TYPE_HTTP)
      u_short  pathlen;             // URL path length
      uint64_t count;               // requests counter
      uint64_t files;               // files counter 
      uint64_t entry;               // entry page counter
      uint64_t exit;                // exit page counter

      uint64_t vstref;              // visit references

      uint64_t xfer;                // xfer size in bytes
      double   avgtime;             // average processing time (sec)
      double   maxtime;             // maximum processing time (sec)

      public:
         typedef void (*s_unpack_cb_t)(unode_t& unode, void *arg);

      public:
         unode_t(uint64_t nodeid = 0);
         unode_t(const unode_t& unode);
         unode_t(const string_t& urlpath, const string_t& srchargs);

         void reset(uint64_t nodeid = 0);

         u_char update_url_type(u_char type);

         //
         // serialization
         //
         size_t s_data_size(void) const;
         size_t s_pack_data(void *buffer, size_t bufsize) const;
         size_t s_unpack_data(const void *buffer, size_t bufsize, s_unpack_cb_t upcb, void *arg);

         static size_t s_data_size(const void *buffer);

         static const void *s_field_value_hash(const void *buffer, size_t bufsize, size_t& datasize);
         static const void *s_field_xfer(const void *buffer, size_t bufsize, size_t& datasize);
         static const void *s_field_hits(const void *buffer, size_t bufsize, size_t& datasize);
         static const void *s_field_entry(const void *buffer, size_t bufsize, size_t& datasize);
         static const void *s_field_exit(const void *buffer, size_t bufsize, size_t& datasize);

         static int64_t s_compare_xfer(const void *buf1, const void *buf2);
         static int64_t s_compare_hits(const void *buf1, const void *buf2);
         static int64_t s_compare_entry(const void *buf1, const void *buf2);
         static int64_t s_compare_exit(const void *buf1, const void *buf2);
};

//
// URLs
//
class u_hash_table : public hash_table<unode_t> {
   public:
      struct param_block {
         nodetype_t type;
         const string_t *url;
         const string_t *srchargs;
      };

   private:
      virtual bool compare(const unode_t *nptr, const void *param) const;

   public:
      u_hash_table(void) : hash_table<unode_t>(LMAXHASH) {}
};

#endif // UNODE_H
