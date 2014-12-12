/*
    webalizer - a web server log analysis program

    Copyright (c) 2004-2014, Stone Steps Inc. (www.stonesteps.ca)

    See COPYING and Copyright files for additional licensing and copyright information

    totals.h
*/
#ifndef __TOTALS_H
#define __TOTALS_H

#include "types.h"
#include "keynode.h"
#include "datanode.h"
#include "version.h"

// -----------------------------------------------------------------------
//
// totals_t
//
// -----------------------------------------------------------------------
//
// 1. Only robot visits may be identified reliably from the start of the 
// visit. Other types of visits may only be identified during the visit, 
// given the types of URLs requested and browsing patterns.
//
class totals_t : public keynode_t<uint32_t>, public datanode_t<totals_t> {
   public:
      tstamp_t cur_tstamp;                       // current timestamp

      u_int f_day;                               // first day of the month
      u_int l_day;                               // last day of the month

      uint64_t max_v_hits;                         // maximum number of hits
      uint64_t max_v_files;                        // files
      uint64_t max_v_pages;                        // pages
      uint64_t max_v_xfer;                         // and transfer amount per visit
      
      uint64_t t_hit;                              // monthly total vars: hits
      uint64_t t_file;                             // files
      uint64_t t_page;                             // pages
      uint64_t t_hosts;                            // hosts
      uint64_t t_hosts_conv;                       // converted hosts
      uint64_t t_url;                              // urls
      uint64_t t_ref;                              // referrers
      uint64_t t_agent;                            // user agents
      uint64_t t_user;                             // users
      uint64_t t_err;                              // errors
      uint64_t t_dlcount;                          // download count
      uint64_t t_downloads;                        // download record count
      uint64_t t_srchits;                          // search string hits
      uint64_t t_search;                           // search strings
      uint64_t t_entry;                            // entry page hits
      uint64_t t_exit;                             // exit page hits
      uint64_t u_entry;                            // entry pages
      uint64_t u_exit;                             // exit pages
      uint64_t t_xfer;                             // transfer

      uint64_t t_rhits;                            // robot hits
      uint64_t t_rfiles;                           // robot files
      uint64_t t_rpages;                           // robot pages
      uint64_t t_rerrors;                          // robot errors
      uint64_t t_rhosts;                           // robot hosts
      uint64_t t_rxfer;                            // robot transfer

      uint64_t t_visits;                           // visits started
      uint64_t t_rvisits;                          // robot visits started
      uint64_t t_visits_end;                       // visits ended
      uint64_t t_rvisits_end;                      // robot visits ended
      uint64_t t_svisits_end;                      // spammer visits ended
      uint64_t t_hvisits_end;                      // human visits ended
      uint64_t t_visits_conv;                      // converted visits ended (% of t_hvisits_end)
      
      uint64_t t_spmhits;                          // spammer hits
      uint64_t t_sfiles;                           // spammer files
      uint64_t t_spages;                           // spammer pages
      uint64_t t_shosts;                           // spammer hosts
      uint64_t t_sxfer;                            // spammer transfer

      double t_visit_avg;                        // average human visit length
      uint64_t t_visit_max;                        // maximum human visit length

      double t_vconv_avg;                        // average converted visit length
      uint64_t t_vconv_max;                        // maximum converted visit length

      uint64_t max_hv_hits;                        // maximum number of hits
      uint64_t max_hv_files;                       // files
      uint64_t max_hv_pages;                       // pages
      uint64_t max_hv_xfer;                        // and transfer amount per human visit

      uint64_t t_grp_hosts;                        // total host groups
      uint64_t t_grp_urls;                         // total url groups
      uint64_t t_grp_users;                        // total user groups
      uint64_t t_grp_refs;                         // total referrer groups
      uint64_t t_grp_agents;                       // total agent groups

	   double a_hitptime;                         // average and maximum
      double m_hitptime;		                   // hit, page and file       	
	   double a_fileptime;                        // processing time
      double m_fileptime;		
	   double a_pageptime;
      double m_pageptime;		

      uint64_t dt_hosts;                           // daily hosts total

      uint64_t hm_hit;                             // current hourly hits maximum
      
      uint64_t ht_hits;                            // current hourly counts
      uint64_t ht_files;
      uint64_t ht_pages;
      uint64_t ht_xfer;
      uint64_t ht_visits;
      uint64_t ht_hosts;

   public:
      typedef void (*s_unpack_cb_t)(totals_t& state, void *arg);

   public:
      totals_t(void);

      void init_counters(void);

      //
      // serialization
      //
      size_t s_data_size(void) const;
      size_t s_pack_data(void *buffer, size_t bufsize) const;
      size_t s_unpack_data(const void *buffer, size_t bufsize, s_unpack_cb_t upcb, void *arg);

      static size_t s_data_size(const void *buffer);
};

#endif // __TOTALS_H
