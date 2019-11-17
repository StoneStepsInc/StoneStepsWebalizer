/*
    webalizer - a web server log analysis program

    Copyright (c) 2004-2019, Stone Steps Inc. (www.stonesteps.ca)

    See COPYING and Copyright files for additional licensing and copyright information

    asnode.h
*/
#ifndef ASNODE_H
#define ASNODE_H

#include "hashtab.h"
#include "keynode.h"
#include "datanode.h"
#include "types.h"

///
/// @brief  An autonomous system node.
///
/// ASN nodes are identified by Autonomous System numbers, which are 32-bit
/// integers. This number is used as a node key. The name of the organization
/// associated with each AS number is stored as data and is never looked up.
///
struct asnode_t : htab_obj_t, keynode_t<uint32_t>, datanode_t<asnode_t> {
   ///
   /// @brief  An autonomous system number used as a hash table key.
   ///
   struct param_block {
      uint32_t    as_num;              ///< A city GeoName ID
   };

   string_t    as_org;                 ///< Autonomous system organization name.

   uint64_t    hits;                   ///< Request count
   uint64_t    files;                  ///< Files requested
   uint64_t    pages;                  ///< Pages requested
   uint64_t    visits;                 ///< Visits started
   uint64_t    xfer;                   ///< Transfer amount in bytes

   public:
      template <typename ... param_t>
      using s_unpack_cb_t = void (*)(asnode_t& asnode, param_t ... param);

    public:
      asnode_t(void);

      asnode_t(uint32_t as_num, const string_t& as_org);

      asnode_t(asnode_t&& ctnode);

      ///
      /// @name   Hash table interface
      ///
      /// @{

      bool match_key_ex(const asnode_t::param_block *pb) const 
      {
         return pb && pb->as_num == nodeid;
      }

      virtual bool match_key(const string_t& key) const override
      {
         throw std::logic_error("This node only supports searches with a numeric key");
      }

      nodetype_t get_type(void) const override {return OBJ_REG;}

      virtual uint64_t get_hash(void) const override
      {
         return hash_num(0, nodeid);
      }
      /// @}

      ///
      /// @name   Serialization
      ///
      /// @{

      size_t s_data_size(void) const;
      size_t s_pack_data(void *buffer, size_t bufsize) const;

      template <typename ... param_t>
      size_t s_unpack_data(const void *buffer, size_t bufsize, s_unpack_cb_t<param_t ...> upcb, param_t ... param);

      static int64_t s_compare_visits(const void *buf1, size_t buf1size, const void *buf2, size_t buf2size);

      static const void *s_field_visits(const void *buffer, size_t bufsize, size_t& datasize);
      /// @}
};

///
/// @brief  A hash table to store autonomous system nodes.
///
class as_hash_table : public hash_table<storable_t<asnode_t>> {
   public:
      as_hash_table(void);

      asnode_t& get_asnode(uint32_t as_num, const string_t& as_org, int64_t tstamp);
};

#endif // ASNODE_H
