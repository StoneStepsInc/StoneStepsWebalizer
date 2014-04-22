/*
    webalizer - a web server log analysis program

    Copyright (c) 2004-2013, Stone Steps Inc. (www.stonesteps.ca)

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
class totals_t : public keynode_t<u_long>, public datanode_t<totals_t> {
   public:
      u_long cur_tstamp;                         // current timestamp

      u_int cur_year;                            // year/month/day/hour
      u_int cur_month;                           // tracking variables
      u_int cur_day;
      u_int cur_hour;
      u_int cur_min;
      u_int cur_sec;

      u_int f_day;                               // first day of the month
      u_int l_day;                               // last day of the month

      u_long max_v_hits;                         // maximum number of hits
      u_long max_v_files;                        // files
      u_long max_v_pages;                        // pages
      double max_v_xfer;                         // and transfer amount per visit
      
      u_long t_hit;                              // monthly total vars: hits
      u_long t_file;                             // files
      u_long t_page;                             // pages
      u_long t_hosts;                            // hosts
      u_long t_hosts_conv;                       // converted hosts
      u_long t_url;                              // urls
      u_long t_ref;                              // referrers
      u_long t_agent;                            // user agents
      u_long t_user;                             // users
      u_long t_err;                              // errors
      u_long t_dlcount;                          // download count
      u_long t_downloads;                        // download record count
      u_long t_srchits;                          // search string hits
      u_long t_search;                           // search strings
      u_long t_entry;                            // entry page hits
      u_long t_exit;                             // exit page hits
      u_long u_entry;                            // entry pages
      u_long u_exit;                             // exit pages
      double t_xfer;                             // transfer

      u_long t_rhits;                            // robot hits
      u_long t_rfiles;                           // robot files
      u_long t_rpages;                           // robot pages
      u_long t_rerrors;                          // robot errors
      u_long t_rhosts;                           // robot hosts
      double t_rxfer;                            // robot transfer

      u_long t_visits;                           // visits started
      u_long t_rvisits;                          // robot visits started
      u_long t_visits_end;                       // visits ended
      u_long t_rvisits_end;                      // robot visits ended
      u_long t_svisits_end;                      // spammer visits ended
      u_long t_hvisits_end;                      // human visits ended
      u_long t_visits_conv;                      // converted visits ended (% of t_hvisits_end)
      
      u_long t_spmhits;                          // spammer hits
      u_long t_sfiles;                           // spammer files
      u_long t_spages;                           // spammer pages
      u_long t_shosts;                           // spammer hosts
      double t_sxfer;                            // spammer transfer

      double t_visit_avg;                        // average human visit length
      u_long t_visit_max;                        // maximum human visit length

      double t_vconv_avg;                        // average converted visit length
      u_long t_vconv_max;                        // maximum converted visit length

      u_long max_hv_hits;                        // maximum number of hits
      u_long max_hv_files;                       // files
      u_long max_hv_pages;                       // pages
      double max_hv_xfer;                        // and transfer amount per human visit

      u_long t_grp_hosts;                        // total host groups
      u_long t_grp_urls;                         // total url groups
      u_long t_grp_users;                        // total user groups
      u_long t_grp_refs;                         // total referrer groups
      u_long t_grp_agents;                       // total agent groups

	   double a_hitptime;                         // average and maximum
      double m_hitptime;		                   // hit, page and file       	
	   double a_fileptime;                        // processing time
      double m_fileptime;		
	   double a_pageptime;
      double m_pageptime;		

      u_long dt_hosts;                           // daily hosts total

      u_long hm_hit;                             // current hourly hits maximum
      
      u_long ht_hits;                            // current hourly counts
      u_long ht_files;
      u_long ht_pages;
      double ht_xfer;
      u_long ht_visits;
      u_long ht_hosts;

   protected:
      void init_counters(void);

   public:
      typedef void (*s_unpack_cb_t)(totals_t& state, void *arg);

   public:
      totals_t(void);

      //
      // serialization
      //
      u_int s_data_size(void) const;
      u_int s_pack_data(void *buffer, u_int bufsize) const;
      u_int s_unpack_data(const void *buffer, u_int bufsize, s_unpack_cb_t upcb, void *arg);

      static u_int s_data_size(const void *buffer);
};

#endif // __TOTALS_H
