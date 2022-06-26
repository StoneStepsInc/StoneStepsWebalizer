/*
   webalizer - a web server log analysis program

   Copyright (c) 2004-2022, Stone Steps Inc. (www.stonesteps.ca)

   See COPYING and Copyright files for additional licensing and copyright information     

   basenode_tmpl.cpp
*/
#include "basenode.h"
#include "serialize.h"
#include "exception.h"

#include <typeinfo>
#include <stdexcept>

template <typename node_t> 
base_node<node_t>::base_node(uint64_t nodeid) : keynode_t<uint64_t>(nodeid) 
{
   flag = OBJ_REG;
}

template <typename node_t> 
base_node<node_t>::base_node(base_node&& node) noexcept :
      keynode_t<uint64_t>(std::move(node)), flag(node.flag), string(std::move(node.string))
{
}

template <typename node_t> 
base_node<node_t>::base_node(const string_t& str, nodetype_t type) :
      keynode_t<uint64_t>(0), flag(type), string(str) 
{
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
            serializer_t::s_size_of(string);       // string
}

template <typename node_t> 
size_t base_node<node_t>::s_pack_data(void *buffer, size_t bufsize) const
{
   serializer_t sr(buffer, bufsize);

   size_t basesize = datanode_t<node_t>::s_pack_data(buffer, bufsize);
   void *ptr = (u_char*) buffer + basesize;

   ptr = sr.serialize<u_char, nodetype_t>(ptr, flag);
   ptr = sr.serialize(ptr, string);

   return sr.data_size(ptr);
}

template <typename node_t> 
size_t base_node<node_t>::s_unpack_data(const void *buffer, size_t bufsize)
{
   serializer_t sr(buffer, bufsize);

   size_t basesize = datanode_t<node_t>::s_unpack_data(buffer, bufsize);

   const void *ptr = (u_char*) buffer + basesize;

   ptr = sr.deserialize<u_char>(ptr, flag);
   ptr = sr.deserialize(ptr, string);

   return sr.data_size(ptr);
}

template <typename node_t> 
uint64_t base_node<node_t>::s_hash_value(void) const
{
   return hash_str(0, string, string.length());
}

///
/// This method is called by the database to skip node values that produce
/// the same database hash returned from `s_hash_value` and used as a key
/// in value tables.
/// 
/// The `s_hash_value` method does not include node type in the hash, which
/// means that we need to include node type in value comparisons in this
/// method. This is done for historical reasons, because adding node type
/// into the hashing method would require node versioning propagated into
/// all methods involved in value lookups, which would be a more intrusive
/// change.
/// 
template <typename node_t>
int64_t base_node<node_t>::s_compare_value(const void *buffer, size_t bufsize) const
{
   serializer_t sr(buffer, bufsize);

   size_t datasize;
   string_t tstr;

   if(bufsize < s_data_size(buffer, bufsize))
      throw std::runtime_error(string_t::_format("Record size is smaller than expected (node: %s; size: %zu; expected: %zu)", typeid(*this).name(), bufsize, s_data_size(buffer, bufsize)));

   // make sure node types match, as group names may be equal to regular item values
   bool is_group = s_is_group(buffer, bufsize);

   // order regular items before groups, in case if this method is used for sorting
   if(is_group != (flag == OBJ_GRP))
      return is_group ? 1 : -1;

   sr.deserialize(s_field_value(buffer, bufsize, datasize), tstr);

   return string.compare(tstr);
}

template <typename node_t> 
size_t base_node<node_t>::s_data_size(const void *buffer, size_t bufsize)
{
   size_t datasize = datanode_t<node_t>::s_data_size(buffer, bufsize)    +
            sizeof(u_char);                                             // flag

   serializer_t sr(buffer, bufsize);

   const void *ptr = sr.s_skip_field<string_t>((u_char*) buffer + datasize);  // string
   
   return sr.data_size(ptr);
}

template <typename node_t> 
const void *base_node<node_t>::s_field_value(const void *buffer, size_t bufsize, size_t& datasize)
{
   const void *ptr = (u_char*) buffer                          + 
            datanode_t<node_t>::s_data_size(buffer, bufsize)   + 
            sizeof(u_char);                                             // flag

   serializer_t sr(buffer, bufsize);

   datasize = sr.s_size_of<string_t>(ptr);

   return ptr;
}

template <typename node_t> 
int64_t base_node<node_t>::s_compare_value_hash(const void *buf1, size_t buf1size, const void *buf2, size_t buf2size)
{
   return s_compare<uint64_t>(buf1, buf1size, buf2, buf2size);
}

template <typename node_t> 
bool base_node<node_t>::s_is_group(const void *buffer, size_t bufsize)
{
   if(!buffer && bufsize < s_data_size(buffer, bufsize))
      throw std::runtime_error(string_t::_format("Record size is smaller than expected (node: %s; size: %zu; expected: %zu)", typeid(node_t).name(), bufsize, s_data_size(buffer, bufsize)));

   // node type is the first serialized value for this node, so just compare it in-place
   return (*((u_char*) buffer + datanode_t<node_t>::s_data_size(buffer, bufsize)) == (u_char) OBJ_GRP);
}

