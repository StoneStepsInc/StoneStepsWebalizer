/*
   webalizer - a web server log analysis program

   Copyright (c) 2004-2019, Stone Steps Inc. (www.stonesteps.ca)

   See COPYING and Copyright files for additional licensing and copyright information 
   
   ctnode.cpp
*/
#include "pch.h"

#include "ctnode.h"
#include "serialize.h"

#include <stdexcept>

ctnode_t::ctnode_t(void) :
      keynode_t<uint64_t>(0),
      hits(0), files(0), pages(0), visits(0), xfer(0)
{
}

ctnode_t::ctnode_t(uint32_t geoname_id, const string_t& city, const string_t& ccode) :
      keynode_t<uint64_t>(make_nodeid(geoname_id, ccode.c_str())),
      ccode(!ccode.isempty() ? ccode : "*"), city(city),
      hits(0), files(0), pages(0), visits(0), xfer(0)
{
}

ctnode_t::ctnode_t(ctnode_t&& ctnode) :
      keynode_t<uint64_t>(std::move(ctnode)),
      ccode(std::move(ctnode.ccode)),
      city(std::move(ctnode.city)),
      hits(ctnode.hits),
      files(ctnode.files),
      pages(ctnode.pages),
      xfer(ctnode.xfer),
      visits(ctnode.visits)
{
}

uint64_t ctnode_t::make_nodeid(uint32_t geoname_id, const char *ccode)
{
   // we shouldn't ever have a city without a country, so this should return a zero
   if(!ccode || !*ccode || *ccode == '*')
      return (uint64_t) geoname_id;

   // city names come from GeoIP and domain name suffixes shouldn't appear here
   if(!*(ccode + 1) || *(ccode + 2))
      throw std::logic_error("Country code must be two characters long");

   // there is enough room to just shift characters without additional bit packing (e.g. ccnode_t::ctry_idx)
   return (uint64_t) (u_char) *ccode << 48 | (uint64_t) (u_char) *(ccode + 1) << 32 | geoname_id;
}

///
/// Older city data in the resolver database may contain city names without
/// GeoName IDs, which cannot be stored in the city database because we need
/// all three values to be in sync, with these valid combinations:
///
/// * Zero GeoName ID, empty city name, empty country code
/// * Zero GeoName ID, empty city name, non-empty country code
/// * Non-zero GeoName ID, non-empty city name, non-empty country code
///
/// Everything else is considered unusable for the purposes of storing city
/// information in the database.
///
/// If this method returns `true`, then `ct_hash_table::get_ctnode` will
/// return a usable `ctnode_t` instance.
///
/// Note that the GeoName ID is only associated with the city name and not
/// with the country code, so the latter should not be checked against the
/// GeoName ID.
///
bool ctnode_t::is_usable_city(uint32_t geoname_id, const string_t& city, const string_t& ccode)
{
   // cannot have a city in an unknown country
   if(!city.isempty() && ccode.isempty())
      return false;

   // cannot have empty city name with a non-zero GeoName ID or vice versa
   return !geoname_id == city.isempty();
}

size_t ctnode_t::s_pack_data(void *buffer, size_t bufsize) const
{
   serializer_t sr(buffer, bufsize);

   size_t basesize = datanode_t<ctnode_t>::s_pack_data(buffer, bufsize);
   void *ptr = (u_char*) buffer + basesize;

   ptr = sr.serialize(ptr, hits);
   ptr = sr.serialize(ptr, files);
   ptr = sr.serialize(ptr, pages);
   ptr = sr.serialize(ptr, visits);
   ptr = sr.serialize(ptr, xfer);
   ptr = sr.serialize(ptr, ccode);
   ptr = sr.serialize(ptr, city);

   return sr.data_size(ptr);
}

template <typename ... param_t>
size_t ctnode_t::s_unpack_data(const void *buffer, size_t bufsize, s_unpack_cb_t<param_t ...> upcb, void *arg, param_t&& ... param)
{
   serializer_t sr(buffer, bufsize);

   size_t basesize = datanode_t<ctnode_t>::s_unpack_data(buffer, bufsize);
   const void *ptr = (u_char*) buffer + basesize;

   u_short version = s_node_ver(buffer);

   ptr = sr.deserialize(ptr, hits);
   ptr = sr.deserialize(ptr, files);
   ptr = sr.deserialize(ptr, pages);
   ptr = sr.deserialize(ptr, visits);
   ptr = sr.deserialize(ptr, xfer);
   ptr = sr.deserialize(ptr, ccode);
   ptr = sr.deserialize(ptr, city);

   if(upcb)
      upcb(*this, arg, std::forward<param_t>(param) ...);

   return sr.data_size(ptr);
}

size_t ctnode_t::s_data_size(void) const
{
   return datanode_t<ctnode_t>::s_data_size() + 
            sizeof(uint64_t) * 3 +     // hits, files, pages
            sizeof(uint64_t) +         // visits
            sizeof(uint64_t) +         // xfer
            serializer_t::s_size_of(ccode) +
            serializer_t::s_size_of(city);
}

int64_t ctnode_t::s_compare_visits(const void *buf1, size_t buf1size, const void *buf2, size_t buf2size)
{
   return s_compare<uint64_t>(buf1, buf1size, buf2, buf2size);
}

const void *ctnode_t::s_field_visits(const void *buffer, size_t bufsize, size_t& datasize)
{
   datasize = sizeof(uint64_t);
   return (u_char*) buffer + datanode_t<ctnode_t>::s_data_size(buffer, bufsize) + 
            sizeof(uint64_t) * 3;      // hits, pages, files
}

ct_hash_table::ct_hash_table(void) : hash_table<storable_t<ctnode_t>>(SMAXHASH) 
{
}

///
/// A GeoName ID with the value of zero indicates an unknown city and must have
/// its city name empty.
///
/// An empty country code is allowed, as long as it is accompanied by an empty
/// city name. This entry is maintained under a key with an asterisk, the same
/// way as the country code hash table does, and can be reported as an unknown
/// country with an empty city name to indicate an unknown city.
///
ctnode_t& ct_hash_table::get_ctnode(uint32_t geoname_id, const string_t& city, const string_t& ccode, int64_t tstamp) 
{
   ctnode_t::param_block pb = {geoname_id, ccode.c_str()};
   uint64_t hashval = ctnode_t::get_hash(geoname_id, ccode);
   ctnode_t *ctnode;

   // we should never see empty city names with a valid GeoName and vice versa
   if(!ctnode_t::is_usable_city(geoname_id, city, ccode))
      throw std::logic_error("GeoName ID must match city name in whether it contains data or not");

   if((ctnode = find_node(hashval, &pb, OBJ_REG, tstamp)) != nullptr)
      return *ctnode;

   return *put_node(hashval, new storable_t<ctnode_t>(geoname_id, city, ccode), tstamp);
}

//
// Instantiate all template callbacks
//
template size_t ctnode_t::s_unpack_data(const void *buffer, size_t bufsize, ctnode_t::s_unpack_cb_t<> upcb, void *arg);
