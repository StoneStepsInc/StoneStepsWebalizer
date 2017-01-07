/*
   webalizer - a web server log analysis program

   Copyright (c) 2004-2017, Stone Steps Inc. (www.stonesteps.ca)

   See COPYING and Copyright files for additional licensing and copyright information 
   
   serialize.cpp
*/
#include "pch.h"

#include "serialize.h"

void *serialize(void *ptr, const string_t& value)
{
   void *cp = serialize<u_int>(ptr, (u_int) value.length());
   if(value.length())
      memcpy(cp, value.c_str(), value.length());
   return (u_char*) ptr + sizeof(u_int) + value.length();
}

void *serialize(void *ptr, const tstamp_t& tstamp)
{
   //
   // Serialize null and UTC flags first, so we can figure out the size 
   // of the timestamp in the buffer after reading one or two bytes.
   //
   ptr = serialize(ptr, tstamp.null);

   if(tstamp.null)
      return ptr;

   ptr = serialize(ptr, tstamp.utc);

   // now serialize all fixed-size fields
   ptr = serialize<u_short>(ptr, tstamp.year);
   ptr = serialize<u_char>(ptr, tstamp.month);
   ptr = serialize<u_char>(ptr, tstamp.day);

   ptr = serialize<u_char>(ptr, tstamp.hour);
   ptr = serialize<u_char>(ptr, tstamp.min);
   ptr = serialize<u_char>(ptr, tstamp.sec);

   // finally, serialize the UTC offset for local times
   if(!tstamp.utc)
      ptr = serialize<short>(ptr, tstamp.offset);

   return ptr;
}

const void *deserialize(const void *ptr, string_t& value)
{
   size_t slen;
   const void *cp;

   // call explicit specialization because size_t is unsigned int on Win32
   cp = deserialize<u_int, size_t>(ptr, slen);

   if(slen) {
      string_t::char_buffer_t sp(slen+1);
      memcpy(sp, cp, slen);
      sp[slen] = 0;
      value.attach(std::move(sp), slen);
   }
   else
      value.reset();

   return &((u_char*)ptr)[sizeof(u_int)+slen];
}

const void *deserialize(const void *ptr, tstamp_t& tstamp)
{
   bool null, utc;
   u_short year;
   u_char month, day, hour, min, sec;
   short offset;

   ptr = deserialize(ptr, null);

   if(null) {
      tstamp.reset();
      return ptr;
   }

   ptr = deserialize(ptr, utc);

   ptr = deserialize(ptr, year);
   ptr = deserialize(ptr, month);
   ptr = deserialize(ptr, day);

   ptr = deserialize(ptr, hour);
   ptr = deserialize(ptr, min);
   ptr = deserialize(ptr, sec);

   if(utc)
      tstamp.reset(year, month, day, hour, min, sec);
   else {
      ptr = deserialize(ptr, offset);
      tstamp.reset(year, month, day, hour, min, sec, offset);
   }

   return ptr;
}

size_t s_size_of(const tstamp_t& tstamp)
{
   size_t datasize = sizeof(u_char);      // null
   
   if(tstamp.null)
      return datasize;
       
   datasize += sizeof(u_char)       +     // utc
            sizeof(u_short)         +     // year
            sizeof(u_char) * 5;           // month, day, hour, min, sec

   return tstamp.utc ? datasize : 
               datasize + sizeof(short);  // offset
}

template <>
size_t s_size_of<tstamp_t>(const void *ptr)
{
   size_t datasize = sizeof(u_char);      // null
   
   if(*((u_char*) ptr))
      return datasize;

   datasize += sizeof(u_char)       +     // utc
            sizeof(u_short)         +     // year
            sizeof(u_char) * 5;           // month, day, hour, min, sec
               
   return (*((u_char*) ptr+1)) ? datasize : 
            datasize + sizeof(short);     // offset
}

//
// Instantiate template functinos defined in this file
//
template void *s_skip_field<tstamp_t>(const void *ptr);
template size_t s_size_of<tstamp_t>(const void *ptr);
