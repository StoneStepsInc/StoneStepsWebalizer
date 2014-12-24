/*
    webalizer - a web server log analysis program

    Copyright (c) 2004-2014, Stone Steps Inc. (www.stonesteps.ca)

    See COPYING and Copyright files for additional licensing and copyright information

    rcnode.h
*/
#ifndef __RCNODE_H
#define __RCNODE_H

#include "basenode.h"
#include "types.h"

//
// HTTP status code node
//
struct rcnode_t : public base_node<rcnode_t> { 
      bool           hexenc;           // any %xx sequences?
      u_short        respcode;         // HTTP status code
      uint64_t         count;
      string_t       method;           // HTTP method

      public:
         typedef void (*s_unpack_cb_t)(rcnode_t& rcnode, void *arg);

      public:
         rcnode_t(void) : base_node<rcnode_t>() {count = 0; hexenc = false; respcode = 0;}
         rcnode_t(const rcnode_t& rcnode);
         rcnode_t(const char *method, const char *url, u_int respcode);

         //
         // serialization
         //
         size_t s_data_size(void) const;
         size_t s_pack_data(void *buffer, size_t bufsize) const;
         size_t s_unpack_data(const void *buffer, size_t bufsize, s_unpack_cb_t upcb, void *arg);

         uint64_t s_hash_value(void) const;

         int64_t s_compare_value(const void *buffer, size_t bufsize) const;

         static size_t s_data_size(const void *buffer);

         static const void *s_field_value_hash(const void *buffer, size_t bufsize, size_t& datasize);
         static const void *s_field_hits(const void *buffer, size_t bufsize, size_t& datasize);
         static const void *s_field_value_mp_url(const void *buffer, size_t bufsize, size_t& datasize);
         static const void *s_field_value_mp_method(const void *buffer, size_t bufsize, size_t& datasize);
         static const void *s_field_value_mp_respcode(const void *buffer, size_t bufsize, size_t& datasize);

         static int64_t s_mp_compare_value(const void *buf1, const void *buf2, u_int partid, bool& lastpart);
         static int64_t s_compare_hits(const void *buf1, const void *buf2);
};

//
// HTTP status codes
//
class rc_hash_table : public hash_table<rcnode_t> {
   public:
      struct param_block {
         u_int       respcode;
         const char  *url;
         const char  *method;
      };

   private:
      virtual bool compare(const rcnode_t *nptr, const void *param) const;

   public:
      rc_hash_table(void) : hash_table<rcnode_t>(SMAXHASH) {}
};

#endif // __RCNODE_H
