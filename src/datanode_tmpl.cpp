/*
   webalizer - a web server log analysis program

   Copyright (c) 2004-2019, Stone Steps Inc. (www.stonesteps.ca)

   See COPYING and Copyright files for additional licensing and copyright information     

   datanode_tmpl.cpp
*/

#include "datanode.h"
#include "serialize.h"

template <typename node_t>
datanode_t<node_t>::datanode_t(void) 
{
}

template <typename node_t>
void datanode_t<node_t>::reset(void)
{
}

//
// serialization
//

template <typename node_t> 
size_t datanode_t<node_t>::s_data_size(void) const
{
   return sizeof(__version);
}

template <typename node_t> 
size_t datanode_t<node_t>::s_pack_data(void *buffer, size_t bufsize) const
{
   serializer_t sr(buffer, bufsize);

   const void *ptr = sr.serialize(buffer, __version);

   return sr.data_size(ptr);
}

template <typename node_t> 
size_t datanode_t<node_t>::s_unpack_data(const void *buffer, size_t bufsize)
{
   serializer_t sr(buffer, bufsize);

   // just make sure we have enough bytes for a version in the buffer, so s_node_ver can read it safely
   return sr.data_size(sr.s_skip_field<u_short>(buffer));
}

template <typename node_t> 
size_t datanode_t<node_t>::s_data_size(const void *buffer, size_t bufsize)
{
   serializer_t sr(buffer, bufsize);

   return sr.data_size(sr.s_skip_field<u_short>(buffer));
}

template <typename node_t>
u_short datanode_t<node_t>::s_node_ver(const void *buffer)
{
   u_short nodever = 0;

   serializer_t sr(buffer, serializer_t::s_size_of(nodever));

   sr.deserialize(buffer, nodever);

   return nodever;
}
