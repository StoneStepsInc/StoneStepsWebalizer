/*
    webalizer - a web server log analysis program

    Copyright (c) 2004-2013, Stone Steps Inc. (www.stonesteps.ca)

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
      u_long th_hits;
      u_long th_files;
      u_long th_pages;
      double th_xfer;

      public:
         typedef void (*s_unpack_cb_t)(hourly_t& hourly, void *arg);

      public:
         hourly_t(u_int hour = 0);

         void reset(u_long nodeid);

         //
         // serialization
         //
         u_int s_data_size(void) const;
         u_int s_pack_data(void *buffer, u_int bufsize) const;
         u_int s_unpack_data(const void *buffer, u_int bufsize, s_unpack_cb_t upcb, void *arg);

         static u_int s_data_size(const void *buffer, bool fixver);
};

#endif // __HOURLY_H

