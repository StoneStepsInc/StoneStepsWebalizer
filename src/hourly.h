/*
    webalizer - a web server log analysis program

    Copyright (c) 2004-2016, Stone Steps Inc. (www.stonesteps.ca)

    See COPYING and Copyright files for additional licensing and copyright information

    hourly.h
*/
#ifndef HOURLY_H
#define HOURLY_H

#include "types.h"
#include "keynode.h"
#include "datanode.h"

// -----------------------------------------------------------------------
//
// Hourly counters
//
// -----------------------------------------------------------------------
struct hourly_t : public keynode_t<uint32_t>, public datanode_t<hourly_t> {
      uint64_t th_hits;             // hourly requests total
      uint64_t th_files;            // hourly files total 
      uint64_t th_pages;            // hourly pages total
      uint64_t th_xfer;             // hourly transfer amount total, in bytes

      public:
         typedef void (*s_unpack_cb_t)(hourly_t& hourly, void *arg);

      public:
         hourly_t(u_int hour = 0);

         void reset(u_int hour);

         //
         // serialization
         //
         size_t s_data_size(void) const;
         size_t s_pack_data(void *buffer, size_t bufsize) const;
         size_t s_unpack_data(const void *buffer, size_t bufsize, s_unpack_cb_t upcb, void *arg);

         static size_t s_data_size(const void *buffer, bool fixver);
};

#endif // HOURLY_H

