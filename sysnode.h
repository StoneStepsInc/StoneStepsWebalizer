/*
    webalizer - a web server log analysis program

    Copyright (c) 2004-2014, Stone Steps Inc. (www.stonesteps.ca)

    See COPYING and Copyright files for additional licensing and copyright information

    sysnode.h
*/
#ifndef __SYSNODE_H
#define __SYSNODE_H

#include "keynode.h"
#include "datanode.h"
#include "tstring.h"

// -----------------------------------------------------------------------
//
// sysnode_t
//
// -----------------------------------------------------------------------
// 1. System node contains application-specific data to be stored in the
// database, such as the application version.
//
// 2. Application version is immutable and once stored, never changes. 
// In other words, appver indicates the version of the application that  
// created the database.
//
struct sysnode_t : public keynode_t<u_long>, datanode_t<sysnode_t> {
   u_int       appver;              // application version
   u_int       appver_last;         // last application version
   bool        incremental;         // incremetal database?
   bool        batch;               // batch processing?
   bool        fixed_dhv;           // fixed daily/hourly records?
   u_long      filepos;             // log file position (not used)
   string_t    logformat;           // log format line (not used)
   
   u_short     sizeof_char;
   u_short     sizeof_short;
   u_short     sizeof_int;
   u_short     sizeof_long;
   u_short     sizeof_double;
   
   u_int       byte_order;

   bool        utc_time;            // UTC or local time?
   int         utc_offset;          // UTC offset in minutes if local time

   public:
      typedef void (*s_unpack_cb_t)(sysnode_t& sysnode, void *arg);

   public:
      sysnode_t(void);
      
      void reset(const config_t& config);
      
      bool check_size_of(void) const;
      bool check_byte_order(void) const {return byte_order == 0x12345678u;}
      bool check_time_settings(const config_t& config);

      //
      // serialization
      //
      u_int s_data_size(void) const;
      u_int s_pack_data(void *buffer, u_int bufsize) const;
      u_int s_unpack_data(const void *buffer, u_int bufsize, s_unpack_cb_t upcb, void *arg);

      static u_int s_data_size(const void *buffer);
};

#endif // __SYSNODE_H
