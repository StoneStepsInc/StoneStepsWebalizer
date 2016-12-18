/*
    webalizer - a web server log analysis program

    Copyright (c) 2004-2016, Stone Steps Inc. (www.stonesteps.ca)

    See COPYING and Copyright files for additional licensing and copyright information

    daily.h
*/
#ifndef DAILY_H
#define DAILY_H

#include "types.h"
#include "keynode.h"
#include "datanode.h"

// -----------------------------------------------------------------------
//
// Daily counters
//
// -----------------------------------------------------------------------
struct daily_t : public keynode_t<uint32_t>, public datanode_t<daily_t> {
      uint64_t tm_hits;          // daily requests total
      uint64_t tm_files;         // daily files total
      uint64_t tm_pages;         // daily pages total
      uint64_t tm_hosts;         // daily hosts total
      uint64_t tm_visits;        // daily visits total
      
      uint64_t h_hits_max;       // maximum hits per hour
      uint64_t h_files_max;      // maximum files per hour
      uint64_t h_pages_max;      // maximum pages per hour
      uint64_t h_visits_max;     // maximum visits per hour
      uint64_t h_hosts_max;      // maximum hosts per hour

      uint64_t h_xfer_max;       // maximum transfer per hour
      
      double h_hits_avg;         // average hits per hour
      double h_files_avg;        // average files per hour
      double h_pages_avg;        // average pages per hour
      double h_visits_avg;       // average visits per hour
      double h_hosts_avg;        // average hosts per hour
      
      double h_xfer_avg;         // average transfer per hour, in bytes 
      
      uint64_t tm_xfer;          // daily transfer total, in bytes
      
      u_short td_hours;          // number of hours processed for a given day 

      public:
         typedef void (*s_unpack_cb_t)(daily_t& daily, void *arg);

      public:
         daily_t(u_int day = 0);

         void reset(u_int day);

         //
         // serialization
         //
         size_t s_data_size(void) const;
         size_t s_pack_data(void *buffer, size_t bufsize) const;
         size_t s_unpack_data(const void *buffer, size_t bufsize, s_unpack_cb_t upcb, void *arg);

         static size_t s_data_size(const void *buffer, bool fixver);
};

#endif // DAILY_H

