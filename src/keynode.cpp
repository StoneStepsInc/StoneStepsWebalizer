/*
    webalizer - a web server log analysis program

    Copyright (c) 2004-2017, Stone Steps Inc. (www.stonesteps.ca)

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
   return s_size_of(nodeid);
}

template <typename type_t>
size_t keynode_t<type_t>::s_pack_key(void *buffer, size_t bufsize) const
{
   size_t datasize = s_key_size();

   if(bufsize < datasize)
      return 0;

   serialize(buffer, nodeid);

   return datasize;
}

template <typename type_t>
size_t keynode_t<type_t>::s_unpack_key(const void *buffer, size_t bufsize)
{
   size_t datasize = s_key_size();

   if(bufsize < datasize)
      return 0;

   deserialize(buffer, nodeid);

   return datasize;
}

template <typename type_t>
size_t keynode_t<type_t>::s_key_size(const void *buffer)
{
   return s_size_of<type_t>(buffer);
}

template <typename type_t>
int64_t keynode_t<type_t>::s_compare_key(const void *buf1, const void *buf2)
{
   return s_compare<type_t>(buf1, buf2);
}

//
// instantiate key nodes of known types
//
template struct keynode_t<uint64_t>;
template struct keynode_t<u_int>;


