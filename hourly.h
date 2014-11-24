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
struct hourly_t : public keynode_t<u_int>, public datanode_t<hourly_t> {
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
         u_int s_data_size(void) const;
         u_int s_pack_data(void *buffer, u_int bufsize) const;
         u_int s_unpack_data(const void *buffer, u_int bufsize, s_unpack_cb_t upcb, void *arg);

         static u_int s_data_size(const void *buffer, bool fixver);
};

#endif // __HOURLY_H

