/*
    webalizer - a web server log analysis program

    Copyright (c) 2004-2021, Stone Steps Inc. (www.stonesteps.ca)

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
/// @brief  URL node
///
/// 1. `vstref` is not a generic reference count, but rather an app-level indicator 
/// of how many visit nodes refer to this URL. That is, if `vstref` is zero, unode_t 
/// may still be valid (although may be swapped out of memory at any time).
///
/// 2. URL path and search arguments are combined in `unode_t` into a single string,
/// along with the question mark that separates the two. This allows us to generate
/// the same hash values regardless whether the entire combined URL was hashed or its 
/// individual components were. URL path lebgth within the combined URL is identified 
/// by `pathlen`.
///
struct unode_t : public base_node<unode_t> {
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
         template <typename ... param_t>
         using s_unpack_cb_t = void (*)(unode_t& unode, param_t ... param);

      public:
         unode_t(uint64_t nodeid = 0);
         unode_t(unode_t&& unode);
         unode_t(const string_t& urlpath, nodetype_t type, const string_t& srchargs);

         void reset(uint64_t nodeid = 0);

         u_char update_url_type(u_char type);

         char get_url_type_ind(void) const;

         // make the single-string match_key visible for full URL look-ups
         using base_node<unode_t>::match_key;

         /// Alternative key matching method that doesn't require concatenating URL components into a single key.
         bool match_key(const string_t& url, const string_t& srchargs) const;

         // make the single-string hash_key visible for hashing a full URL key
         using base_node<unode_t>::hash_key;

         /// Alternative key hashing method that doesn't require concatenating URL components into a single key.
         static uint64_t hash_key(const string_t& url, const string_t& srchargs) 
         {
            // hash pieces as if the entire URL was hashed (must yield same hash as base_node<unode_t>::hash_key)
            return (srchargs.isempty()) ? hash_ex(0, url) : hash_ex(hash_byte(hash_ex(0, url), '?'), srchargs);
         }

         //
         // serialization
         //
         size_t s_data_size(void) const;
         size_t s_pack_data(void *buffer, size_t bufsize) const;

         template <typename ... param_t>
         size_t s_unpack_data(const void *buffer, size_t bufsize, s_unpack_cb_t<param_t ...> upcb, param_t ... param);

         static const void *s_field_value_hash(const void *buffer, size_t bufsize, size_t& datasize);
         static const void *s_field_xfer(const void *buffer, size_t bufsize, size_t& datasize);
         static const void *s_field_hits(const void *buffer, size_t bufsize, size_t& datasize);
         static const void *s_field_entry(const void *buffer, size_t bufsize, size_t& datasize);
         static const void *s_field_exit(const void *buffer, size_t bufsize, size_t& datasize);

         static int64_t s_compare_xfer(const void *buf1, size_t buf1size, const void *buf2, size_t buf2size);
         static int64_t s_compare_hits(const void *buf1, size_t buf1size, const void *buf2, size_t buf2size);
         static int64_t s_compare_entry(const void *buf1, size_t buf1size, const void *buf2, size_t buf2size);
         static int64_t s_compare_exit(const void *buf1, size_t buf1size, const void *buf2, size_t buf2size);
};

///
/// @brief  A hash table containing all URLs for the current month.
///
class u_hash_table : public hash_table<storable_t<unode_t>> {
   public:
      u_hash_table(void) : hash_table<storable_t<unode_t>>(LMAXHASH) {}
};

#endif // UNODE_H
