/*
    webalizer - a web server log analysis program

    Copyright (c) 2004-2021, Stone Steps Inc. (www.stonesteps.ca)

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
      template <typename ... param_t>
      using s_unpack_cb_t = void (*)(sysnode_t& sysnode, param_t ... param);

   public:
      /// Constructs a default instance without the configuration object
      sysnode_t(void);
      
      /// Resets the instance using the configuration object.
      void reset(const config_t& config);
      
      /// Returns `true` if sizes of all fundamental data in the database matches run time data sizes, `false` otherwise.
      bool check_size_of(void) const;

      /// Returns `true` if the byte order in the database matches run time byte order, `false` otherwise.
      bool check_byte_order(void) const;

      /// Returns `true` if the time setting in the database match run time values, `false` otherwise.
      bool check_time_settings(const config_t& config) const;

      //
      // serialization
      //

      /// Returns the size of the current node if it is serialized.
      size_t s_data_size(void) const;

      /// Serializes the current node into the specified buffer. 
      size_t s_pack_data(void *buffer, size_t bufsize) const;

      /// Populates current node with deserialized data from the buffer.

      template <typename ... param_t>
      size_t s_unpack_data(const void *buffer, size_t bufsize, s_unpack_cb_t<param_t ...> upcb, param_t ... param);
};

#endif // SYSNODE_H
