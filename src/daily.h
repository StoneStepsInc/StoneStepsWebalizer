/*
    webalizer - a web server log analysis program

    Copyright (c) 2004-2019, Stone Steps Inc. (www.stonesteps.ca)

    See COPYING and Copyright files for additional licensing and copyright information

    daily.h
*/
#ifndef DAILY_H
#define DAILY_H

#include "types.h"
#include "keynode.h"
#include "datanode.h"

///
/// @struct daily_t
///
/// @brief  Daily counters node 
///
///
struct daily_t : public keynode_t<uint32_t>, public datanode_t<daily_t> {
      uint64_t tm_hits;          ///< Daily requests total
      uint64_t tm_files;         ///< Daily files total
      uint64_t tm_pages;         ///< Daily pages total
      uint64_t tm_hosts;         ///< Daily hosts total
      uint64_t tm_visits;        ///< Daily visits total
      
      uint64_t h_hits_max;       ///< Maximum hits per hour
      uint64_t h_files_max;      ///< Maximum files per hour
      uint64_t h_pages_max;      ///< Maximum pages per hour
      uint64_t h_visits_max;     ///< Maximum visits per hour
      uint64_t h_hosts_max;      ///< maximum hosts per hour

      uint64_t h_xfer_max;       ///< Maximum transfer per hour
      
      double h_hits_avg;         ///< Average hits per hour
      double h_files_avg;        ///< Average files per hour
      double h_pages_avg;        ///< Average pages per hour
      double h_visits_avg;       ///< Average visits per hour
      double h_hosts_avg;        ///< Average hosts per hour
      
      double h_xfer_avg;         ///< Average transfer per hour, in bytes 
      
      uint64_t tm_xfer;          ///< Daily transfer total, in bytes
      
      u_short td_hours;          ///< Number of hours processed for a given day 

      public:
         template <typename ... param_t>
         using s_unpack_cb_t = void (*)(daily_t& daily, void *arg, param_t ... param);

      public:
         daily_t(u_int day = 0);

         void reset(u_int day);

         //
         // serialization
         //
         size_t s_data_size(void) const;
         size_t s_pack_data(void *buffer, size_t bufsize) const;

         template <typename ... param_t>
         size_t s_unpack_data(const void *buffer, size_t bufsize, s_unpack_cb_t<param_t ...> upcb, void *arg, param_t&& ... param);
};

#endif // DAILY_H

