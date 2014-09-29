/*
    webalizer - a web server log analysis program

    Copyright (c) 2004-2014, Stone Steps Inc. (www.stonesteps.ca)

    See COPYING and Copyright files for additional licensing and copyright information

    totals.cpp
*/
#include "pch.h"

#include "totals.h"
#include "serialize.h"
#include "preserve.h"

totals_t::totals_t(void) : keynode_t<u_long>(1)
{
   cur_tstamp = 0;                            /* Timestamp...             */

   a_hitptime = 0.0;							       /* average hit processing time */
   m_hitptime = 0.0;							       /* maximum hit processing time */
   a_fileptime = 0.0;
   m_fileptime = 0.0;
   a_pageptime = 0.0;
   m_pageptime = 0.0;

   cur_year=0, cur_month=0;                   /* year/month/day/hour      */
   cur_day=0, cur_hour=0;                     /* tracking variables       */
   cur_min=0, cur_sec=0;

   max_v_xfer = 0.;

   max_v_hits = 0;
   max_v_files = 0;
   max_v_pages = 0;

   max_hv_xfer = 0.;

   max_hv_hits = 0;
   max_hv_files = 0;
   max_hv_pages = 0;

   t_visit_avg = .0;
   t_visit_max = 0;
   
   t_vconv_avg = .0;
   t_vconv_max = 0;

   dt_hosts = 0;                              /* daily hosts total      */

   ht_hits = 0;                                /* hourly hits totals       */
   ht_files = 0;
   ht_pages = 0;
   ht_xfer = .0;
   ht_visits = 0;
   ht_hosts = 0;
   hm_hit = 0;                               

   f_day = 1; 
   l_day = 1;

   t_hit=0;
   t_file=0;
   t_hosts = t_hosts_conv = 0;
               
   t_url = 0;                                 /* monthly total vars       */
   t_ref = 0;
   t_agent = 0;
   t_page = 0;
   
   t_visits = 0;
   t_visits_end = 0;
   t_hvisits_end = 0;
   t_rvisits_end = 0;
   t_svisits_end = 0;
   t_visits_conv = 0;
   
   t_user = 0;
   t_err = 0;

   t_dlcount = 0;
   t_downloads = 0;
   t_srchits = 0;
   t_search = 0;
   t_entry = 0;
   t_exit = 0;
   u_entry = 0;
   u_exit = 0;

   t_xfer = 0.0;                              /* monthly total xfer value */

   t_rhits = t_rfiles = t_rpages = t_rerrors = 0;
   t_rvisits = 0;
   t_rhosts = 0;
   t_rxfer = 0.0;
   
   t_shosts = 0;
   t_spmhits = t_sfiles = t_spages = 0;
   t_sxfer = .0;

   t_grp_hosts = 0;
   t_grp_urls = 0;
   t_grp_users = 0;
   t_grp_refs = 0;
   t_grp_agents = 0;
}

void totals_t::init_counters(void)
{
   t_hit = t_file = t_hosts_conv = t_url = t_ref = t_agent = t_page = t_user = t_err = 0;
   t_visits = t_visits_end = t_visits_conv = t_rvisits = t_hvisits_end = t_rvisits_end = t_svisits_end = 0;
   t_hosts = t_rhosts = t_shosts = 0;
   
   max_v_hits = max_v_files = max_v_pages = 0;
   max_hv_hits = max_hv_files = max_hv_pages = 0;
   
   t_visit_avg = t_vconv_avg = .0;
   t_visit_max = t_vconv_max = 0;
   
   max_v_xfer = 0.;
   max_hv_xfer = 0.;
	a_hitptime = m_hitptime = a_fileptime = m_fileptime = a_pageptime = m_pageptime = 0.0;
   t_xfer = 0.0;
   
   dt_hosts = 0;
   
   f_day = l_day = 1;
   
   t_dlcount = 0;
   t_downloads = 0;
   t_srchits = 0;
   t_search = 0;
   
   t_entry = t_exit = 0;
   u_entry = u_exit = 0;
   
   ht_hits = ht_files = ht_pages = 0;
   ht_visits = ht_hosts = 0;
   ht_xfer = .0;
   hm_hit = 0;
   
   t_rhits = t_rfiles = t_rpages = t_rerrors = 0;
   t_rxfer = 0.0;
   
   t_spmhits = t_sfiles = t_spages = 0;
   t_sxfer = .0;

   t_grp_hosts = 0;
   t_grp_urls = 0;
   t_grp_users = 0;
   t_grp_refs = 0;
   t_grp_agents = 0;
}

//
// serialization
//

u_int totals_t::s_data_size(void) const
{
   return datanode_t<totals_t>::s_data_size() + 
            sizeof(int64_t)      +     // cur_tstamp
            sizeof(u_int)  * 6   +     // cur_year, cur_month, cur_day, cur_hour cur_min, cur_sec
            sizeof(u_int)  * 2   +     // f_day, l_day
            sizeof(u_long) * 3   +     // max_v_hits, max_v_files, max_v_pages
            sizeof(double)       +     // max_v_xfer
            sizeof(u_long) * 8   +     // t_hit, t_file, t_page, t_hosts, t_url, t_ref, t_agent, t_user
            sizeof(u_long) * 5   +     // t_err, t_dlcount, t_srchits, t_entry, t_exit
            sizeof(u_long) * 2   +     // u_entry, u_exit
            sizeof(u_long) * 3   +     // dt_hosts, ht_hits, hm_hit
            sizeof(u_long)       +     // t_visits
            sizeof(double)       +     // t_xfer
            sizeof(double)       +     // t_visit_avg
            sizeof(double) * 6   +     // a_hitptime, m_hitptime, a_fileptime, m_fileptime, a_pageptime, m_pageptime
            sizeof(u_long) * 2   +     // t_visits_end, t_visit_max
            sizeof(u_long) * 2   +     // t_visits_conv, t_hosts_conv
            sizeof(u_long) * 6   +     // t_rhits, t_rfiles, t_rpages, t_rerrors, t_rvisits, t_rhosts
            sizeof(double)       +     // t_rxfer
            sizeof(double)       +     // t_vconv_avg
            sizeof(u_long)       +     // t_vconv_max
            sizeof(u_long) * 4   +     // ht_files, ht_pages, ht_visits, ht_hosts
            sizeof(double)       +     // ht_xfer
            sizeof(u_long) * 5   +     // t_hvisits_end, t_rvisits_end, t_svisits_end, t_search, t_downloads
            sizeof(u_long) * 5   +     // t_grp_hosts, t_grp_urls, t_grp_users, t_grp_refs, t_grp_agents
            sizeof(u_long)       +     // t_shosts
            sizeof(u_long) * 3   +     // t_spmhits, t_sfiles, t_spages
            sizeof(double)       +     // t_sxfer
            sizeof(u_long) * 3   +     // max_hv_hits, max_hv_files, max_hv_pages
            sizeof(double)       ;     // max_hv_xfer            
}

u_int totals_t::s_pack_data(void *buffer, u_int bufsize) const
{
   u_int datasize, basesize;
   void *ptr = buffer;

   basesize = datanode_t<totals_t>::s_data_size();
   datasize = s_data_size();

   if(bufsize < datasize)
      return 0;

   datanode_t<totals_t>::s_pack_data(buffer, bufsize);
   ptr = (u_char*) buffer + basesize;

   ptr = serialize<int64_t>(ptr, cur_tstamp);

   ptr = serialize(ptr, cur_year);
   ptr = serialize(ptr, cur_month);
   ptr = serialize(ptr, cur_day);
   ptr = serialize(ptr, cur_hour);
   ptr = serialize(ptr, cur_min);
   ptr = serialize(ptr, cur_sec);

   ptr = serialize(ptr, f_day);
   ptr = serialize(ptr, l_day);

   ptr = serialize(ptr, max_v_hits);
   ptr = serialize(ptr, max_v_files);
   ptr = serialize(ptr, max_v_pages);
   ptr = serialize(ptr, max_v_xfer);

   ptr = serialize(ptr, t_hit);
   ptr = serialize(ptr, t_file);
   ptr = serialize(ptr, t_page);
   ptr = serialize(ptr, t_hosts);
   ptr = serialize(ptr, t_url);
   ptr = serialize(ptr, t_ref);
   ptr = serialize(ptr, t_agent);
   ptr = serialize(ptr, t_visits);
   ptr = serialize(ptr, t_visits_end);
   ptr = serialize(ptr, t_user);
   ptr = serialize(ptr, t_err);
   ptr = serialize(ptr, t_dlcount);
   ptr = serialize(ptr, t_srchits);
   ptr = serialize(ptr, t_entry);
   ptr = serialize(ptr, t_exit);
   ptr = serialize(ptr, u_entry);
   ptr = serialize(ptr, u_exit);
   ptr = serialize(ptr, t_xfer);

   ptr = serialize(ptr, t_visit_avg);
   ptr = serialize(ptr, t_visit_max);

   ptr = serialize(ptr, a_hitptime);
   ptr = serialize(ptr, m_hitptime);
   ptr = serialize(ptr, a_fileptime);
   ptr = serialize(ptr, m_fileptime);
   ptr = serialize(ptr, a_pageptime);
   ptr = serialize(ptr, m_pageptime);

   ptr = serialize(ptr, dt_hosts);

   ptr = serialize(ptr, ht_hits);
   ptr = serialize(ptr, hm_hit);

   ptr = serialize(ptr, t_rhits);
   ptr = serialize(ptr, t_rfiles);
   ptr = serialize(ptr, t_rpages);
   ptr = serialize(ptr, t_rerrors);
   ptr = serialize(ptr, t_rvisits);
   ptr = serialize(ptr, t_rxfer);
   
   ptr = serialize(ptr, t_visits_conv);
   ptr = serialize(ptr, t_hosts_conv);
   ptr = serialize(ptr, t_rhosts);
   ptr = serialize(ptr, t_vconv_avg);
   ptr = serialize(ptr, t_vconv_max);
   
   ptr = serialize(ptr, ht_files);
   ptr = serialize(ptr, ht_pages);
   ptr = serialize(ptr, ht_xfer);
   ptr = serialize(ptr, ht_visits);
   ptr = serialize(ptr, ht_hosts);
   ptr = serialize(ptr, t_hvisits_end);
   ptr = serialize(ptr, t_rvisits_end);
   ptr = serialize(ptr, t_svisits_end);
   ptr = serialize(ptr, t_search);
   ptr = serialize(ptr, t_downloads);

   ptr = serialize(ptr, t_grp_hosts);
   ptr = serialize(ptr, t_grp_urls);
   ptr = serialize(ptr, t_grp_users);
   ptr = serialize(ptr, t_grp_refs);
   ptr = serialize(ptr, t_grp_agents);
   
   ptr = serialize(ptr, t_shosts);
   ptr = serialize(ptr, t_spmhits);
   ptr = serialize(ptr, t_sfiles);
   ptr = serialize(ptr, t_spages);
   ptr = serialize(ptr, t_sxfer);

   ptr = serialize(ptr, max_hv_hits);
   ptr = serialize(ptr, max_hv_files);
   ptr = serialize(ptr, max_hv_pages);
   ptr = serialize(ptr, max_hv_xfer);
         
   return datasize;
}

u_int totals_t::s_unpack_data(const void *buffer, u_int bufsize, s_unpack_cb_t upcb, void *arg)
{
   u_short version;
   u_int datasize, basesize;
   const void *ptr = buffer;

   basesize = datanode_t<totals_t>::s_data_size(buffer);
   datasize = s_data_size(buffer);

   if(bufsize < datasize)
      return 0;

   version = s_node_ver(buffer);
   datanode_t<totals_t>::s_unpack_data(buffer, bufsize);
   ptr = (u_char*) buffer + basesize;

   compatibility_deserializer<6, u_long, int64_t, time_t> deserialize_time_t(version);

   ptr = deserialize_time_t(ptr, cur_tstamp);

   ptr = deserialize(ptr, cur_year);
   ptr = deserialize(ptr, cur_month);
   ptr = deserialize(ptr, cur_day);
   ptr = deserialize(ptr, cur_hour);
   ptr = deserialize(ptr, cur_min);
   ptr = deserialize(ptr, cur_sec);

   ptr = deserialize(ptr, f_day);
   ptr = deserialize(ptr, l_day);

   ptr = deserialize(ptr, max_v_hits);
   ptr = deserialize(ptr, max_v_files);
   ptr = deserialize(ptr, max_v_pages);
   ptr = deserialize(ptr, max_v_xfer);

   ptr = deserialize(ptr, t_hit);
   ptr = deserialize(ptr, t_file);
   ptr = deserialize(ptr, t_page);
   ptr = deserialize(ptr, t_hosts);
   ptr = deserialize(ptr, t_url);
   ptr = deserialize(ptr, t_ref);
   ptr = deserialize(ptr, t_agent);
   ptr = deserialize(ptr, t_visits);
   ptr = deserialize(ptr, t_visits_end);
   ptr = deserialize(ptr, t_user);
   ptr = deserialize(ptr, t_err);
   ptr = deserialize(ptr, t_dlcount);
   ptr = deserialize(ptr, t_srchits);
   ptr = deserialize(ptr, t_entry);
   ptr = deserialize(ptr, t_exit);
   ptr = deserialize(ptr, u_entry);
   ptr = deserialize(ptr, u_exit);
   ptr = deserialize(ptr, t_xfer);

   ptr = deserialize(ptr, t_visit_avg);
   ptr = deserialize(ptr, t_visit_max);

   ptr = deserialize(ptr, a_hitptime);
   ptr = deserialize(ptr, m_hitptime);
   ptr = deserialize(ptr, a_fileptime);
   ptr = deserialize(ptr, m_fileptime);
   ptr = deserialize(ptr, a_pageptime);
   ptr = deserialize(ptr, m_pageptime);

   ptr = deserialize(ptr, dt_hosts);

   ptr = deserialize(ptr, ht_hits);
   ptr = deserialize(ptr, hm_hit);

   if(version >= 2) {
      ptr = deserialize(ptr, t_rhits);
      ptr = deserialize(ptr, t_rfiles);
      ptr = deserialize(ptr, t_rpages);
      ptr = deserialize(ptr, t_rerrors);
      ptr = deserialize(ptr, t_rvisits);
      ptr = deserialize(ptr, t_rxfer);
   }
   else {
      t_rhits = 0;
      t_rfiles = 0;
      t_rpages = 0;
      t_rerrors = 0;
      t_rvisits = 0;
      t_rxfer = .0;
   }
   
   if(version >= 3) {
      ptr = deserialize(ptr, t_visits_conv);
      ptr = deserialize(ptr, t_hosts_conv);
      ptr = deserialize(ptr, t_rhosts);
      ptr = deserialize(ptr, t_vconv_avg);
      ptr = deserialize(ptr, t_vconv_max);
      ptr = deserialize(ptr, ht_files);
      ptr = deserialize(ptr, ht_pages);
      ptr = deserialize(ptr, ht_xfer);
      ptr = deserialize(ptr, ht_visits);
      ptr = deserialize(ptr, ht_hosts);
   }
   else {
      t_visits_conv = 0;
      t_hosts_conv = 0;
      t_rhosts = 0;
      t_vconv_avg = .0;
      t_vconv_max = 0;
      ht_files = ht_pages = 0;
      ht_xfer = .0;
      ht_visits = 0;
      ht_hosts = 0;
   }
   
   if(version >= 4) {
      ptr = deserialize(ptr, t_hvisits_end);
      ptr = deserialize(ptr, t_rvisits_end);
      ptr = deserialize(ptr, t_svisits_end);
      ptr = deserialize(ptr, t_search);
      ptr = deserialize(ptr, t_downloads);

      ptr = deserialize(ptr, t_grp_hosts);
      ptr = deserialize(ptr, t_grp_urls);
      ptr = deserialize(ptr, t_grp_users);
      ptr = deserialize(ptr, t_grp_refs);
      ptr = deserialize(ptr, t_grp_agents);
   }
   else {
      // reduce the error by assigning closest existing values
      t_hvisits_end = t_visits_end;
      t_rvisits_end = t_rvisits;
      t_svisits_end = 0;
      
      // set these to zero, so they can be populated in state_t::restore_state_ex
      t_search = 0;
      t_downloads = 0;
      
      t_grp_hosts = 0;
      t_grp_urls = 0;
      t_grp_users = 0;
      t_grp_refs = 0;
      t_grp_agents = 0;
   }
   
   if(version >= 5) {
      ptr = deserialize(ptr, t_shosts);
      ptr = deserialize(ptr, t_spmhits);
      ptr = deserialize(ptr, t_sfiles);
      ptr = deserialize(ptr, t_spages);
      ptr = deserialize(ptr, t_sxfer);

      ptr = deserialize(ptr, max_hv_hits);
      ptr = deserialize(ptr, max_hv_files);
      ptr = deserialize(ptr, max_hv_pages);
      ptr = deserialize(ptr, max_hv_xfer);
   }
   else {
      t_spmhits = 0;
      t_sfiles = 0;
      t_spages = 0;
      t_shosts = 0;
      t_sxfer = .0;

      max_hv_hits = 0;
      max_hv_files = 0;
      max_hv_pages = 0;
      max_hv_xfer = 0;
   }

   if(upcb)
      upcb(*this, arg);
   
   return datasize;
}

u_int totals_t::s_data_size(const void *buffer)
{
   u_short version = s_node_ver(buffer);
   u_long datasize = datanode_t<totals_t>::s_data_size(buffer) + 
            sizeof(u_int)  *  6  +  // year, month, day, hour, minute, second
            sizeof(u_int)  *  2  +  // first, last day
            sizeof(u_long) *  3  +  // max hits, files, pages
            sizeof(double)       +  // max_v_xfer
            sizeof(u_long) *  8  +  // t_hit, t_file, t_page, t_hosts, t_url, t_ref, t_agent, t_user 
            sizeof(u_long) *  5  +  // t_err, t_dlcount, t_srchits, t_entry, t_exit
            sizeof(u_long) *  2  +  // tvisits, t_visits_end
            sizeof(u_long) *  2  +  // u_entry, u_exit
            sizeof(double)       +  // t_xfer
            sizeof(double)       +  // t_visit_avg
            sizeof(u_long)       +  // t_visit_max
            sizeof(double) *  6  +  // a_hitptime, m_hitptime, a_fileptime, m_fileptime, a_pageptime, m_pageptime 
            sizeof(u_long)       +  // dt_hosts
            sizeof(u_long) *  2  ;  // ht_hits, hm_hit 

   if(version < 2)
      return datasize +
            sizeof(u_long);         // cur_tstamp

   datasize += sizeof(u_long) * 5 + // t_rhits, t_rfiles, t_rpages, t_rerrors, t_rvisits 
            sizeof(double);         // t_rxfer (v2)
   
   if(version < 3)
      return datasize +
            sizeof(u_long);         // cur_tstamp
      
   datasize += sizeof(double)    +  // t_vconv_avg 
            sizeof(u_long) * 4   +  // t_visits_conv, t_hosts_conv, t_rhosts, t_vconv_max 
            sizeof(u_long) * 4   +  // ht_files, ht_pages, ht_visits, ht_hosts
            sizeof(double);         // ht_xfer

   if(version < 4)
      return datasize +
            sizeof(u_long);         // cur_tstamp
   
   datasize += 
            sizeof(u_long) * 5   +  // t_hvisits_end, t_rvisits_end, t_svisits_end, t_search, t_downloads
            sizeof(u_long) * 5   ;  // t_grp_hosts, t_grp_urls, t_grp_users, t_grp_refs, t_grp_agents
            
   if(version < 5)
      return datasize + 
            sizeof(u_long);         // cur_tstamp
   
   datasize += 
            sizeof(u_long)       +  // t_shosts
            sizeof(u_long) * 3   +  // t_spmhits, t_sfiles, t_spages
            sizeof(double)       +  // t_sxfer
            sizeof(u_long) * 3   +  // max_hv_hits, max_vh_files, max_vh_pages
            sizeof(double)       ;  // max_hv_xfer

   if(version < 6)   
      return datasize + 
            sizeof(u_long)       ;  // cur_tstamp

   return datasize + 
            sizeof(int64_t)      ;  // cur_tstamp
}
