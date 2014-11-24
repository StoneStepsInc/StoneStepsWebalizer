/*
    webalizer - a web server log analysis program

    Copyright (c) 2004-2014, Stone Steps Inc. (www.stonesteps.ca)

    See COPYING and Copyright files for additional licensing and copyright information

    daily.h
*/
#ifndef __DAILY_H
#define __DAILY_H

#include "types.h"
#include "keynode.h"
#include "datanode.h"

// -----------------------------------------------------------------------
//
// daily_t
//
// -----------------------------------------------------------------------
struct daily_t : public keynode_t<u_int>, public datanode_t<daily_t> {
      uint64_t tm_hits;
      uint64_t tm_files;
      uint64_t tm_pages;
      uint64_t tm_hosts;
      uint64_t tm_visits;
      
      uint64_t h_hits_max;         // maximum hits per hour
      uint64_t h_files_max;        // maximum files per hour
      uint64_t h_pages_max;        // maximum pages per hour
      uint64_t h_visits_max;       // maximum visits per hour
      uint64_t h_hosts_max;        // maximum hosts per hour

      uint64_t h_xfer_max;         // maximum transfer per hour
      
      double h_hits_avg;         // average hits per hour
      double h_files_avg;        // average files per hour
      double h_pages_avg;        // average pages per hour
      double h_visits_avg;       // average visits per hour
      double h_hosts_avg;        // average hosts per hour
      
      double h_xfer_avg;
      
      uint64_t tm_xfer;
      
      u_short td_hours;

      public:
         typedef void (*s_unpack_cb_t)(daily_t& daily, void *arg);

      public:
         daily_t(u_int month = 0);

         void reset(u_int nodeid);

         //
         // serialization
         //
         u_int s_data_size(void) const;
         u_int s_pack_data(void *buffer, u_int bufsize) const;
         u_int s_unpack_data(const void *buffer, u_int bufsize, s_unpack_cb_t upcb, void *arg);

         static u_int s_data_size(const void *buffer, bool fixver);
};

#endif // __DAILY_H

