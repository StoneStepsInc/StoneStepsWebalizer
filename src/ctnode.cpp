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

size_t ctnode_t::s_pack_data(void *buffer, size_t bufsize) const
{
   size_t datasize, basesize;
   void *ptr = buffer;

   basesize = datanode_t<ctnode_t>::s_data_size();
   datasize = s_data_size();

   if(bufsize < s_data_size())
      return 0;

   datanode_t<ctnode_t>::s_pack_data(buffer, bufsize);
   ptr = (u_char*) buffer + basesize;

   ptr = serialize(ptr, hits);
   ptr = serialize(ptr, files);
   ptr = serialize(ptr, pages);
   ptr = serialize(ptr, visits);
   ptr = serialize(ptr, xfer);
   ptr = serialize(ptr, ccode);
         serialize(ptr, city);

   return datasize;
}

template <typename ... param_t>
size_t ctnode_t::s_unpack_data(const void *buffer, size_t bufsize, s_unpack_cb_t<param_t ...> upcb, void *arg, param_t&& ... param)
{
   u_short version;
   size_t datasize, basesize;
   const void *ptr;

   basesize = datanode_t<ctnode_t>::s_data_size();
   datasize = s_data_size(buffer);

   if(bufsize < datasize)
      return 0;

   version = s_node_ver(buffer);

   datanode_t<ctnode_t>::s_unpack_data(buffer, bufsize);
   ptr = (u_char*) buffer + basesize;

   ptr = deserialize(ptr, hits);
   ptr = deserialize(ptr, files);
   ptr = deserialize(ptr, pages);
   ptr = deserialize(ptr, visits);
   ptr = deserialize(ptr, xfer);
   ptr = deserialize(ptr, ccode);
         deserialize(ptr, city);

   if(upcb)
      upcb(*this, arg, std::forward<param_t>(param) ...);

   return datasize;
}

size_t ctnode_t::s_data_size(void) const
{
   return datanode_t<ctnode_t>::s_data_size() + 
            sizeof(uint64_t) * 3 +     // hits, files, pages
            sizeof(uint64_t) +         // visits
            sizeof(uint64_t) +         // xfer
            s_size_of(ccode) +
            s_size_of(city);
}

size_t ctnode_t::s_data_size(const void *buffer)
{
   u_short version = s_node_ver(buffer);
   size_t datasize = datanode_t<ctnode_t>::s_data_size(buffer) + 
            sizeof(uint64_t) * 3 +     // hits, files, pages
            sizeof(uint64_t) +         // visits
            sizeof(uint64_t);          // xfer
   
   datasize += s_size_of<string_t>((u_char*) buffer + datasize);  // ccode
   datasize += s_size_of<string_t>((u_char*) buffer + datasize);  // city

   return datasize;
}

int64_t ctnode_t::s_compare_visits(const void *buf1, const void *buf2)
{
   return s_compare<uint64_t>(buf1, buf2);
}

const void *ctnode_t::s_field_visits(const void *buffer, size_t bufsize, size_t& datasize)
{
   datasize = sizeof(uint64_t);
   return (u_char*) buffer + datanode_t<ctnode_t>::s_data_size(buffer) + 
            sizeof(uint64_t) * 3;      // hits, pages, files
}

ct_hash_table::ct_hash_table(void) : hash_table<storable_t<ctnode_t>>(SMAXHASH) 
{
}

///
/// A GeoName ID with the value of zero indicates an unknown city and must have
/// its city name empty. Use `ctnode_t::unknown` to localize this city name in
/// the reports.
///
ctnode_t& ct_hash_table::get_ctnode(uint32_t geoname_id, const string_t& city, const string_t& ccode, int64_t tstamp) 
{
   ctnode_t::param_block pb = {geoname_id, ccode.c_str()};
   uint64_t hashval = ctnode_t::get_hash(geoname_id, ccode);
   ctnode_t *ctnode;

   // we should never see empty city names with a valid GeoName and vice versa
   if(!geoname_id != city.isempty())
      throw std::logic_error("GeoName ID must match city name in whether it contains data or not");

   if((ctnode = find_node(hashval, &pb, OBJ_REG, tstamp)) != nullptr)
      return *ctnode;

   return *put_node(hashval, new storable_t<ctnode_t>(geoname_id, city, ccode), tstamp);
}

//
// Instantiate all template callbacks
//
template size_t ctnode_t::s_unpack_data(const void *buffer, size_t bufsize, ctnode_t::s_unpack_cb_t<> upcb, void *arg);
