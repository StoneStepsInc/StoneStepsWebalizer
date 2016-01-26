/*
    webalizer - a web server log analysis program

    Copyright (c) 2004-2016, Stone Steps Inc. (www.stonesteps.ca)

    See COPYING and Copyright files for additional licensing and copyright information

    totals.cpp
*/
#include "pch.h"

#include "totals.h"
#include "serialize.h"
#include "preserve.h"

totals_t::totals_t(void) : keynode_t<uint32_t>(1)
{
   a_hitptime = 0.0;							       /* average hit processing time */
   m_hitptime = 0.0;							       /* maximum hit processing time */
   a_fileptime = 0.0;
   m_fileptime = 0.0;
   a_pageptime = 0.0;
   m_pageptime = 0.0;

   max_v_xfer = 0;

   max_v_hits = 0;
   max_v_files = 0;
   max_v_pages = 0;

   max_hv_xfer = 0;

   max_hv_hits = 0;
   max_hv_files = 0;
   max_hv_pages = 0;

   t_visit_avg = .0;
   t_visit_max = 0;
   
   t_vconv_avg = .0;
   t_vconv_max = 0;

   ht_hits = 0;                                /* hourly hits totals       */
   ht_files = 0;
   ht_pages = 0;
   ht_xfer = 0;
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

   t_xfer = 0;                              /* monthly total xfer value */

   t_rhits = t_rfiles = t_rpages = t_rerrors = 0;
   t_rvisits = 0;
   t_rhosts = 0;
   t_rxfer = 0;
   
   t_shosts = 0;
   t_spmhits = t_sfiles = t_spages = 0;
   t_sxfer = 0;

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
   
   max_v_xfer = 0;
   max_hv_xfer = 0;
	a_hitptime = m_hitptime = a_fileptime = m_fileptime = a_pageptime = m_pageptime = 0.0;
   t_xfer = 0;
   
   f_day = l_day = 1;
   
   t_dlcount = 0;
   t_downloads = 0;
   t_srchits = 0;
   t_search = 0;
   
   t_entry = t_exit = 0;
   u_entry = u_exit = 0;
   
   ht_hits = ht_files = ht_pages = 0;
   ht_visits = ht_hosts = 0;
   ht_xfer = 0;
   hm_hit = 0;
   
   t_rhits = t_rfiles = t_rpages = t_rerrors = 0;
   t_rxfer = 0;
   
   t_spmhits = t_sfiles = t_spages = 0;
   t_sxfer = 0;

   t_grp_hosts = 0;
   t_grp_urls = 0;
   t_grp_users = 0;
   t_grp_refs = 0;
   t_grp_agents = 0;
}

//
// serialization
//

size_t totals_t::s_data_size(void) const
{
   return datanode_t<totals_t>::s_data_size() + 
            s_size_of(cur_tstamp)+     // cur_tstamp
            sizeof(u_int)  * 2   +     // f_day, l_day
            sizeof(uint64_t) * 3   +     // max_v_hits, max_v_files, max_v_pages
            sizeof(uint64_t)       +     // max_v_xfer
            sizeof(uint64_t) * 8   +     // t_hit, t_file, t_page, t_hosts, t_url, t_ref, t_agent, t_user
            sizeof(uint64_t) * 5   +     // t_err, t_dlcount, t_srchits, t_entry, t_exit
            sizeof(uint64_t) * 2   +     // u_entry, u_exit
            sizeof(uint64_t) * 2   +     // ht_hits, hm_hit
            sizeof(uint64_t)       +     // t_visits
            sizeof(uint64_t)       +     // t_xfer
            sizeof(double)       +     // t_visit_avg
            sizeof(double) * 6   +     // a_hitptime, m_hitptime, a_fileptime, m_fileptime, a_pageptime, m_pageptime
            sizeof(uint64_t) * 2   +     // t_visits_end, t_visit_max
            sizeof(uint64_t) * 2   +     // t_visits_conv, t_hosts_conv
            sizeof(uint64_t) * 6   +     // t_rhits, t_rfiles, t_rpages, t_rerrors, t_rvisits, t_rhosts
            sizeof(uint64_t)       +     // t_rxfer
            sizeof(double)       +     // t_vconv_avg
            sizeof(uint64_t)       +     // t_vconv_max
            sizeof(uint64_t) * 4   +     // ht_files, ht_pages, ht_visits, ht_hosts
            sizeof(uint64_t)       +     // ht_xfer
            sizeof(uint64_t) * 5   +     // t_hvisits_end, t_rvisits_end, t_svisits_end, t_search, t_downloads
            sizeof(uint64_t) * 5   +     // t_grp_hosts, t_grp_urls, t_grp_users, t_grp_refs, t_grp_agents
            sizeof(uint64_t)       +     // t_shosts
            sizeof(uint64_t) * 3   +     // t_spmhits, t_sfiles, t_spages
            sizeof(uint64_t)       +     // t_sxfer
            sizeof(uint64_t) * 3   +     // max_hv_hits, max_hv_files, max_hv_pages
            sizeof(uint64_t)       ;     // max_hv_xfer            
}

size_t totals_t::s_pack_data(void *buffer, size_t bufsize) const
{
   size_t datasize, basesize;
   void *ptr = buffer;

   basesize = datanode_t<totals_t>::s_data_size();
   datasize = s_data_size();

   if(bufsize < datasize)
      return 0;

   datanode_t<totals_t>::s_pack_data(buffer, bufsize);
   ptr = (u_char*) buffer + basesize;

   ptr = serialize(ptr, cur_tstamp);

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

size_t totals_t::s_unpack_data(const void *buffer, size_t bufsize, s_unpack_cb_t upcb, void *arg)
{
   u_short version;
   size_t datasize, basesize;
   const void *ptr = buffer;

   basesize = datanode_t<totals_t>::s_data_size(buffer);
   datasize = s_data_size(buffer);

   if(bufsize < datasize)
      return 0;

   version = s_node_ver(buffer);
   datanode_t<totals_t>::s_unpack_data(buffer, bufsize);
   ptr = (u_char*) buffer + basesize;

   if(version >= 6)
      ptr = deserialize(ptr, cur_tstamp);
   else {
      uint64_t tmp;
      ptr = deserialize(ptr, tmp);
      cur_tstamp.reset((time_t) tmp);
   }

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

   if(version < 7)
      ptr = s_skip_field<uint64_t>(ptr);     // dt_hosts

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
      t_rxfer = 0;
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
      ht_xfer = 0;
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
      t_sxfer = 0;

      max_hv_hits = 0;
      max_hv_files = 0;
      max_hv_pages = 0;
      max_hv_xfer = 0;
   }

   if(upcb)
      upcb(*this, arg);
   
   return datasize;
}

size_t totals_t::s_data_size(const void *buffer)
{
   u_short version = s_node_ver(buffer);
   size_t datasize = datanode_t<totals_t>::s_data_size(buffer);;
   
   if(version >= 6)
      datasize += s_size_of<tstamp_t>((u_char*) buffer + datasize);  // cur_tstamp
   else
      datasize += sizeof(uint64_t);   // cur_tstamp
  
   datasize += 
            sizeof(u_int)  *  2  +  // first, last day
            sizeof(uint64_t) *  3  +  // max hits, files, pages
            sizeof(uint64_t)       +  // max_v_xfer
            sizeof(uint64_t) *  8  +  // t_hit, t_file, t_page, t_hosts, t_url, t_ref, t_agent, t_user 
            sizeof(uint64_t) *  5  +  // t_err, t_dlcount, t_srchits, t_entry, t_exit
            sizeof(uint64_t) *  2  +  // tvisits, t_visits_end
            sizeof(uint64_t) *  2  +  // u_entry, u_exit
            sizeof(uint64_t)       +  // t_xfer
            sizeof(double)       +  // t_visit_avg
            sizeof(uint64_t)       +  // t_visit_max
            sizeof(double) *  6  +  // a_hitptime, m_hitptime, a_fileptime, m_fileptime, a_pageptime, m_pageptime 
            sizeof(uint64_t) *  2  ;  // ht_hits, hm_hit 

   if(version < 7)
      datasize += sizeof(uint64_t);    // dt_hosts (removed in v7)

   if(version < 2)
      return datasize;

   datasize += sizeof(uint64_t) * 5 + // t_rhits, t_rfiles, t_rpages, t_rerrors, t_rvisits 
            sizeof(uint64_t);         // t_rxfer (v2)
   
   if(version < 3)
      return datasize;
      
   datasize += sizeof(double)    +  // t_vconv_avg 
            sizeof(uint64_t) * 4   +  // t_visits_conv, t_hosts_conv, t_rhosts, t_vconv_max 
            sizeof(uint64_t) * 4   +  // ht_files, ht_pages, ht_visits, ht_hosts
            sizeof(uint64_t);         // ht_xfer

   if(version < 4)
      return datasize;
   
   datasize += 
            sizeof(uint64_t) * 5   +  // t_hvisits_end, t_rvisits_end, t_svisits_end, t_search, t_downloads
            sizeof(uint64_t) * 5   ;  // t_grp_hosts, t_grp_urls, t_grp_users, t_grp_refs, t_grp_agents
            
   if(version < 5)
      return datasize;
   
   datasize += 
            sizeof(uint64_t)       +  // t_shosts
            sizeof(uint64_t) * 3   +  // t_spmhits, t_sfiles, t_spages
            sizeof(uint64_t)       +  // t_sxfer
            sizeof(uint64_t) * 3   +  // max_hv_hits, max_vh_files, max_vh_pages
            sizeof(uint64_t)       ;  // max_hv_xfer

   return datasize;
}
