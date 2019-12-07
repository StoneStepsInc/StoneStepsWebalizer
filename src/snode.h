/*
    webalizer - a web server log analysis program

    Copyright (c) 2004-2019, Stone Steps Inc. (www.stonesteps.ca)

    See COPYING and Copyright files for additional licensing and copyright information

    snode.h
*/
#ifndef SNODE_H
#define SNODE_H

#include "hashtab.h"
#include "basenode.h"
#include "types.h"
#include "storable.h"

///
/// @brief  Search engine search terms node
///
/// 1. Search strings are length-encoded and include the type of the search and 
/// search terms. For example, a search for phrase "webalizer css" and any type
/// of file (e.g. HTML, PDF, etc) would look like this:
/// ```
///    [6]Phrase[13]webalizer css[9]File Type[3]any
/// ```
/// Type length is always present and may be zero to indicate that the type of
/// the search term is either missing or unknown.
///
struct snode_t : public base_node<snode_t> {     
      u_short        termcnt;          ///< Search term count
      uint64_t       count;            ///< Request count
      uint64_t       visits;           ///< Visits started

      public:
         template <typename ... param_t>
         using s_unpack_cb_t = void (*)(snode_t& snode, param_t ... param);

      public:
         snode_t(void) : base_node<snode_t>() {count = 0; termcnt = 0; visits = 0;}
         snode_t(const string_t& srch) : base_node<snode_t>(srch) {count = 0; termcnt = 0; visits = 0;}

         //
         // serialization
         //
         size_t s_data_size(void) const;
         size_t s_pack_data(void *buffer, size_t bufsize) const;

         template <typename ... param_t>
         size_t s_unpack_data(const void *buffer, size_t bufsize, s_unpack_cb_t<param_t ...> upcb, param_t ... param);

         static const void *s_field_value_hash(const void *buffer, size_t bufsize, size_t& datasize);
         static const void *s_field_hits(const void *buffer, size_t bufsize, size_t& datasize);

         static int64_t s_compare_hits(const void *buf1, size_t buf1size, const void *buf2, size_t buf2size);
};

//
// Search Strings
//
class s_hash_table : public hash_table<storable_t<snode_t>> {
};

#endif // SNODE_H
