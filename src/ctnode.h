/*
    webalizer - a web server log analysis program

    Copyright (c) 2004-2022, Stone Steps Inc. (www.stonesteps.ca)

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
/// @brief  A city node
///
/// Each city is identified by a GeoName identifier, as reported by the GeoIP library,
/// which in turn is derived it from this database:
///
///   http://www.geonames.org/
///
/// Each `ctnode_t` is identified by a 64-bit value that is made up of a city GeoName
/// ID stored in the lower 32 bits and country code characters shifted to the upper
/// 32 bits. This allows us to identify unknown cities within their countries when the
/// city is not found in the GeoIP database and its GeoName ID is zero.
///
/// There are no latitude and longitude available for cities in the GeoIP databases
/// at this point, only for their locations within cities, even though it is listed
/// on the GeoNames site. If these coordinates are added later to GeoIP databases or
/// if more precise databases may list city coordinates, latitude and longitude may
/// be added to `ctnode_t`.
///
struct ctnode_t : htab_obj_t<uint32_t, const string_t&>, keynode_t<uint64_t>, datanode_t<ctnode_t> {
   string_t    ccode;                  ///< Country code
   string_t    city;                   ///< A localized city name, as reported by GeoIP
                                       ///< for the current language.

   uint64_t    hits;                   ///< Request count
   uint64_t    files;                  ///< Files requested
   uint64_t    pages;                  ///< Pages requested
   uint64_t    visits;                 ///< Visits started
   uint64_t    xfer;                   ///< Transfer amount in bytes

   public:
      template <typename ... param_t>
      using s_unpack_cb_t = void (*)(ctnode_t& ctnode, param_t ... param);

   private:
      /// Combines a 32-bit GeoName ID and a country code to form a unique 64-bit `ctnode_t` identifier.
      static uint64_t make_nodeid(uint32_t geoname_id, const char *ccode);      

   public:
      ctnode_t(void);

      ctnode_t(uint32_t geoname_id, const string_t& city, const string_t& ccode);

      ctnode_t(ctnode_t&& ctnode) noexcept;

      /// Returns the GeoName ID for this city.
      uint32_t geoname_id(void) const {return (uint32_t) nodeid;}

      /// Returns `nodeid` packed into `42` least-significant bits.
      uint64_t compact_nodeid(void) const;

      /// Indicates whether this city was found in the GeoIP database or not.
      bool unknown_city(void) const {return geoname_id() == 0;}

      /// Returns a hash value for a GeoName ID and a country code.
      static uint64_t hash_key(uint32_t geoname_id, const string_t& ccode)
      {
         return hash_num(0, make_nodeid(geoname_id, ccode));
      }

      /// Returns `true` for usable combinations of GeoName ID, city name and country code, `false` otherwise.
      static bool is_usable_city(uint32_t geoname_id, const string_t& city, const string_t& ccode);

      ///
      /// @name   Hash table interface
      ///
      /// @{

      bool match_key(uint32_t geoname_id, const string_t& ccode) const override
      {
         return make_nodeid(geoname_id, ccode) == nodeid;
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
/// @brief  A hash table to store city nodes.
///
class ct_hash_table : public hash_table<storable_t<ctnode_t>> {
   public:
      ct_hash_table(void);

      /// Looks up a city node by a GeoName ID and inserts a new node if none is found.
      ctnode_t& get_ctnode(uint32_t geoname_id, const string_t& city, const string_t& ccode, int64_t tstamp);
};

#endif // CTNODE_H
