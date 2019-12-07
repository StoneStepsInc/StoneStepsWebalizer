/*
    webalizer - a web server log analysis program

    Copyright (c) 2004-2019, Stone Steps Inc. (www.stonesteps.ca)

    See COPYING and Copyright files for additional licensing and copyright information

    rcnode.h
*/
#ifndef RCNODE_H
#define RCNODE_H

#include "hashtab.h"
#include "keynode.h"
#include "datanode.h"
#include "types.h"
#include "storable.h"

///
/// @brief  URL request method and status code for HTTP errors node
/// 
/// 1. `rcnode_t` tracks URLs that resulted in an HTTP error
///
struct rcnode_t : public htab_obj_t<u_short, const string_t&, const string_t&>, public keynode_t<uint64_t>, public datanode_t<rcnode_t> { 
      string_t       url;              ///< Requested URL.
      u_short        respcode;         ///< HTTP status code
      uint64_t       count;            ///< Request count
      string_t       method;           ///< HTTP method

      public:
         template <typename ... param_t>
         using s_unpack_cb_t = void (*)(rcnode_t& rcnode, param_t ... param);

      public:
         rcnode_t(void);
         rcnode_t(const string_t& method, const string_t& url, u_short respcode);

         nodetype_t get_type(void) const override {return OBJ_REG;}

         bool match_key(u_short respcode, const string_t& method, const string_t& url) const;

         static uint64_t hash_key(u_short respcode, const string_t& method, const string_t& url) 
         {
            return hash_ex(hash_ex(hash_num(0, respcode), method), url);
         }

         virtual uint64_t get_hash(void) const override 
         {
            return hash_ex(hash_ex(hash_num(0, respcode), method), url);
         }

         //
         // serialization
         //
         size_t s_data_size(void) const;
         size_t s_pack_data(void *buffer, size_t bufsize) const;

         template <typename ... param_t>
         size_t s_unpack_data(const void *buffer, size_t bufsize, s_unpack_cb_t<param_t ...> upcb, param_t ... param);

         uint64_t s_hash_value(void) const;
         size_t s_hash_value_size(void) const {return sizeof(uint64_t);}

         int64_t s_compare_value(const void *buffer, size_t bufsize) const;

         static const void *s_field_value_hash(const void *buffer, size_t bufsize, size_t& datasize);
         static const void *s_field_hits(const void *buffer, size_t bufsize, size_t& datasize);

         static int64_t s_compare_value_hash(const void *buf1, size_t buf1size, const void *buf2, size_t buf2size);
         static int64_t s_compare_hits(const void *buf1, size_t buf1size, const void *buf2, size_t buf2size);
};

//
// HTTP status codes
//
class rc_hash_table : public hash_table<storable_t<rcnode_t>> {
   public:
      rc_hash_table(void) : hash_table<storable_t<rcnode_t>>(SMAXHASH) {}
};

#endif // RCNODE_H
