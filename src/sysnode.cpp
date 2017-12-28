/*
    webalizer - a web server log analysis program

    Copyright (c) 2004-2017, Stone Steps Inc. (www.stonesteps.ca)

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
   byte_order_x64 = 0x1234567809ABCDEFull;

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
   byte_order_x64 = 0x1234567809ABCDEFull;

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
            s_size_of(logformat) +     // logformat
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
   size_t datasize, basesize;
   void *ptr = buffer;

   basesize = datanode_t<sysnode_t>::s_data_size();
   datasize = s_data_size();

   if(bufsize < s_data_size())
      return 0;

   datanode_t<sysnode_t>::s_pack_data(buffer, bufsize);
   ptr = (u_char*) buffer + basesize;

   ptr = serialize(ptr, appver);
   ptr = serialize(ptr, incremental);
   ptr = serialize(ptr, batch);
   ptr = serialize(ptr, filepos);
   ptr = serialize(ptr, logformat);
   ptr = serialize(ptr, fixed_dhv);

   ptr = serialize(ptr, sizeof_char);
   ptr = serialize(ptr, sizeof_short);
   ptr = serialize(ptr, sizeof_int);
   ptr = serialize(ptr, sizeof_long);
   ptr = serialize(ptr, sizeof_double);

   ptr = serialize(ptr, byte_order);
   
   ptr = serialize(ptr, appver_last);

   ptr = serialize(ptr, utc_time);
   ptr = serialize<short, int>(ptr, utc_offset);

   ptr = serialize(ptr, sizeof_longlong);
   ptr = serialize(ptr, byte_order_x64);

   return datasize;
}

size_t sysnode_t::s_unpack_data(const void *buffer, size_t bufsize, s_unpack_cb_t upcb, void *arg)
{
   u_short version;
   size_t datasize, basesize;
   const void *ptr = buffer;

   basesize = datanode_t<sysnode_t>::s_data_size(buffer);
   datasize = s_data_size(buffer);

   if(bufsize < datasize)
      return 0;

   version = s_node_ver(buffer);
   datanode_t<sysnode_t>::s_unpack_data(buffer, bufsize);
   ptr = (u_char*) buffer + basesize;

   ptr = deserialize(ptr, appver);
   ptr = deserialize(ptr, incremental);

   if(version >= 2)
      ptr = deserialize(ptr, batch);
   else
      batch = false;

   ptr = s_skip_field<uint32_t>(ptr);       // filepos
   ptr = s_skip_field<string_t>(ptr);       // logformat

   if(version >= 3)
      ptr = deserialize(ptr, fixed_dhv);
   else
      fixed_dhv = false;

   if(version >= 4) {
      ptr = deserialize(ptr, sizeof_char);
      ptr = deserialize(ptr, sizeof_short);
      ptr = deserialize(ptr, sizeof_int);
      ptr = deserialize(ptr, sizeof_long);
      ptr = deserialize(ptr, sizeof_double);
      
      ptr = deserialize(ptr, byte_order);
      
      ptr = deserialize(ptr, appver_last);
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
      ptr = deserialize(ptr, utc_time);
      ptr = deserialize<short>(ptr, utc_offset);
   }

   if(version >= 6) {
      ptr = deserialize(ptr, sizeof_longlong);
      ptr = deserialize(ptr, byte_order_x64);
   }
   else {
      sizeof_longlong = sizeof(long long);
      byte_order_x64 = 0x1234567890ABCDEFull;
   }

   if(upcb)
      upcb(*this, arg);

   return datasize;
}

size_t sysnode_t::s_data_size(const void *buffer)
{
   u_short version = s_node_ver(buffer);
   size_t datasize;

   datasize = datanode_t<sysnode_t>::s_data_size(buffer) + 
            sizeof(u_int)        +        // appver
            sizeof(u_char);               // incremental

   if(version >= 2)
      datasize += sizeof(u_char);         // batch mode
   
   // batch_mode was inserted between filepos and logformat in v2   
   datasize += sizeof(uint32_t);          // filepos
   datasize += s_size_of<string_t>((u_char*) buffer + datasize);    // logformat
   
   if(version < 3)
      return datasize;
      
   datasize += sizeof(u_char);            // fixed_dhv
   
   if(version < 4)
      return datasize;
      
   datasize += 
      sizeof(u_short) * 5  +  // sizeof char, short, int, long and double 
      sizeof(u_int) * 2;      // byte_order, appver_last

   if(version < 5)
      return datasize;

   datasize +=
            sizeof(u_char)       +        // utc_time
            sizeof(short);                // utc_offset

   if(version < 6)
      return datasize;

   return datasize +
            sizeof(u_short)      +        // sizeof_longlong
            sizeof(uint64_t);             // byte_order_x64
}
