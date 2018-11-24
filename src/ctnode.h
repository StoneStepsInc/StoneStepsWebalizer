/*
    webalizer - a web server log analysis program

    Copyright (c) 2004-2018, Stone Steps Inc. (www.stonesteps.ca)

    See COPYING and Copyright files for additional licensing and copyright information

    ctnode.h
*/
#ifndef CTNODE_H
#define CTNODE_H

#include "hashtab.h"
#include "keynode.h"
#include "datanode.h"
#include "types.h"

///
/// @struct ctnode_t
///
/// @brief  A city node
///
/// Each city is identified by a geoname identifier, as reported by the GeoIP library
/// in the `geoname_id` field, which in turn is derived it from this database:
///
///   http://www.geonames.org/
///
/// There are no latitude and longitude available for cities in the GeoIP databases
/// at this point, only for their locations within cities, even though it is listed
/// on the GeoNames site. If these coordinates are added later to GeoIP databases or
/// if more precise databases may list city coordinates, latitude and longitude may
/// be added to `ctnode_t`.
///
struct ctnode_t : htab_obj_t, keynode_t<uint32_t>, datanode_t<ctnode_t> {
   string_t    city;                   ///< A localized city name, as reported by GeoIP
                                       ///< for the current language.

   uint64_t    hits;                   ///< Request count
   uint64_t    files;                  ///< Files requested
   uint64_t    pages;                  ///< Pages requested
   uint64_t    visits;                 ///< Visits started
   uint64_t    xfer;                   ///< Transfer amount in bytes

   public:
      template <typename ... param_t>
      using s_unpack_cb_t = void (*)(ctnode_t& vnode, void *arg, param_t ... param);

   public:
      ctnode_t(void);

      ctnode_t(uint32_t geoname_id, const string_t& city);

      ctnode_t(ctnode_t&& ctnode);

      /// Indicates whether this city was found in the GeoIP database or not.
      bool unknown(void) const {return !city.isempty() && *city == '*';}

      ///
      /// @name   Hash table interface
      ///
      /// @{

      bool match_key(const string_t& key) const override {return city == key;}

      nodetype_t get_type(void) const override {return OBJ_REG;}

      virtual uint64_t get_hash(void) const override {return hash_ex(0, city);}

      /// @}

      ///
      /// @name   Serialization
      ///
      /// @{

      size_t s_data_size(void) const;
      size_t s_pack_data(void *buffer, size_t bufsize) const;

      template <typename ... param_t>
      size_t s_unpack_data(const void *buffer, size_t bufsize, s_unpack_cb_t<param_t ...> upcb, void *arg, param_t&& ... param);

      static size_t s_data_size(const void *buffer);

      static int64_t s_compare_visits(const void *buf1, const void *buf2);

      static const void *s_field_visits(const void *buffer, size_t bufsize, size_t& datasize);
      /// @}
};

///
/// @brief  A hash table to store city nodes.
///
class ct_hash_table : public hash_table<storable_t<ctnode_t>> {

   public:
      ct_hash_table(void);

      ctnode_t& get_ctnode(uint32_t geoname_id, const string_t& city, int64_t tstamp);
};

#endif // CTNODE_H
