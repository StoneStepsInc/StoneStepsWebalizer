/*
    webalizer - a web server log analysis program

    Copyright (c) 2004-2017, Stone Steps Inc. (www.stonesteps.ca)

    See COPYING and Copyright files for additional licensing and copyright information

    sysnode.h
*/
#ifndef SYSNODE_H
#define SYSNODE_H

#include "keynode.h"
#include "datanode.h"
#include "tstring.h"
#include "config.h"

///
/// @struct sysnode_t
///
/// @brief  Application node
///
/// 1. System node contains application-specific data to be stored in the
/// database, such as the application version.
///
/// 2. Application version is immutable and once stored, never changes. 
/// In other words, appver indicates the version of the application that  
/// created the database.
///
struct sysnode_t : public keynode_t<uint32_t>, datanode_t<sysnode_t> {
   u_int       appver;              ///< Application version
   u_int       appver_last;         ///< Last application version
   bool        incremental;         ///< Incremetal database?
   bool        batch;               ///< Batch processing?
   bool        fixed_dhv;           ///< Fixed daily/hourly records?
   uint32_t    filepos;             ///< Log file position (not used)
   string_t    logformat;           ///< Log format line (not used)
   
   u_short     sizeof_char;         ///< `sizeof(char)` where `sysnode_t` was created
   u_short     sizeof_short;        ///< `sizeof(short)` where `sysnode_t` was created
   u_short     sizeof_int;          ///< `sizeof(int)` where `sysnode_t` was created
   u_short     sizeof_long;         ///< `sizeof(long)` where `sysnode_t` was created
   u_short     sizeof_double;       ///< `sizeof(double)` where `sysnode_t` was created
   u_short     sizeof_longlong;     ///< `sizeof(longlong)` where `sysnode_t` was created
   
   u_int       byte_order;          ///< Value `0x12345678` where `sysnode_t` was created
   uint64_t    byte_order_x64;      ///< Value `0x1234567890ABCDEF` where `sysnode_t` was created

   bool        utc_time;            ///< UTC or local time?
   int         utc_offset;          ///< UTC offset in minutes if local time

   public:
      typedef void (*s_unpack_cb_t)(sysnode_t& sysnode, void *arg);

   public:
      sysnode_t(void);
      
      void reset(const config_t& config);
      
      bool check_size_of(void) const;
      bool check_byte_order(void) const;
      bool check_time_settings(const config_t& config) const;

      //
      // serialization
      //
      size_t s_data_size(void) const;
      size_t s_pack_data(void *buffer, size_t bufsize) const;
      size_t s_unpack_data(const void *buffer, size_t bufsize, s_unpack_cb_t upcb, void *arg);

      static size_t s_data_size(const void *buffer);
};

#endif // SYSNODE_H
