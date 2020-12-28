/*
    webalizer - a web server log analysis program

    Copyright (c) 2004-2021, Stone Steps Inc. (www.stonesteps.ca)

    See COPYING and Copyright files for additional licensing and copyright information

    totals.h
*/
#ifndef TOTALS_H
#define TOTALS_H

#include "keynode.h"
#include "datanode.h"
#include "tstamp.h"
#include "types.h"
#include "version.h"

///
/// @brief  Monthly totals node
///
/// 1. Only robot visits may be identified reliably from the start of the 
/// visit. Other types of visits may only be identified during the visit, 
/// given the types of URLs requested and browsing patterns.
///
class totals_t : public keynode_t<uint32_t>, public datanode_t<totals_t> {
   public:
      tstamp_t cur_tstamp;                         ///< Current timestamp

      u_int f_day;                                 ///< First day of the month
      u_int l_day;                                 ///< Last day of the month

      uint64_t max_v_hits;                         ///< Maximum number of requests
      uint64_t max_v_files;                        ///< Maximum number of file requests
      uint64_t max_v_pages;                        ///< Maximum number of page requests
      uint64_t max_v_xfer;                         ///< Transfer amount per visit
      
      uint64_t t_hit;                              ///< Request count
      uint64_t t_file;                             ///< File request count
      uint64_t t_page;                             ///< Page request count
      uint64_t t_hosts;                            ///< Host count
      uint64_t t_hosts_conv;                       ///< Converted host  count
      uint64_t t_url;                              ///< URL count
      uint64_t t_ref;                              ///< Referrer count
      uint64_t t_agent;                            ///< User agent count
      uint64_t t_user;                             ///< User count
      uint64_t t_err;                              ///< Error count
      uint64_t t_dlcount;                          ///< Download count
      uint64_t t_downloads;                        ///< Download job count
      uint64_t t_srchits;                          ///< Search string request count
      uint64_t t_search;                           ///< Search string count
      uint64_t t_entry;                            ///< Entry page request count
      uint64_t t_exit;                             ///< Exit page request count
      uint64_t u_entry;                            ///< Entry page count
      uint64_t u_exit;                             ///< Exit page count
      uint64_t t_xfer;                             ///< Transfer

      uint64_t t_rhits;                            ///< Robot request count
      uint64_t t_rfiles;                           ///< Robot file request count
      uint64_t t_rpages;                           ///< Robot page request count
      uint64_t t_rerrors;                          ///< Robot error request count
      uint64_t t_rhosts;                           ///< Robot host request count
      uint64_t t_rxfer;                            ///< Robot transfer

      uint64_t t_visits;                           ///< Visits started
      uint64_t t_rvisits;                          ///< Robot visits started
      uint64_t t_visits_end;                       ///< Visits ended
      uint64_t t_rvisits_end;                      ///< Robot visits ended
      uint64_t t_svisits_end;                      ///< Spammer visits ended
      uint64_t t_hvisits_end;                      ///< Human visits ended
      uint64_t t_visits_conv;                      ///< Converted visits ended (% of t_hvisits_end)
      
      uint64_t t_spmhits;                          ///< Spammer hits
      uint64_t t_sfiles;                           ///< Spammer files
      uint64_t t_spages;                           ///< Spammer pages
      uint64_t t_shosts;                           ///< Spammer hosts
      uint64_t t_sxfer;                            ///< Spammer transfer

      double t_visit_avg;                          ///< Average human visit length
      uint64_t t_visit_max;                        ///< Maximum human visit length

      double t_vconv_avg;                          ///< Average converted visit length
      uint64_t t_vconv_max;                        ///< Maximum converted visit length

      uint64_t max_hv_hits;                        ///< Maximum number of requests per human visit
      uint64_t max_hv_files;                       ///< Maximum number of file requests per human visit
      uint64_t max_hv_pages;                       ///< Maximum number of page requests per human visit
      uint64_t max_hv_xfer;                        ///< Maximum transfer amount per human visit

      uint64_t t_grp_hosts;                        ///< Total number of host groups
      uint64_t t_grp_urls;                         ///< Total number of URL groups
      uint64_t t_grp_users;                        ///< Total number of user groups
      uint64_t t_grp_refs;                         ///< Total number of referrer groups
      uint64_t t_grp_agents;                       ///< Total number of agent groups

      double a_hitptime;                           ///< Average request processing time
      double m_hitptime;                           ///< Maximim request processing time
      double a_fileptime;                          ///< Average file request processing time
      double m_fileptime;                          ///< Maximum file request processing time
      double a_pageptime;                          ///< Average page request processing time
      double m_pageptime;                          ///< Maximum page request processing time

      uint64_t hm_hit;                             ///< Hourly request count maximum
      
      uint64_t ht_hits;                            ///< Hourly request count
      uint64_t ht_files;                           ///< Hourly file request count
      uint64_t ht_pages;                           ///< Hourly page request count
      uint64_t ht_xfer;                            ///< Hourly tranfer
      uint64_t ht_visits;                          ///< Hourly visit count
      uint64_t ht_hosts;                           ///< Hourly host count

   public:
      template <typename ... param_t>
      using s_unpack_cb_t = void (*)(totals_t& state, param_t ... param);

   public:
      totals_t(void);

      void init_counters(void);

      //
      // serialization
      //
      size_t s_data_size(void) const;
      size_t s_pack_data(void *buffer, size_t bufsize) const;

      template <typename ... param_t>
      size_t s_unpack_data(const void *buffer, size_t bufsize, s_unpack_cb_t<param_t ...> upcb, param_t ... param);
};

#endif // TOTALS_H
