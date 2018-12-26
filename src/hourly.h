/*
    webalizer - a web server log analysis program

    Copyright (c) 2004-2019, Stone Steps Inc. (www.stonesteps.ca)

    See COPYING and Copyright files for additional licensing and copyright information

    hourly.h
*/
#ifndef HOURLY_H
#define HOURLY_H

#include "types.h"
#include "keynode.h"
#include "datanode.h"

///
/// @struct hourly_t
///
/// @brief  Hourly counters node
///
struct hourly_t : public keynode_t<uint32_t>, public datanode_t<hourly_t> {
      uint64_t th_hits;             ///< Hourly requests total
      uint64_t th_files;            ///< Hourly files total 
      uint64_t th_pages;            ///< Hourly pages total
      uint64_t th_xfer;             ///< Hourly transfer amount total, in bytes

      public:
         template <typename ... param_t>
         using s_unpack_cb_t = void (*)(hourly_t& hourly, void *arg, param_t ... param);

      public:
         hourly_t(u_int hour = 0);

         void reset(u_int hour);

         //
         // serialization
         //
         size_t s_data_size(void) const;
         size_t s_pack_data(void *buffer, size_t bufsize) const;

         template <typename ... param_t>
         size_t s_unpack_data(const void *buffer, size_t bufsize, s_unpack_cb_t<param_t ...> upcb, void *arg, param_t&& ... param);

         static size_t s_data_size(const void *buffer, bool fixver);
};

#endif // HOURLY_H

