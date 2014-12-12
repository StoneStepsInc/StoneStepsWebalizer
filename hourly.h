/*
    webalizer - a web server log analysis program

    Copyright (c) 2004-2014, Stone Steps Inc. (www.stonesteps.ca)

    See COPYING and Copyright files for additional licensing and copyright information

    hourly.h
*/
#ifndef __HOURLY_H
#define __HOURLY_H

#include "types.h"
#include "keynode.h"
#include "datanode.h"

// -----------------------------------------------------------------------
//
// hourly_t
//
// -----------------------------------------------------------------------
struct hourly_t : public keynode_t<uint32_t>, public datanode_t<hourly_t> {
      uint64_t th_hits;
      uint64_t th_files;
      uint64_t th_pages;
      uint64_t th_xfer;

      public:
         typedef void (*s_unpack_cb_t)(hourly_t& hourly, void *arg);

      public:
         hourly_t(u_int hour = 0);

         void reset(u_int nodeid);

         //
         // serialization
         //
         size_t s_data_size(void) const;
         size_t s_pack_data(void *buffer, size_t bufsize) const;
         size_t s_unpack_data(const void *buffer, size_t bufsize, s_unpack_cb_t upcb, void *arg);

         static size_t s_data_size(const void *buffer, bool fixver);
};

#endif // __HOURLY_H

