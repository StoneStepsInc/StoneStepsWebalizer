/*
    webalizer - a web server log analysis program

    Copyright (c) 2004-2019, Stone Steps Inc. (www.stonesteps.ca)

    See COPYING and Copyright files for additional licensing and copyright information

    rcnode.h
*/
#ifndef RCNODE_H
#define RCNODE_H

#include "basenode.h"
#include "types.h"
#include "storable.h"

///
/// @struct rcnode_t
///
/// @brief  URL request method and status code for HTTP errors node
/// 
/// 1. `rcnode_t` tracks URLs that resulted in an HTTP error
///
struct rcnode_t : public base_node<rcnode_t> { 
      ///
      /// @struct param_block
      ///
      /// @brief  A compound key structure for an HTP response code node.
      ///
      struct param_block {
         u_int       respcode;         ///< HTTP status code
         const char  *url;             ///< Request URL
         const char  *method;          ///< HTTP method
      };

      u_short        respcode;         ///< HTTP status code
      uint64_t       count;            ///< Request count
      string_t       method;           ///< HTTP method

      public:
         template <typename ... param_t>
         using s_unpack_cb_t = void (*)(rcnode_t& rcnode, void *arg, param_t ... param);

      public:
         rcnode_t(void) : base_node<rcnode_t>() {count = 0; respcode = 0;}
         rcnode_t(const string_t& method, const string_t& url, u_short respcode);

         bool match_key_ex(const rcnode_t::param_block *pb) const;

         virtual bool match_key(const string_t& key) const override 
         {
            throw std::logic_error("This node only supports searches with a compound key");
         }

         static uint64_t hash(u_short respcode, const string_t& method, const string_t& url) 
         {
            return hash_ex(hash_ex(hash_num(0, respcode), method), url);
         }

         virtual uint64_t get_hash(void) const override 
         {
            return hash_ex(hash_ex(hash_num(0, respcode), method), string);
         }

         //
         // serialization
         //
         size_t s_data_size(void) const;
         size_t s_pack_data(void *buffer, size_t bufsize) const;

         template <typename ... param_t>
         size_t s_unpack_data(const void *buffer, size_t bufsize, s_unpack_cb_t<param_t ...> upcb, void *arg, param_t&& ... param);

         uint64_t s_hash_value(void) const;

         int64_t s_compare_value(const void *buffer, size_t bufsize) const;

         static const void *s_field_value_hash(const void *buffer, size_t bufsize, size_t& datasize);
         static const void *s_field_hits(const void *buffer, size_t bufsize, size_t& datasize);
         static const void *s_field_value_mp_url(const void *buffer, size_t bufsize, size_t& datasize);
         static const void *s_field_value_mp_method(const void *buffer, size_t bufsize, size_t& datasize);
         static const void *s_field_value_mp_respcode(const void *buffer, size_t bufsize, size_t& datasize);

         static int64_t s_compare_hits(const void *buf1, const void *buf2);
};

//
// HTTP status codes
//
class rc_hash_table : public hash_table<storable_t<rcnode_t>> {
   public:
      rc_hash_table(void) : hash_table<storable_t<rcnode_t>>(SMAXHASH) {}
};

#endif // RCNODE_H
