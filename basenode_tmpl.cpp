/*
   webalizer - a web server log analysis program

   Copyright (c) 2004-1013, Stone Steps Inc. (www.stonesteps.ca)

   See COPYING and Copyright files for additional licensing and copyright information     

   basenode_tmpl.cpp
*/
#include "basenode.h"
#include "serialize.h"
#include "util.h"
#include "exception.h"

#include <typeinfo>

template <typename node_t> 
base_node<node_t>::base_node(uint64_t nodeid) : keynode_t<uint64_t>(nodeid) 
{
   flag = OBJ_REG;
}

template <typename node_t> 
base_node<node_t>::base_node(const base_node& node) : keynode_t<uint64_t>(node) 
{
   flag = node.flag; 
   string = node.string;
}

template <typename node_t> 
base_node<node_t>::base_node(const char *str) : keynode_t<uint64_t>(0) 
{
   string = str; 
   flag = OBJ_REG;
}

template <typename node_t> 
base_node<node_t>::base_node(const string_t& str) : keynode_t<uint64_t>(0) 
{
   string = str; 
   flag = OBJ_REG;
}

template <typename node_t>
void base_node<node_t>::reset(uint64_t nodeid)
{
   keynode_t<uint64_t>::reset(nodeid);
   datanode_t<node_t>::reset();
}

//
// serialization
//

template <typename node_t> 
size_t base_node<node_t>::s_data_size(void) const
{
   return datanode_t<node_t>::s_data_size()  + 
            sizeof(u_char)                   +     // flag 
            s_size_of(string);                     // string
}

template <typename node_t> 
size_t base_node<node_t>::s_pack_data(void *buffer, size_t bufsize) const
{
   void *ptr = buffer;
   size_t datasize, basesize;

   basesize = datanode_t<node_t>::s_data_size();
   datasize = s_data_size();

   if(bufsize < datasize)
      return 0;

   datanode_t<node_t>::s_pack_data(buffer, bufsize);
   ptr = (u_char*) buffer + basesize;

   ptr = serialize<u_char>(ptr, flag);
   ptr = serialize(ptr, string);

   return datasize;
}

template <typename node_t> 
size_t base_node<node_t>::s_unpack_data(const void *buffer, size_t bufsize)
{
   const void *ptr = buffer;
   size_t datasize, basesize;

   basesize = datanode_t<node_t>::s_data_size(buffer);
   datasize = s_data_size(buffer);

   if(bufsize < datasize)
      return 0;

   datanode_t<node_t>::s_unpack_data(buffer, bufsize);
   ptr = (u_char*) buffer + basesize;

   ptr = deserialize<u_char>(ptr, flag);
   ptr = deserialize(ptr, string);

   return datasize;
}

template <typename node_t> 
uint64_t base_node<node_t>::s_hash_value(void) const
{
   return hash_str(0, string, string.length());
}

template <typename node_t>
int64_t base_node<node_t>::s_compare_value(const void *buffer, size_t bufsize) const
{
   size_t datasize;
   string_t tstr;

   if(bufsize < s_data_size(buffer))
      throw exception_t(0, string_t::_format("Record size is smaller than expected (node: %s; size: %"PRI_SZ"; expected: %"PRI_SZ")", typeid(*this).name(), bufsize, s_data_size(buffer)));

   deserialize(s_field_value(buffer, bufsize, datasize), tstr);

   return string.compare(tstr);
}

template <typename node_t> 
size_t base_node<node_t>::s_data_size(const void *buffer)
{
   size_t datasize = datanode_t<node_t>::s_data_size(buffer)    +
            sizeof(u_char);                                             // flag
   
   return datasize + 
            s_size_of<string_t>((u_char*) buffer + datasize);           // string
}

template <typename node_t> 
const void *base_node<node_t>::s_field_value(const void *buffer, size_t bufsize, size_t& datasize)
{
   const void *ptr = (u_char*) buffer                          + 
            datanode_t<node_t>::s_data_size(buffer)            + 
            sizeof(u_char);                                             // flag

   datasize = s_size_of<string_t>(ptr);
   return ptr;
}

template <typename node_t> 
int64_t base_node<node_t>::s_compare_value_hash(const void *buf1, const void *buf2)
{
   return s_compare<uint64_t>(buf1, buf2);
}

template <typename node_t> 
bool base_node<node_t>::s_is_group(const void *buffer, size_t bufsize)
{
   if(!buffer && bufsize < s_data_size(buffer))
      return false;

   return (*((u_char*) buffer + datanode_t<node_t>::s_data_size(buffer)) == OBJ_GRP) ? true : false;
}

