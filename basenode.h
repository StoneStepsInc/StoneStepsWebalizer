/*
    webalizer - a web server log analysis program

    Copyright (c) 2004-2014, Stone Steps Inc. (www.stonesteps.ca)

    See COPYING and Copyright files for additional licensing and copyright information

    basenode.h
*/
#ifndef __BASENODE_H
#define __BASENODE_H

#include "tstring.h"
#include "types.h"
#include "hashtab.h"
#include "keynode.h"
#include "datanode.h"

// -----------------------------------------------------------------------
//
// base_node
//
// -----------------------------------------------------------------------
template <typename node_t> 
struct base_node : public htab_node_t<node_t>, public keynode_t<uint64_t>, public datanode_t<node_t> {
      string_t string;              // node value (URL, user agent, etc)
      nodetype_t flag;              // object type (REG, GRP)

      public:
         base_node(uint64_t nodeid = 0);
         base_node(const base_node& node);
         base_node(const char *str);
         base_node(const string_t& str);

         virtual ~base_node(void) {}

         void reset(uint64_t nodeid = 0);

         const string_t& key(void) const {return string;}

         nodetype_t get_type(void) const {return flag;}

         //
         // serialization
         //
         //    key  : nodeid (uint64_t)
         //    data : flag (u_char), string (string_t)
         //    value: hash(string) (uint64_t)
         //
         size_t s_data_size(void) const;

         size_t s_pack_data(void *buffer, size_t bufsize) const;
         size_t s_unpack_data(const void *buffer, size_t bufsize);

         uint64_t s_hash_value(void) const;

         int64_t s_compare_value(const void *buffer, size_t bufsize) const;

         static size_t s_data_size(const void *buffer);

         static const void *s_field_value(const void *buffer, size_t bufsize, size_t& datasize);

         static int64_t s_compare_value_hash(const void *buf1, const void *buf2);

         static bool s_is_group(const void *buffer, size_t bufsize);
};

#endif // __BASENODE_H
