/*
    webalizer - a web server log analysis program

    Copyright (c) 2004-2021, Stone Steps Inc. (www.stonesteps.ca)

    See COPYING and Copyright files for additional licensing and copyright information

    keynode.cpp
*/
#include "pch.h"

#include "keynode.h"
#include "serialize.h"

template <typename type_t>
keynode_t<type_t>::keynode_t(type_t nodeid) : nodeid(nodeid)
{
}

template <typename type_t>
keynode_t<type_t>::keynode_t(keynode_t&& keynode) noexcept : nodeid(keynode.nodeid)
{
   keynode.nodeid = 0;
}

template <typename type_t>
keynode_t<type_t>::keynode_t(const keynode_t& keynode) : nodeid(keynode.nodeid)
{
}

template <typename type_t>
void keynode_t<type_t>::reset(type_t _nodeid)
{
   nodeid = _nodeid;
}

//
// serialization
//

template <typename type_t>
size_t keynode_t<type_t>::s_key_size(void) const
{
   return serializer_t::s_size_of(nodeid);
}

template <typename type_t>
size_t keynode_t<type_t>::s_pack_key(void *buffer, size_t bufsize) const
{
   serializer_t sr(buffer, bufsize);

   void *ptr = sr.serialize(buffer, nodeid);

   return sr.data_size(ptr);
}

template <typename type_t>
size_t keynode_t<type_t>::s_unpack_key(const void *buffer, size_t bufsize)
{
   serializer_t sr(buffer, bufsize);

   const void *ptr = sr.deserialize(buffer, nodeid);

   return sr.data_size(ptr);
}

template <typename type_t>
int64_t keynode_t<type_t>::s_compare_key(const void *buf1, size_t buf1size, const void *buf2, size_t buf2size)
{
   return s_compare<type_t>(buf1, buf1size, buf2, buf2size);
}

//
// instantiate key nodes of known types
//
template struct keynode_t<uint64_t>;
template struct keynode_t<u_int>;


