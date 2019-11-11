/*
    webalizer - a web server log analysis program

    Copyright (c) 2004-2019, Stone Steps Inc. (www.stonesteps.ca)

    See COPYING and Copyright files for additional licensing and copyright information

    sysnode.cpp
*/
#include "pch.h"

#include "sysnode.h"
#include "version.h"
#include "serialize.h"

sysnode_t::sysnode_t(void) : keynode_t<uint32_t>(1)
{
   appver = 0;
   appver_last = 0;
   incremental = false;
   batch = false;
   fixed_dhv = false;
   filepos = 0;

   sizeof_char = sizeof(char);
   sizeof_short = sizeof(short);
   sizeof_int = sizeof(int);
   sizeof_long = sizeof(long);
   sizeof_double = sizeof(double);
   sizeof_longlong = sizeof(long long);
   
   byte_order = 0x12345678u;
   byte_order_x64 = 0x1234567890ABCDEFull;

   utc_time = true;
   utc_offset = 0; 
}

void sysnode_t::reset(const config_t& config)
{
   keynode_t<uint32_t>::reset(1);

   appver = 0;
   appver_last = 0;
   incremental = false;
   batch = false;
   fixed_dhv = false;
   filepos = 0;

   sizeof_char = sizeof(char);
   sizeof_short = sizeof(short);
   sizeof_int = sizeof(int);
   sizeof_long = sizeof(long);
   sizeof_double = sizeof(double);
   sizeof_longlong = sizeof(long long);
   
   byte_order = 0x12345678u;
   byte_order_x64 = 0x1234567890ABCDEFull;

   utc_time = !config.local_time;
   utc_offset = config.utc_offset; 
}

bool sysnode_t::check_size_of(void) const
{
   if(sizeof_char != sizeof(char))
      return false;
      
   if(sizeof_short != sizeof(short))
      return false;
      
   if(sizeof_int != sizeof(int))
      return false;
      
   if(sizeof_long != sizeof(long))
      return false;
      
   if(sizeof_double != sizeof(double))
      return false;
      
   if(sizeof_longlong != sizeof(long long))
      return false;

   return true;
}

bool sysnode_t::check_byte_order(void) const 
{
   return byte_order == 0x12345678u && byte_order_x64 == 0x1234567890ABCDEFull;
}

bool sysnode_t::check_time_settings(const config_t& config) const
{
   return utc_time == !config.local_time && utc_offset == config.utc_offset;
}

size_t sysnode_t::s_data_size(void) const
{
   return datanode_t<sysnode_t>::s_data_size() + 
            sizeof(u_int)        +     // application version
            sizeof(u_char)       +     // incremental database?
            sizeof(u_char)       +     // batch mode?
            sizeof(u_char)       +     // fixed_dhv
            sizeof(uint32_t)     +     // filepos
            serializer_t::s_size_of(logformat) +   // logformat
            sizeof(u_short) * 5  +     // sizeof char, short, int, long and double
            sizeof(u_int)        +     // byte_order
            sizeof(u_int)        +     // appvar_last
            sizeof(u_char)       +     // utc_time
            sizeof(short)        +     // utc_offset
            sizeof(u_short)      +     // sizeof_longlong
            sizeof(uint64_t)     ;     // byte_order_x64
}

size_t sysnode_t::s_pack_data(void *buffer, size_t bufsize) const
{
   serializer_t sr(buffer, bufsize);

   size_t basesize = datanode_t<sysnode_t>::s_pack_data(buffer, bufsize);
   void *ptr = (u_char*) buffer + basesize;

   ptr = sr.serialize(ptr, appver);
   ptr = sr.serialize(ptr, incremental);
   ptr = sr.serialize(ptr, batch);
   ptr = sr.serialize(ptr, filepos);
   ptr = sr.serialize(ptr, logformat);
   ptr = sr.serialize(ptr, fixed_dhv);

   ptr = sr.serialize(ptr, sizeof_char);
   ptr = sr.serialize(ptr, sizeof_short);
   ptr = sr.serialize(ptr, sizeof_int);
   ptr = sr.serialize(ptr, sizeof_long);
   ptr = sr.serialize(ptr, sizeof_double);

   ptr = sr.serialize(ptr, byte_order);
   
   ptr = sr.serialize(ptr, appver_last);

   ptr = sr.serialize(ptr, utc_time);
   ptr = sr.serialize<short, int>(ptr, utc_offset);

   ptr = sr.serialize(ptr, sizeof_longlong);
   ptr = sr.serialize(ptr, byte_order_x64);

   return sr.data_size(ptr);
}

template <typename ... param_t>
size_t sysnode_t::s_unpack_data(const void *buffer, size_t bufsize, s_unpack_cb_t<param_t ...> upcb, param_t ... param)
{
   serializer_t sr(buffer, bufsize);

   size_t basesize = datanode_t<sysnode_t>::s_unpack_data(buffer, bufsize);
   const void *ptr = (u_char*) buffer + basesize;

   u_short version = s_node_ver(buffer);

   ptr = sr.deserialize(ptr, appver);
   ptr = sr.deserialize(ptr, incremental);

   if(version >= 2)
      ptr = sr.deserialize(ptr, batch);
   else
      batch = false;

   ptr = sr.s_skip_field<uint32_t>(ptr);       // filepos
   ptr = sr.s_skip_field<string_t>(ptr);       // logformat

   if(version >= 3)
      ptr = sr.deserialize(ptr, fixed_dhv);
   else
      fixed_dhv = false;

   if(version >= 4) {
      ptr = sr.deserialize(ptr, sizeof_char);
      ptr = sr.deserialize(ptr, sizeof_short);
      ptr = sr.deserialize(ptr, sizeof_int);
      ptr = sr.deserialize(ptr, sizeof_long);
      ptr = sr.deserialize(ptr, sizeof_double);
      
      ptr = sr.deserialize(ptr, byte_order);
      
      ptr = sr.deserialize(ptr, appver_last);
   }
   else {
      sizeof_char = sizeof(char);
      sizeof_short = sizeof(short);
      sizeof_int = sizeof(int);
      sizeof_long = sizeof(long);
      sizeof_double = sizeof(double);
      
      byte_order = 0x12345678u;
      appver_last = 0;
   }

   //
   // Leave time settings at their initial values for older versions of 
   // sysnode_t, which assumes that the database is in sync with time
   // settings in the configuration file. 
   //
   if(version >= 5) {
      ptr = sr.deserialize(ptr, utc_time);
      ptr = sr.deserialize<short>(ptr, utc_offset);
   }

   if(version >= 6) {
      ptr = sr.deserialize(ptr, sizeof_longlong);
      ptr = sr.deserialize(ptr, byte_order_x64);
   }
   else {
      sizeof_longlong = sizeof(long long);
      byte_order_x64 = 0x1234567890ABCDEFull;
   }

   if(upcb)
      upcb(*this, std::forward<param_t>(param) ...);

   return sr.data_size(ptr);
}

//
// Instantiate all template callbacks
//
template size_t sysnode_t::s_unpack_data(const void *buffer, size_t bufsize, sysnode_t::s_unpack_cb_t<> upcb);
