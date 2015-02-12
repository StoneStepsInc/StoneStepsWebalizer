/*
   webalizer - a web server log analysis program

   Copyright (c) 2004-1013, Stone Steps Inc. (www.stonesteps.ca)

   See COPYING and Copyright files for additional licensing and copyright information     

   datanode_tmpl.cpp
*/

#include "datanode.h"
#include "serialize.h"

template <typename node_t>
datanode_t<node_t>::datanode_t(void) 
{
   dirty = false;
   storage = false;
}

template <typename node_t>
void datanode_t<node_t>::reset(void)
{
   dirty = false;
   storage = false;
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
   if(bufsize < s_data_size())
      return 0;

   serialize(buffer, __version);

   return s_data_size();
}

template <typename node_t> 
size_t datanode_t<node_t>::s_unpack_data(const void *buffer, size_t bufsize)
{
   if(bufsize < s_data_size(buffer))
      return 0;

   return s_data_size(buffer);
}

template <typename node_t> 
size_t datanode_t<node_t>::s_data_size(const void *buffer)
{
   return sizeof(__version);
}

template <typename node_t>
u_short datanode_t<node_t>::s_node_ver(const void *buffer)
{
   u_short nodever;
   deserialize(buffer, nodever);
   return nodever;
}
