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
#include "storable.h"

///
/// @struct unode_t
///
/// @brief  URL node
///
/// 1. `vstref` is not a generic reference count, but rather an app-level indicator 
/// of how many visit nodes refer to this URL. That is, if `vstref` is zero, unode_t 
/// may still be valid (although may be swapped out of memory at any time).
///
/// 2. URL path and search arguments are combined in `unode_t` into a single string,
/// along with the question mark that separates the two. This allows us to generate
/// the same hash values regardless whether the entire combined URL was hashed or
/// its individual components. URL path lebgth withiin the combined URL is identified 
/// by `pathlen`.
///
struct unode_t : public base_node<unode_t> {
      ///
      /// @struct param_block
      ///
      /// @brief  A compound key for URL hash table searches with URL path and 
      ///         the search argumets string being two separate key components.
      ///
      struct param_block : base_node<unode_t>::param_block {
         nodetype_t type;           ///< Regular URL or a group?
         const string_t *url;       ///< URL path
         const string_t *srchargs;  ///< URL search argments
      };

      bool     target : 1;          ///< Target URL?
      u_char   urltype;             ///< URL type (e.g. URL_TYPE_HTTP)
      u_short  pathlen;             ///< URL path length
      uint64_t count;               ///< Requests counter
      uint64_t files;               ///< Files counter 
      uint64_t entry;               ///< Entry page counter
      uint64_t exit;                ///< Exit page counter

      uint64_t vstref;              ///< Visit reference count

      uint64_t xfer;                ///< Transfer size in bytes
      double   avgtime;             ///< Average processing time (seconds)
      double   maxtime;             ///< maximum processing time (seconds)

      public:
         typedef void (*s_unpack_cb_t)(unode_t& unode, void *arg);

      public:
         unode_t(uint64_t nodeid = 0);
         unode_t(const unode_t& unode);
         unode_t(const string_t& urlpath, const string_t& srchargs);

         void reset(uint64_t nodeid = 0);

         u_char update_url_type(u_char type);

         bool match_key_ex(const unode_t::param_block *pb) const;

         virtual bool match_key(const string_t& key) const override 
         {
            // `key` must include the query string, if there is one
            return string == key;
         }

         static uint64_t hash(const string_t& url, const string_t& srchargs) 
         {
            // hash pieces as if the entire URL was hashed
            return (srchargs.isempty()) ? hash_ex(0, url) : hash_ex(hash_byte(hash_ex(0, url), '?'), srchargs);
         }

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

///
/// @class  u_hash_table
///
/// @brief  A hash table containing all URLs for the current month.
///
class u_hash_table : public hash_table<storable_t<unode_t>> {
   public:
      u_hash_table(void) : hash_table<storable_t<unode_t>>(LMAXHASH) {}
};

#endif // UNODE_H
