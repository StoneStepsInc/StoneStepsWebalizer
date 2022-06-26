/*
    webalizer - a web server log analysis program

    Copyright (c) 2004-2022, Stone Steps Inc. (www.stonesteps.ca)

    See COPYING and Copyright files for additional licensing and copyright information

    scnode.cpp
*/
#include "pch.h"

#include "scnode.h"
#include "serialize.h"
#include "storable.h"

#include <stdexcept>

sc_table_t::sc_table_t(void)
{
   memset(clsindex, 0, sizeof(clsindex));
   stcodes.push_back(storable_t<scnode_t>(0));           // unknown status code node
}

sc_table_t::~sc_table_t(void)
{
}

void sc_table_t::add_status_code(u_int code)
{
   u_int cls = code / 100;

   // check if in the valid range 
   if(cls < 1 || cls > 5) 
      return;

   // and in the ascending order
   if(stcodes.size() && code <= stcodes[stcodes.size()-1].get_scode())
      throw std::runtime_error(string_t::_format("HTTP response code %d is being inserted out of order", code));

   // if first in its class, update the index
   if(clsindex[cls] == 0)
      clsindex[cls] = stcodes.size();

   stcodes.push_back(storable_t<scnode_t>(code));
}

storable_t<scnode_t>& sc_table_t::get_status_code(u_int code)
{
   size_t index;
   u_int cls = code / 100;

   // check if class is valid
   if(cls < 1 || cls > 5) 
      return stcodes[0];

   // start with the first code in its class
   for(index = clsindex[cls]; index < stcodes.size(); index++) {
      if(stcodes[index].get_scode() == code)
         return stcodes[index];

      // codes are sorted, break
      if(code < stcodes[index].get_scode())
         break;
   }

   // not found
   return stcodes[0];
}

storable_t<scnode_t>& sc_table_t::operator [] (size_t index)
{
   if(index >= stcodes.size())
      return stcodes[0];

   return stcodes[index];
}

const storable_t<scnode_t>& sc_table_t::operator [] (size_t index) const
{
   if(index >= stcodes.size())
      return stcodes[0];

   return stcodes[index];
}

//
// serialization
//

size_t scnode_t::s_data_size(void) const
{
   return datanode_t<scnode_t>::s_data_size() + 
            sizeof(uint64_t) +         // count
            sizeof(u_char);            // v2pad
}

size_t scnode_t::s_pack_data(void *buffer, size_t bufsize) const
{
   serializer_t sr(buffer, bufsize);

   size_t basesize = datanode_t<scnode_t>::s_pack_data(buffer, bufsize);
   void *ptr = (u_char*) buffer + basesize;

   ptr = sr.serialize(ptr, count);
   ptr = sr.serialize(ptr, v2pad);

   return sr.data_size(ptr);
}

template <typename ... param_t>
size_t scnode_t::s_unpack_data(const void *buffer, size_t bufsize, s_unpack_cb_t<param_t ...> upcb, param_t ... param)
{
   serializer_t sr(buffer, bufsize);

   u_short version;
   const void *ptr;

   // deal with the missing version in v1 (see class definition)
   if(bufsize == sizeof(u_short) + sizeof(uint64_t)) {
      version = 1;
      ptr = buffer;
   }
   else {
      size_t basesize = datanode_t<scnode_t>::s_unpack_data(buffer, bufsize);
      ptr = (u_char*) buffer + basesize;
      version = s_node_ver(buffer);
   }

   ptr = sr.deserialize(ptr, count);

   if(version >= 2)
      ptr = sr.deserialize(ptr, v2pad);
   
   return sr.data_size(ptr);
}

//
// Instantiate all template callbacks
//
template size_t scnode_t::s_unpack_data(const void *buffer, size_t bufsize, scnode_t::s_unpack_cb_t<> upcb);
