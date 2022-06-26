/*
   webalizer - a web server log analysis program

   Copyright (c) 2004-2022, Stone Steps Inc. (www.stonesteps.ca)

   See COPYING and Copyright files for additional licensing and copyright information 
   
   asnode.cpp
*/
#include "pch.h"

#include "asnode.h"
#include "serialize.h"

asnode_t::asnode_t(void) :
      keynode_t<uint32_t>(0),
      hits(0), files(0), pages(0), visits(0), xfer(0)
{
}

asnode_t::asnode_t(uint32_t as_num, const string_t& as_org) :
      keynode_t<uint32_t>(as_num),
      as_org(as_org),
      hits(0), files(0), pages(0), visits(0), xfer(0)
{
}

asnode_t::asnode_t(asnode_t&& asnode) noexcept :
      keynode_t<uint32_t>(std::move(asnode)),
      as_org(std::move(asnode.as_org)),
      hits(asnode.hits),
      files(asnode.files),
      pages(asnode.pages),
      xfer(asnode.xfer),
      visits(asnode.visits)
{
}

size_t asnode_t::s_pack_data(void *buffer, size_t bufsize) const
{
   serializer_t sr(buffer, bufsize);

   size_t basesize = datanode_t<asnode_t>::s_pack_data(buffer, bufsize);
   void *ptr = (u_char*) buffer + basesize;

   ptr = sr.serialize(ptr, hits);
   ptr = sr.serialize(ptr, files);
   ptr = sr.serialize(ptr, pages);
   ptr = sr.serialize(ptr, visits);
   ptr = sr.serialize(ptr, xfer);
   ptr = sr.serialize(ptr, as_org);

   return sr.data_size(ptr);
}

template <typename ... param_t>
size_t asnode_t::s_unpack_data(const void *buffer, size_t bufsize, s_unpack_cb_t<param_t ...> upcb, param_t ... param)
{
   serializer_t sr(buffer, bufsize);

   size_t basesize = datanode_t<asnode_t>::s_unpack_data(buffer, bufsize);
   const void *ptr = (u_char*) buffer + basesize;

   u_short version = s_node_ver(buffer);

   ptr = sr.deserialize(ptr, hits);
   ptr = sr.deserialize(ptr, files);
   ptr = sr.deserialize(ptr, pages);
   ptr = sr.deserialize(ptr, visits);
   ptr = sr.deserialize(ptr, xfer);
   ptr = sr.deserialize(ptr, as_org);

   if(upcb)
      upcb(*this, std::forward<param_t>(param) ...);

   return sr.data_size(ptr);
}

size_t asnode_t::s_data_size(void) const
{
   return datanode_t<asnode_t>::s_data_size() + 
            serializer_t::s_size_of(hits) +
            serializer_t::s_size_of(files) +
            serializer_t::s_size_of(pages) +
            serializer_t::s_size_of(visits) +
            serializer_t::s_size_of(xfer) +
            serializer_t::s_size_of(as_org);
}

int64_t asnode_t::s_compare_visits(const void *buf1, size_t buf1size, const void *buf2, size_t buf2size)
{
   return s_compare<uint64_t>(buf1, buf1size, buf2, buf2size);
}

const void *asnode_t::s_field_visits(const void *buffer, size_t bufsize, size_t& datasize)
{
   datasize = serializer_t::s_size_of<decltype(visits)>();
   return (u_char*) buffer + datanode_t<asnode_t>::s_data_size(buffer, bufsize) + 
            serializer_t::s_size_of<decltype(hits)>() +
            serializer_t::s_size_of<decltype(pages)>() + 
            serializer_t::s_size_of<decltype(files)>();
}

as_hash_table::as_hash_table(void) : hash_table<storable_t<asnode_t>>(SMAXHASH) 
{
}

asnode_t& as_hash_table::get_asnode(uint32_t as_num, const string_t& as_org, int64_t tstamp)
{
   uint64_t hashval = asnode_t::hash_key(as_num);
   asnode_t *asnode;

   if((asnode = find_node(hashval, OBJ_REG, tstamp, as_num)) != nullptr)
      return *asnode;

   return *put_node(hashval, new storable_t<asnode_t>(as_num, as_org), tstamp);
}

//
// Instantiate all template callbacks
//
template size_t asnode_t::s_unpack_data(const void *buffer, size_t bufsize, asnode_t::s_unpack_cb_t<> upcb);

#include "hashtab_tmpl.cpp"
