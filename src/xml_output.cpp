/*
   webalizer - a web server log analysis program

   Copyright (c) 2004-2016, Stone Steps Inc. (www.stonesteps.ca)

   See COPYING and Copyright files for additional licensing and copyright information 
   
   xml_output.cpp
*/
#include "pch.h"

#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#ifndef _WIN32
#include <unistd.h>
#endif

#include <sys/types.h>

/* some systems need this */
#ifdef HAVE_MATH_H
#include <math.h>
#endif

#include "lang.h"
#include "hashtab.h"
#include "linklist.h"
#include "util.h"
#include "exception.h"
#include "xml_output.h"
#include "preserve.h"
#include "history.h"

// -----------------------------------------------------------------------
// multi_reverse_iterator
//
// A template iterator class containing multiple database_t reverse 
// iterators.
//
template <template<typename node_t> class iterator_t, typename node_t>
class multi_reverse_iterator {
   private:
      struct itnode_t : public list_node_t<itnode_t> {
         iterator_t<node_t>   *iter;
         
         itnode_t(iterator_t<node_t> *iter) : iter(iter) {}
      };
   private:
      list_t<itnode_t> iterators;
   
   public:
      void push(iterator_t<node_t> *iter) 
      {
         if(iter) 
            iterators.add_tail(new itnode_t(iter));
      }
      
      bool prev(node_t& node, typename node_t::s_unpack_cb_t upcb = NULL, void *arg = NULL)
      {
         while(iterators.size()) {
            if(iterators.get_head()->iter->prev(node, upcb, arg))
               return true;
               
            delete iterators.remove_head();
         }
         
         return false;
      }
};

//
//
//
xml_output_t::xml_output_t(const config_t& config, const state_t& state) : output_t(config, state), graph(config), xml_encoder(buffer, BUFSIZE, xml_encoder_t::overwrite), xml_encode(xml_encoder)
{
}

xml_output_t::~xml_output_t(void)
{
}

bool xml_output_t::init_output_engine(void)
{
   // initialize the graph engine even if we don't need to make images (e.g. read configuration)
   graph.init_graph_engine();

   return true;
}

void xml_output_t::cleanup_output_engine(void)
{
   graph.cleanup_graph_engine();
}

int xml_output_t::write_main_index()
{
   string_t index_fname, png_fname, title;
   u_int days_in_month;
   uint64_t gt_hit=0;
   uint64_t gt_files=0;
   uint64_t gt_pages=0;
   uint64_t gt_xfer=0;
   uint64_t gt_visits=0;
   uint64_t gt_hosts=0;
   const hist_month_t *hptr;
   history_t::const_reverse_iterator iter;

   index_fname = "index.xml";
   png_fname = "usage.png";

   if((out_fp = open_out_file(index_fname)) == NULL)
      return 1;

   if(config.html_ext_lang)
      png_fname += '.' + config.lang.language_code;

   title.format("%s %s", config.lang.msg_main_us, config.hname.c_str());

   if(makeimgs)
      graph.year_graph6x(state.history, png_fname, title, graphinfo->usage_width, graphinfo->usage_height);

   write_xml_head(true);

   fprintf(out_fp, "<report id=\"summary\" help=\"yearly_summary_report\" title=\"%s\">\n", config.lang.msg_main_sum);

   /* year graph */
	fprintf(out_fp, "<graph title=\"%s\">\n", title.c_str());
   fprintf(out_fp, "<image filename=\"%s\" width=\"%d\" height=\"%d\"/>\n", png_fname.c_str(), graphinfo->usage_width, graphinfo->usage_height);
   
   // output all history months for XSL to iterate through
   fputs("<months>", out_fp);
   for(u_int i = 0, m = state.history.first_month()-1; i < state.history.disp_length(); i++)
      fprintf(out_fp, "<month title=\"%s\"/>", lang_t::s_month[(m+i)%12]);
   fputs("</months>\n", out_fp);
   
	fputs("</graph>\n", out_fp);

	// output the columns section
	fputs("<columns>\n", out_fp);
   fprintf(out_fp,"<col title=\"%s\"/>\n", config.lang.msg_h_mth);
   
   fprintf(out_fp,"<col title=\"%s\" help=\"hits\" class=\"hits\"><columns><col title=\"%s\"/><col title=\"%s\"/></columns></col>\n", config.lang.msg_h_hits, config.lang.msg_h_avg, config.lang.msg_h_total);
   fprintf(out_fp,"<col title=\"%s\" help=\"files\" class=\"files\"><columns><col title=\"%s\"/><col title=\"%s\"/></columns></col>\n", config.lang.msg_h_files, config.lang.msg_h_avg, config.lang.msg_h_total);
   fprintf(out_fp,"<col title=\"%s\" help=\"pages\" class=\"pages\"><columns><col title=\"%s\"/><col title=\"%s\"/></columns></col>\n", config.lang.msg_h_pages, config.lang.msg_h_avg, config.lang.msg_h_total);
   fprintf(out_fp,"<col title=\"%s\" help=\"visits\" class=\"visits\"><columns><col title=\"%s\"/><col title=\"%s\"/></columns></col>\n", config.lang.msg_h_visits, config.lang.msg_h_avg, config.lang.msg_h_total);
   fprintf(out_fp,"<col title=\"%s\" help=\"hosts\" class=\"hosts\"><columns><col title=\"%s\"/><col title=\"%s\"/></columns></col>\n", config.lang.msg_h_hosts, config.lang.msg_h_avg, config.lang.msg_h_total);
   fprintf(out_fp,"<col title=\"%s\" help=\"transfer\" class=\"xfer\"><columns><col title=\"%s\"/><col title=\"%s\"/></columns></col>\n", config.lang.msg_h_xfer, config.lang.msg_h_avg, config.lang.msg_h_total);

	fputs("</columns>\n", out_fp);

	/*
	 * Summary data section
	 */
	fputs("<rows>\n", out_fp);

   iter = state.history.rbegin();
   while(iter != state.history.rend()) {
      hptr = &*iter++;
      if(hptr->hits==0) continue;
      days_in_month=(hptr->lday-hptr->fday)+1;
      
      fprintf(out_fp, "<row count=\"%d\" rowid=\"%d\">\n", days_in_month, state.history.month_index(hptr->year, hptr->month)+1);
      
      fprintf(out_fp,"<data url=\"usage_%04d%02d.xml\"><value>%s %d</value></data>\n", hptr->year, hptr->month, lang_t::s_month[hptr->month-1], hptr->year);
      fprintf(out_fp,"<data><avg>%" PRIu64 "</avg><sum>%" PRIu64 "</sum></data>\n", hptr->hits/days_in_month, hptr->hits);
      fprintf(out_fp,"<data><avg>%" PRIu64 "</avg><sum>%" PRIu64 "</sum></data>\n", hptr->files/days_in_month, hptr->files);
      fprintf(out_fp,"<data><avg>%" PRIu64 "</avg><sum>%" PRIu64 "</sum></data>\n", hptr->pages/days_in_month, hptr->pages);
      fprintf(out_fp,"<data><avg>%" PRIu64 "</avg><sum>%" PRIu64 "</sum></data>\n", hptr->visits/days_in_month, hptr->visits);
      fprintf(out_fp,"<data><avg>%" PRIu64 "</avg><sum>%" PRIu64 "</sum></data>\n", hptr->hosts/days_in_month, hptr->hosts);
      fprintf(out_fp,"<data><avg>%.0f</avg><sum>%.0f</sum></data>\n", hptr->xfer/1024./days_in_month, hptr->xfer/1024.);
      fputs("</row>\n", out_fp);

      gt_hit   += hptr->hits;
      gt_files += hptr->files;
      gt_pages += hptr->pages;
      gt_xfer  += hptr->xfer;
      gt_visits+= hptr->visits;
      gt_hosts += hptr->hosts;
   }
	fputs("</rows>\n", out_fp);

	/*
	 *	Summary totals section
	 */
	fprintf(out_fp,"<totals title=\"%s\">\n", config.lang.msg_h_totals);
   fprintf(out_fp,"<data><avg/><sum>%" PRIu64 "</sum></data>\n", gt_hit);
   fprintf(out_fp,"<data><avg/><sum>%" PRIu64 "</sum></data>\n", gt_files);
   fprintf(out_fp,"<data><avg/><sum>%" PRIu64 "</sum></data>\n", gt_pages);
   fprintf(out_fp,"<data><avg/><sum>%" PRIu64 "</sum></data>\n", gt_visits);
   fprintf(out_fp,"<data><avg/><sum>%" PRIu64 "</sum></data>\n", gt_hosts);
   fprintf(out_fp,"<data><avg/><sum>%.0f</sum></data>\n", gt_xfer/1024.);
	fputs("</totals>\n", out_fp);

   fprintf(out_fp,"<notes>\n<note>%s</note>\n</notes>\n", xml_encode(config.lang.msg_misc_pages));
   fputs("</report>\n", out_fp);

   write_xml_tail();

   fclose(out_fp);

   return 0;
}

void xml_output_t::write_monthly_totals(void)
{
   u_int i, days_in_month;
   uint64_t max_files=0,max_hits=0,max_visits=0,max_pages=0;
   uint64_t max_xfer=0;

   days_in_month=(state.totals.l_day-state.totals.f_day)+1;
   for (i=0;i<31;i++)
   {  /* Get max/day values */
      if (state.t_daily[i].tm_hits > max_hits)     max_hits  = state.t_daily[i].tm_hits;
      if (state.t_daily[i].tm_files > max_files)   max_files = state.t_daily[i].tm_files;
      if (state.t_daily[i].tm_pages > max_pages)   max_pages = state.t_daily[i].tm_pages;
      if (state.t_daily[i].tm_visits > max_visits) max_visits= state.t_daily[i].tm_visits;
      if (state.t_daily[i].tm_xfer > max_xfer)     max_xfer  = state.t_daily[i].tm_xfer;
   }

	xml_encoder.set_scope_mode(xml_encoder_t::append),
	fprintf(out_fp, "<report id=\"monthly_totals\" help=\"monthly_summary_report\" title=\"%s %s %d\">\n", xml_encode(config.lang.msg_mtot_ms), xml_encode(lang_t::l_month[state.totals.cur_tstamp.month-1]), state.totals.cur_tstamp.year);
	
	fputs("<section>\n", out_fp);
	fprintf(out_fp,"<columns><col help=\"totals_section\" title=\"%s\"/><col/></columns>\n", xml_encode(config.lang.msg_h_totals));

   /* Total Hits */
   fprintf(out_fp,"<data name=\"t_hits\" help=\"hits\" title=\"%s\"><sum>%" PRIu64 "</sum></data>\n", xml_encode(config.lang.msg_h_hits), state.totals.t_hit);
   /* Total Files */
   fprintf(out_fp,"<data name=\"t_files\" help=\"files\" title=\"%s\"><sum>%" PRIu64 "</sum></data>\n", xml_encode(config.lang.msg_h_files), state.totals.t_file);
   
   /* Total Pages */
   fprintf(out_fp,"<data name=\"t_pages\" help=\"pages\" title=\"%s\"><sum>%" PRIu64 "</sum></data>\n", xml_encode(config.lang.msg_h_pages), state.totals.t_page);
   /* Total Visits */
   fprintf(out_fp,"<data name=\"t_visits\" help=\"visits\" title=\"%s\"><sum>%" PRIu64 "</sum></data>\n", xml_encode(config.lang.msg_h_visits), state.totals.t_visits);
   
   /* Total XFer */
   fprintf(out_fp,"<data name=\"t_xfer\" help=\"transfer\" title=\"%s\"><sum>%.0f</sum></data>\n", xml_encode(config.lang.msg_h_xfer), state.totals.t_xfer/1024.);
   
   // Total Downloads (skip if download groups are not configured)
   if(config.downloads.size())
      fprintf(out_fp,"<data name=\"t_downloads\" help=\"downloads\" title=\"%s\"><sum>%" PRIu64 "</sum></data>\n", xml_encode(config.lang.msg_h_downloads), state.totals.t_downloads);
   
   /**********************************************/

   /* Unique Hosts */
   fprintf(out_fp,"<data name=\"t_hosts\" help=\"hosts\" title=\"%s\"><sum>%" PRIu64 "</sum></data>\n", xml_encode(config.lang.msg_h_hosts), state.totals.t_hosts);
   /* Unique URL's */
   fprintf(out_fp,"<data name=\"t_urls\" help=\"urls\" title=\"%s\"><sum>%" PRIu64 "</sum></data>\n", xml_encode(config.lang.msg_h_urls), state.totals.t_url);
   /* Unique Referrers */
	fprintf(out_fp,"<data name=\"t_referrers\" help=\"referrers\" title=\"%s\"><sum>%" PRIu64 "</sum></data>\n", xml_encode(config.lang.msg_h_refs), state.totals.t_ref);

   /* Unique Usernames */
   if (state.totals.t_user)
	   fprintf(out_fp,"<data name=\"t_users\" help=\"users\" title=\"%s\"><sum>%" PRIu64 "</sum></data>\n", xml_encode(config.lang.msg_mtot_ui), state.totals.t_user);
	   
   /* Unique Agents */
	fprintf(out_fp,"<data name=\"t_agents\" help=\"user_agents\" title=\"%s\"><sum>%" PRIu64 "</sum></data>\n", xml_encode(config.lang.msg_h_agents), state.totals.t_agent);

   fputs("</section>\n", out_fp);

   // output human totals if robot or spammer filters are configured
   if(config.spam_refs.size() || config.robots.size()) {

	   fputs("<section>\n", out_fp);
	   fprintf(out_fp, "<columns><col help=\"human_totals_section\" title=\"%s\"/><col/></columns>\n", xml_encode(config.lang.msg_mtot_htot));

      fprintf(out_fp,"<data name=\"t_hhits\" help=\"hits\" title=\"%s\"><sum>%" PRIu64 "</sum></data>\n", xml_encode(config.lang.msg_h_hits), state.totals.t_hit - state.totals.t_rhits - state.totals.t_spmhits);
      fprintf(out_fp,"<data name=\"t_hfiles\" help=\"files\" title=\"%s\"><sum>%" PRIu64 "</sum></data>\n", xml_encode(config.lang.msg_h_files), state.totals.t_file - state.totals.t_rfiles - state.totals.t_sfiles);
      fprintf(out_fp,"<data name=\"t_hpages\" help=\"pages\" title=\"%s\"><sum>%" PRIu64 "</sum></data>\n", xml_encode(config.lang.msg_h_pages), state.totals.t_page - state.totals.t_rpages - state.totals.t_spages);
      fprintf(out_fp,"<data name=\"t_hxfer\" help=\"transfer\" title=\"%s\"><sum>%.0f</sum></data>\n", xml_encode(config.lang.msg_h_xfer), (state.totals.t_xfer - state.totals.t_rxfer - state.totals.t_sxfer)/1024.);

      /* Total Non-Robot Hosts */
      fprintf(out_fp,"<data name=\"t_hhosts\" help=\"hosts\" title=\"%s\"><sum>%" PRIu64 "</sum></data>\n", xml_encode(config.lang.msg_h_hosts), state.totals.t_hosts - state.totals.t_rhosts - state.totals.t_shosts);

      // Total Human Visits
      fprintf(out_fp,"<data name=\"t_hvisits\" help=\"visits\" title=\"%s\"><sum>%" PRIu64 "</sum></data>\n", xml_encode(config.lang.msg_h_visits), state.totals.t_hvisits_end);

      // output the conversion section only if target URLs or downloads are configured
      if(config.target_urls.size() || config.downloads.size()) {
         /* Unique Converted Hosts */
         fprintf(out_fp,"<data name=\"t_hcvhosts\" help=\"converted_hosts\" title=\"%s\"><sum>%" PRIu64 "</sum></data>\n", xml_encode(config.lang.msg_h_chosts), state.totals.t_hosts_conv);
         /* Total Converted Visits */
         fprintf(out_fp,"<data name=\"t_hcvvisits\" help=\"converted_visits\" title=\"%s\"><sum>%" PRIu64 "</sum></data>\n", xml_encode(config.lang.msg_h_cvisits), state.totals.t_visits_conv);
         /* Host Conversion Rate */
         fprintf(out_fp,"<data name=\"t_cvrate\" help=\"host_conversion_rate\" title=\"%s\"><value>%.2f</value></data>\n", xml_encode(config.lang.msg_mtot_hcr), (double)state.totals.t_hosts_conv*100./(state.totals.t_hosts - state.totals.t_rhosts - state.totals.t_shosts));
      }
      
      fputs("</section>\n", out_fp);

      // output human per-visit totals if there are ended human visits
      if(state.totals.t_hvisits_end) {
	      fputs("<section extension=\"yes\">\n", out_fp);

			xml_encoder.set_scope_mode(xml_encoder_t::append),
	      fprintf(out_fp,"<columns><col/><col title=\"%s\"/><col title=\"%s\"/></columns>\n", xml_encode(config.lang.msg_h_avg), xml_encode(config.lang.msg_h_max));
	      
         fprintf(out_fp,"<data name=\"t_hphvisit\" help=\"hits_per_visit\" title=\"%s\"><avg>%" PRIu64 "</avg><max>%" PRIu64 "</max></data>\n", xml_encode(config.lang.msg_mtot_mhv), (state.totals.t_hit - state.totals.t_rhits - state.totals.t_spmhits)/state.totals.t_hvisits_end, state.totals.max_hv_hits);
         fprintf(out_fp,"<data name=\"t_fphvisit\" help=\"files_per_visit\" title=\"%s\"><avg>%" PRIu64 "</avg><max>%" PRIu64 "</max></data>\n", xml_encode(config.lang.msg_mtot_mfv), (state.totals.t_file - state.totals.t_rfiles - state.totals.t_sfiles)/state.totals.t_hvisits_end, state.totals.max_hv_files);
         fprintf(out_fp,"<data name=\"t_pphvisit\" help=\"pages_per_visit\" title=\"%s\"><avg>%" PRIu64 "</avg><max>%" PRIu64 "</max></data>\n", xml_encode(config.lang.msg_mtot_mpv), (state.totals.t_page - state.totals.t_rpages - state.totals.t_spages)/state.totals.t_hvisits_end, state.totals.max_hv_pages);
         fprintf(out_fp,"<data name=\"t_xphvisit\" help=\"transfer_per_visit\" title=\"%s\"><avg>%.0f</avg><max>%.0f</max></data>\n", xml_encode(config.lang.msg_mtot_mkv), ((state.totals.t_xfer - state.totals.t_rxfer - state.totals.t_sxfer)/1024.)/state.totals.t_hvisits_end, state.totals.max_hv_xfer/1024.);
         
         fprintf(out_fp,"<data name=\"t_vlength\" help=\"visit_duration\" title=\"%s\"><avg>%.02f</avg><max>%.02f</max></data>\n", xml_encode(config.lang.msg_mtot_mdv), state.totals.t_visit_avg/60., state.totals.t_visit_max/60.);

         if(state.totals.t_visits_conv)
            fprintf(out_fp,"<data name=\"t_cvvisits\" help=\"visit_duration\" title=\"%s\"><avg>%.02f</avg><max>%.02f</max></data>\n", xml_encode(config.lang.msg_mtot_cvd), state.totals.t_vconv_avg/60., state.totals.t_vconv_max/60.);
            
         fputs("</section>\n", out_fp);
      }
   }

   // output the robot section only if robots are configured
   if(config.robots.size()) {
	   fputs("<section>\n", out_fp);
	   fprintf(out_fp,"<columns><col help=\"robot_totals_section\" title=\"%s\"/><col/></columns>\n", xml_encode(config.lang.msg_mtot_rtot));
   		
      // Robot Totals
      fprintf(out_fp,"<data name=\"t_rhits\" help=\"hits\" title=\"%s\"><sum>%" PRIu64 "</sum></data>\n", xml_encode(config.lang.msg_h_hits), state.totals.t_rhits);
      fprintf(out_fp,"<data name=\"t_rfiles\" help=\"files\" title=\"%s\"><sum>%" PRIu64 "</sum></data>\n", xml_encode(config.lang.msg_h_files), state.totals.t_rfiles);
      fprintf(out_fp,"<data name=\"t_rpages\" help=\"pages\" title=\"%s\"><sum>%" PRIu64 "</sum></data>\n", xml_encode(config.lang.msg_h_pages), state.totals.t_rpages);
      fprintf(out_fp,"<data name=\"t_rerrors\" help=\"errors\" title=\"%s\"><sum>%" PRIu64 "</sum></data>\n", xml_encode(config.lang.msg_h_errors), state.totals.t_rerrors);
      fprintf(out_fp,"<data name=\"t_rxfer\" help=\"transfer\" title=\"%s\"><sum>%.0f</sum></data>\n", xml_encode(config.lang.msg_h_xfer), state.totals.t_rxfer/1024.);
      fprintf(out_fp,"<data name=\"t_rvisits\" help=\"visits\" title=\"%s\"><sum>%" PRIu64 "</sum></data>\n", xml_encode(config.lang.msg_h_visits), state.totals.t_rvisits_end);
      fprintf(out_fp,"<data name=\"t_rhosts\" help=\"hosts\" title=\"%s\"><sum>%" PRIu64 "</sum></data>\n", xml_encode(config.lang.msg_h_hosts), state.totals.t_rhosts);

      fputs("</section>\n", out_fp);
   }

   // Spammer Totals
   if(config.spam_refs.size()) {
	   fputs("<section>\n", out_fp);
	   fprintf(out_fp,"<columns><col help=\"spammer_totals_section\" title=\"%s\"/><col/></columns>\n", xml_encode(config.lang.msg_mtot_stot));

      fprintf(out_fp,"<data name=\"t_spmhits\" help=\"hits\" title=\"%s\"><sum>%" PRIu64 "</sum></data>\n", xml_encode(config.lang.msg_h_hits), state.totals.t_spmhits);
      fprintf(out_fp,"<data name=\"t_sxfer\" help=\"transfer\" title=\"%s\"><sum>%.0f</sum></data>\n", xml_encode(config.lang.msg_h_xfer), state.totals.t_sxfer/1024.);

      fprintf(out_fp,"<data name=\"t_svisits\" help=\"visits\" title=\"%s\"><sum>%" PRIu64 "</sum></data>\n", xml_encode(config.lang.msg_h_visits), state.totals.t_svisits_end);
      fprintf(out_fp,"<data name=\"t_shosts\" help=\"hosts\" title=\"%s\"><sum>%" PRIu64 "</sum></data>\n", xml_encode(config.lang.msg_h_hosts), state.totals.t_shosts);

      fputs("</section>\n", out_fp);
   }

   /* Hit/file/page processing time (output only if there's data) */
   if(state.totals.m_hitptime) {
	   fputs("<section>\n", out_fp);

		xml_encoder.set_scope_mode(xml_encoder_t::append),
	   fprintf(out_fp,"<columns><col help=\"performance_section\" title=\"%s\"/><col title=\"%s\"/><col title=\"%s\"/></columns>\n", xml_encode(config.lang.msg_mtot_perf), xml_encode(config.lang.msg_h_avg), xml_encode(config.lang.msg_h_max));

	   fprintf(out_fp,"<data name=\"t_htime\" help=\"hit_proc_time\" title=\"%s\"><avg>%.3f</avg><max>%.3f</max></data>\n", xml_encode(config.lang.msg_mtot_sph), state.totals.a_hitptime, state.totals.m_hitptime);
	   fprintf(out_fp,"<data name=\"t_ftime\" help=\"file_proc_time\" title=\"%s\"><avg>%.3f</avg><max>%.3f</max></data>\n", xml_encode(config.lang.msg_mtot_spf), state.totals.a_fileptime, state.totals.m_fileptime);
	   fprintf(out_fp,"<data name=\"t_ptime\" help=\"page_proc_time\" title=\"%s\"><avg>%.3f</avg><max>%.3f</max></data>\n", xml_encode(config.lang.msg_mtot_spp), state.totals.a_pageptime, state.totals.m_pageptime);
	   
      fputs("</section>\n", out_fp);
	}

	fputs("<section>\n", out_fp);

	xml_encoder.set_scope_mode(xml_encoder_t::append),
	fprintf(out_fp,"<columns><col help=\"hourly_daily_totals_section\" title=\"%s\"/><col title=\"%s\"/><col title=\"%s\"/></columns>\n", xml_encode(config.lang.msg_mtot_hdt), xml_encode(config.lang.msg_h_avg), xml_encode(config.lang.msg_h_max));

   /* Hourly/Daily avg/max totals */

   /* Max/Avg Hits per Hour */
   fprintf(out_fp,"<data name=\"t_hphour\" help=\"hits_per_hour\" title=\"%s\"><avg>%" PRIu64 "</avg><max>%" PRIu64 "</max></data>\n", xml_encode(config.lang.msg_mtot_mhh), state.totals.t_hit/(24*days_in_month), state.totals.hm_hit);
   /* Max/Avg Hits per Day */
   fprintf(out_fp,"<data name=\"t_hpday\" help=\"hits_per_day\" title=\"%s\"><avg>%" PRIu64 "</avg><max>%" PRIu64 "</max></data>\n", xml_encode(config.lang.msg_mtot_mhd), state.totals.t_hit/days_in_month, max_hits);
   /* Max/Avg Hits per Visit */
   if(state.totals.t_visits)
      fprintf(out_fp,"<data name=\"t_hpvisit\" help=\"hits_per_visit\" title=\"%s\"><avg>%" PRIu64 "</avg><max>%" PRIu64 "</max></data>\n", xml_encode(config.lang.msg_mtot_mhv), state.totals.t_hit/state.totals.t_visits, state.totals.max_v_hits);

   /* Max/Avg Files per Day */
   fprintf(out_fp,"<data name=\"t_fpday\" help=\"files_per_day\" title=\"%s\"><avg>%" PRIu64 "</avg><max>%" PRIu64 "</max></data>\n", xml_encode(config.lang.msg_mtot_mfd), state.totals.t_file/days_in_month, max_files);
   /* Max/Avg Files per Visit */
   if(state.totals.t_visits)
      fprintf(out_fp,"<data name=\"t_fpvisit\" help=\"files_per_visit\" title=\"%s\"><avg>%" PRIu64 "</avg><max>%" PRIu64 "</max></data>\n", xml_encode(config.lang.msg_mtot_mfv), state.totals.t_file/state.totals.t_visits, state.totals.max_v_files);

   /* Max/Avg Pages per Day */
   fprintf(out_fp,"<data name=\"t_ppday\" help=\"pages_per_day\" title=\"%s\"><avg>%" PRIu64 "</avg><max>%" PRIu64 "</max></data>\n", xml_encode(config.lang.msg_mtot_mpd), state.totals.t_page/days_in_month, max_pages);
   /* Max/Avg Pages per Visit */
   if(state.totals.t_visits)
      fprintf(out_fp,"<data name=\"t_ppvisit\" help=\"pages_per_visit\" title=\"%s\"><avg>%" PRIu64 "</avg><max>%" PRIu64 "</max></data>\n", xml_encode(config.lang.msg_mtot_mpv), state.totals.t_page/state.totals.t_visits, state.totals.max_v_pages);

   /* Max/Avg KBytes per Day */
   fprintf(out_fp,"<data name=\"t_xpday\" help=\"transfer_per_day\" title=\"%s\"><avg>%.0f</avg><max>%.0f</max></data>\n", xml_encode(config.lang.msg_mtot_mkd), (state.totals.t_xfer/1024.)/days_in_month,max_xfer/1024.);
   /* Max/Avg KBytes per Visit */
   if(state.totals.t_visits)
      fprintf(out_fp,"<data name=\"t_xpvisit\" help=\"transfer_per_visit\" title=\"%s\"><avg>%.0f</avg><max>%.0f</max></data>\n", xml_encode(config.lang.msg_mtot_mkv), (state.totals.t_xfer/1024.)/state.totals.t_visits, state.totals.max_v_xfer/1024.);

   /* Max/Avg Visits per Day */
   fprintf(out_fp,"<data name=\"t_vpday\" help=\"visits_per_day\" title=\"%s\"><avg>%" PRIu64 "</avg><max>%" PRIu64 "</max></data>\n", xml_encode(config.lang.msg_mtot_mvd), state.totals.t_visits/days_in_month, max_visits);

   fputs("</section>\n", out_fp);

	fputs("<section>\n", out_fp);
	fprintf(out_fp,"<columns><col help=\"status_codes_section\" title=\"%s\"/><col title=\"%s\"/></columns>\n", xml_encode(config.lang.msg_mtot_rc), config.lang.msg_h_total);

   /**********************************************/
   /* response code totals */
   for (i=0; i < state.response.size(); i++) {
      if (state.response[i].count != 0)
         fprintf(out_fp,"<data title=\"%s\" code=\"%d\"><sum>%" PRIu64 "</sum></data>\n", xml_encode(config.lang.get_resp_code(state.response[i].get_scode()).desc), state.response[i].get_scode(), state.response[i].count);
   }

   fputs("</section>\n", out_fp);

   fprintf(out_fp,"<notes>\n<note>%s</note>\n</notes>\n", xml_encode(config.lang.msg_misc_visitors));

	fputs("</report>\n\n", out_fp);
}

void xml_output_t::write_daily_totals(void)
{
   string_t png_fname;
   string_t title;
   u_int wday;
   int i;
   const hist_month_t *hptr;

   if((hptr = state.history.find_month(state.totals.cur_tstamp.year, state.totals.cur_tstamp.month)) == NULL)
      return;

   //
   //
   //
   png_fname.format("daily_usage_%04d%02d.png",state.totals.cur_tstamp.year,state.totals.cur_tstamp.month);

   if(config.html_ext_lang)
      png_fname = png_fname + '.' + config.lang.language_code;

   title.format("%s %s %d",config.lang.msg_hmth_du, lang_t::l_month[state.totals.cur_tstamp.month-1], state.totals.cur_tstamp.year);
   
   if(makeimgs)
      graph.month_graph6(png_fname, title, state.totals.cur_tstamp.month, state.totals.cur_tstamp.year, state.t_daily);

   //
   //
   //   
   wday = tstamp_t::wday(state.totals.cur_tstamp.year, state.totals.cur_tstamp.month, 1);

	fprintf(out_fp, "<report id=\"daily_totals\" help=\"daily_totals_report\" title=\"%s %s %d\">\n", config.lang.msg_dtot_ds, lang_t::l_month[state.totals.cur_tstamp.month-1], state.totals.cur_tstamp.year);
	
   fprintf(out_fp, "<graph title=\"%s %s %d\">\n", config.lang.msg_hmth_du, lang_t::l_month[state.totals.cur_tstamp.month-1], state.totals.cur_tstamp.year);
   
   fprintf(out_fp, "<image filename=\"%s\" width=\"%d\" height=\"%d\"/>\n", png_fname.c_str(), 512, 400);

   fputs("<days>", out_fp);
	for(i = 0; i < 31; i++)
	   fprintf(out_fp, "<day weekday=\"%d\"/>", (wday + i) % 7);
   fputs("</days>\n", out_fp);
   
   fputs("</graph>\n", out_fp);
	
   fputs("<columns>\n", out_fp);
   fprintf(out_fp, "<col title=\"%s\" class=\"counter\"/>\n", config.lang.msg_h_day);
   fprintf(out_fp, "<col help=\"hits\" title=\"%s\" class=\"hits\"><columns><col title=\"%s\" percent=\"yes\"/><col title=\"%s\"/><col title=\"%s\"/></columns></col>\n", config.lang.msg_h_hits, config.lang.msg_h_total, config.lang.msg_h_avg, config.lang.msg_h_max);
   fprintf(out_fp, "<col help=\"files\" title=\"%s\" class=\"files\"><columns><col title=\"%s\" percent=\"yes\"/><col title=\"%s\"/><col title=\"%s\"/></columns></col>\n", config.lang.msg_h_files, config.lang.msg_h_total, config.lang.msg_h_avg, config.lang.msg_h_max);
   fprintf(out_fp, "<col help=\"pages\" title=\"%s\" class=\"pages\"><columns><col title=\"%s\" percent=\"yes\"/><col title=\"%s\"/><col title=\"%s\"/></columns></col>\n", config.lang.msg_h_pages, config.lang.msg_h_total, config.lang.msg_h_avg, config.lang.msg_h_max);
   fprintf(out_fp, "<col help=\"visits\" title=\"%s\" class=\"visits\"><columns><col title=\"%s\" percent=\"yes\"/><col title=\"%s\"/><col title=\"%s\"/></columns></col>\n", config.lang.msg_h_visits, config.lang.msg_h_total, config.lang.msg_h_avg, config.lang.msg_h_max);
   fprintf(out_fp, "<col help=\"hosts\" title=\"%s\" class=\"hosts\"><columns><col title=\"%s\" percent=\"yes\"/><col title=\"%s\"/><col title=\"%s\"/></columns></col>\n", config.lang.msg_h_hosts, config.lang.msg_h_total, config.lang.msg_h_avg, config.lang.msg_h_max);
   fprintf(out_fp, "<col help=\"transfer\" title=\"%s\" class=\"xfer\"><columns><col title=\"%s\" percent=\"yes\"/><col title=\"%s\"/><col title=\"%s\"/></columns></col>\n", config.lang.msg_h_xfer, config.lang.msg_h_total, config.lang.msg_h_avg, config.lang.msg_h_max);
   fputs("</columns>\n", out_fp);

   /* skip beginning blank days in a month */
   for (i=0; i < hptr->lday; i++) {
		if (state.t_daily[i].tm_hits != 0)
			break;
	}

   if(i == hptr->lday)
		i=0;

	fputs("<rows>\n", out_fp);
	
   for (; i < hptr->lday; i++) {
      fprintf(out_fp,"<row count=\"%hu\" rowid=\"%d\">\n", state.t_daily[i].td_hours, i+1);
      fprintf(out_fp,"<data><value weekday=\"%d\">%d</value></data>\n", (wday + i) % 7, i+1);
      fprintf(out_fp,"<data><sum percent=\"%3.02f\">%" PRIu64 "</sum><avg>%.2f</avg><max>%" PRIu64 "</max></data>\n", PCENT(state.t_daily[i].tm_hits, state.totals.t_hit), state.t_daily[i].tm_hits, state.t_daily[i].h_hits_avg, state.t_daily[i].h_hits_max);
      fprintf(out_fp,"<data><sum percent=\"%3.02f\">%" PRIu64 "</sum><avg>%.2f</avg><max>%" PRIu64 "</max></data>\n", PCENT(state.t_daily[i].tm_files, state.totals.t_file), state.t_daily[i].tm_files, state.t_daily[i].h_files_avg, state.t_daily[i].h_files_max);
      fprintf(out_fp,"<data><sum percent=\"%3.02f\">%" PRIu64 "</sum><avg>%.2f</avg><max>%" PRIu64 "</max></data>\n", PCENT(state.t_daily[i].tm_pages, state.totals.t_page), state.t_daily[i].tm_pages, state.t_daily[i].h_pages_avg, state.t_daily[i].h_pages_max);
      fprintf(out_fp,"<data><sum percent=\"%3.02f\">%" PRIu64 "</sum><avg>%.2f</avg><max>%" PRIu64 "</max></data>\n", PCENT(state.t_daily[i].tm_visits, state.totals.t_visits), state.t_daily[i].tm_visits, state.t_daily[i].h_visits_avg, state.t_daily[i].h_visits_max);
      fprintf(out_fp,"<data><sum percent=\"%3.02f\">%" PRIu64 "</sum><avg>%.2f</avg><max>%" PRIu64 "</max></data>\n", PCENT(state.t_daily[i].tm_hosts, state.totals.t_hosts), state.t_daily[i].tm_hosts, state.t_daily[i].h_hosts_avg, state.t_daily[i].h_hosts_max);
      fprintf(out_fp,"<data><sum percent=\"%3.02f\">%.0f</sum><avg>%.0f</avg><max>%.0f</max></data>\n", PCENT(state.t_daily[i].tm_xfer, state.totals.t_xfer), state.t_daily[i].tm_xfer/1024., state.t_daily[i].h_xfer_avg/1024., state.t_daily[i].h_xfer_max/1024.);
      fputs("</row>\n", out_fp);
   }
   
	fputs("</rows>\n", out_fp);

   fprintf(out_fp,"<notes>\n<note>%s</note>\n</notes>\n", xml_encode(config.lang.msg_misc_pages));
	
   fputs("</report>\n\n", out_fp);
}

void xml_output_t::write_hourly_totals(void)
{
   string_t png_fname;
   string_t title;
   int i,days_in_month;

   //
   //
   //
   png_fname.format("hourly_usage_%04d%02d.png",state.totals.cur_tstamp.year,state.totals.cur_tstamp.month);

   if(config.html_ext_lang)
      png_fname = png_fname + '.' + config.lang.language_code;

   title.format("%s %s %d", config.lang.msg_hmth_hu, lang_t::l_month[state.totals.cur_tstamp.month-1],state.totals.cur_tstamp.year);
   
   if(makeimgs)
      graph.day_graph3(png_fname, title, state.t_hourly);

   //
   //
   //
   days_in_month=(state.totals.l_day-state.totals.f_day)+1;

	fprintf(out_fp, "<report id=\"hourly_totals\" help=\"hourly_totals_report\" title=\"%s %s %d\">\n", config.lang.msg_htot_hs, lang_t::l_month[state.totals.cur_tstamp.month-1], state.totals.cur_tstamp.year);

   fprintf(out_fp, "<graph title=\"%s %s %d\">\n", config.lang.msg_hmth_hu, lang_t::l_month[state.totals.cur_tstamp.month-1],state.totals.cur_tstamp.year);

   fprintf(out_fp, "<image filename=\"%s\" width=\"%d\" height=\"%d\"/>\n", png_fname.c_str(), 512, 340);

   fputs("<hours>", out_fp);
	for(i = 0; i < 24; i++)
	   fputs("<hour/>", out_fp);
   fputs("</hours>\n", out_fp);
   
   fputs("</graph>\n", out_fp);

   fputs("<columns>\n", out_fp);
   fprintf(out_fp, "<col title=\"%s\" class=\"counter\"/>\n", config.lang.msg_h_day);
   fprintf(out_fp, "<col help=\"hits\" title=\"%s\" class=\"hits\"><columns><col title=\"%s\"/><col title=\"%s\" percent=\"yes\"/></columns></col>\n", config.lang.msg_h_hits, config.lang.msg_h_avg, config.lang.msg_h_total);
   fprintf(out_fp, "<col help=\"files\" title=\"%s\" class=\"files\"><columns><col title=\"%s\"/><col title=\"%s\" percent=\"yes\"/></columns></col>\n", config.lang.msg_h_files, config.lang.msg_h_avg, config.lang.msg_h_total);
   fprintf(out_fp, "<col help=\"pages\" title=\"%s\" class=\"pages\"><columns><col title=\"%s\"/><col title=\"%s\" percent=\"yes\"/></columns></col>\n", config.lang.msg_h_pages, config.lang.msg_h_avg, config.lang.msg_h_total);
   fprintf(out_fp, "<col help=\"transfer\" title=\"%s\" class=\"xfer\"><columns><col title=\"%s\"/><col title=\"%s\" percent=\"yes\"/></columns></col>\n", config.lang.msg_h_xfer, config.lang.msg_h_avg, config.lang.msg_h_total);
   fputs("</columns>\n", out_fp);

	fputs("<rows>\n", out_fp);

   for(i=0; i < 24; i++) {
      fprintf(out_fp,"<row count=\"%d\" rowid=\"%d\">\n", days_in_month, i+1);
      fprintf(out_fp,"<data><value>%d</value></data>\n", i);
      fprintf(out_fp, "<data><avg>%" PRIu64 "</avg><sum percent=\"%3.02f\">%" PRIu64 "</sum></data>\n", state.t_hourly[i].th_hits/days_in_month, PCENT(state.t_hourly[i].th_hits, state.totals.t_hit), state.t_hourly[i].th_hits);
      fprintf(out_fp, "<data><avg>%" PRIu64 "</avg><sum percent=\"%3.02f\">%" PRIu64 "</sum></data>\n", state.t_hourly[i].th_files/days_in_month, PCENT(state.t_hourly[i].th_files, state.totals.t_file), state.t_hourly[i].th_files);
      fprintf(out_fp, "<data><avg>%" PRIu64 "</avg><sum percent=\"%3.02f\">%" PRIu64 "</sum></data>\n", state.t_hourly[i].th_pages/days_in_month, PCENT(state.t_hourly[i].th_pages, state.totals.t_page), state.t_hourly[i].th_pages);
      fprintf(out_fp, "<data><avg>%.0f</avg><sum percent=\"%3.02f\">%.0f</sum></data>\n", (state.t_hourly[i].th_xfer/days_in_month)/1024., PCENT(state.t_hourly[i].th_xfer, state.totals.t_xfer), state.t_hourly[i].th_xfer/1024.);
      fputs("</row>\n", out_fp);
   }

	fputs("</rows>\n", out_fp);
	
   fputs("</report>\n\n", out_fp);
}

void xml_output_t::write_top_urls(bool s_xfer)
{
   uint64_t a_ctr;
   u_int i; 
   uint32_t top_num, ntop_num;
   unode_t *uptr;
   const char *href, *dispurl;
   unode_t *u_array;
   string_t str;

   uint64_t max_urls = s_xfer ? config.max_urls_kb : config.max_urls;
   
   multi_reverse_iterator<database_t::reverse_iterator, unode_t> miter;

   // return if nothing to process
   if (state.totals.t_url == 0) return;

   // get the total number, including groups and hidden items
   if((a_ctr = state.totals.t_url + state.totals.t_grp_urls) == 0)
      return;

   /* get max to do... */
   ntop_num = (s_xfer) ? config.ntop_urlsK : config.ntop_urls;
   top_num = (a_ctr > ntop_num) ? ntop_num : (uint32_t) a_ctr;

   // allocate as if there are no hidden items (see write_top_referrers)
   u_array = new unode_t[top_num+1];

   i = 0;

   database_t::reverse_iterator<unode_t> grpiter = state.database.rbegin_urls(s_xfer ? "urls.groups.xfer" : "urls.groups.hits");

   // if groups are bundled, put them first
   if(config.bundle_groups) {

      while(i < top_num && grpiter.prev(u_array[i]))
         i++;

      // if there's more, add the iterator to the combined iterator
      if(i == top_num && config.all_urls)
         miter.push(&grpiter);
   }

   database_t::reverse_iterator<unode_t> iter = state.database.rbegin_urls(s_xfer ? "urls.xfer" : "urls.hits");
   
   // populate the remainder of the array
   if(i < top_num) {

      while(i < top_num && iter.prev(u_array[i])) {
         if(u_array[i].flag == OBJ_REG) {
            // ignore URLs matching any of the hiding patterns
            if(config.hidden_urls.isinlistex(u_array[i].string, u_array[i].pathlen, true))
               continue;
         }
         else if(u_array[i].flag == OBJ_GRP) {
            // ignore groups if we did them before
            if(config.bundle_groups)
               continue;
         }
            
         i++;
      }
   }

   // check if all items are hidden
   if(i == 0) {
      delete [] u_array;
      return;
   }

   // if we need to output all URLs, add the iterator to the combined iterator
   if(i == top_num && config.all_urls)
      miter.push(&iter);

   // adjust array size if it's not filled up
   if(i < top_num)
      top_num = (uint32_t) i;

   if(s_xfer) 
		fprintf(out_fp, "<report id=\"top_url_xfer\" help=\"url_report\" title=\"%s %u %s %" PRIu64 " %s %s %s\" top=\"%d\" total=\"%" PRIu64 "\" max=\"%" PRIu64 "\" groups=\"%" PRIu64 "\">\n", config.lang.msg_top_top, top_num, config.lang.msg_top_of, state.totals.t_url, config.lang.msg_top_u, config.lang.msg_h_by, config.lang.msg_h_xfer, top_num, state.totals.t_url, max_urls, state.totals.t_grp_urls);
   else 
		fprintf(out_fp, "<report id=\"top_url_hits\" help=\"url_report\" title=\"%s %u %s %" PRIu64 " %s\" top=\"%d\" total=\"%" PRIu64 "\" max=\"%" PRIu64 "\" groups=\"%" PRIu64 "\">\n", config.lang.msg_top_top, top_num, config.lang.msg_top_of, state.totals.t_url, config.lang.msg_top_u, top_num, state.totals.t_url, max_urls, state.totals.t_grp_urls);

   // output the top element for the top URL section
   fputs("<top>", out_fp);
   for(i = 0; i < top_num; i++)
      fputs("<item/>", out_fp);
   fputs("</top>\n", out_fp);

   fputs("<columns>\n", out_fp);

   fputs("<col title=\"#\" class=\"counter\"/>\n", out_fp);
   fprintf(out_fp,"<col help=\"hits\" title=\"%s\" class=\"hits\" percent=\"yes\"/>\n", config.lang.msg_h_hits);
   fprintf(out_fp,"<col help=\"transfer\" title=\"%s\" class=\"xfer\" percent=\"yes\"/>\n", config.lang.msg_h_xfer);
   fprintf(out_fp,"<col help=\"req_proc_time\" title=\"%s\" class=\"time\"><columns><col title=\"%s\"/><col title=\"%s\"/></columns></col>\n", config.lang.msg_h_time, config.lang.msg_h_avg, config.lang.msg_h_max);
   fprintf(out_fp,"<col help=\"url\" title=\"%s\" class=\"item\"/>\n", config.lang.msg_h_url);

   fputs("</columns>\n", out_fp);

	fputs("<rows>\n", out_fp);

   uptr = &u_array[0];
   i = 0;
   while(i < top_num || (config.all_urls && i < max_urls && miter.prev(*uptr))) {

      if(i >= top_num) {
         if(uptr->flag == OBJ_REG) {
            // ignore URLs matching any of the hiding patterns
            if(config.hidden_urls.isinlistex(uptr->string, uptr->pathlen, true))
               continue;
         }
         else if(uptr->flag == OBJ_GRP) {
            // see write_top_referrers
            if(config.bundle_groups && i >= state.totals.t_grp_urls)
               continue;
         }
      }
      
		fputs("<row>\n", out_fp);

      fprintf(out_fp,
         "<data><value>%u</value></data>\n" \
         "<data><sum percent=\"%3.02f\">%" PRIu64 "</sum></data>\n" \
         "<data><sum percent=\"%3.02f\">%.0f</sum></data>\n"\
         "<data><avg>%0.3f</avg><max>%0.3f</max></data>\n",
         i+1, 
         (state.totals.t_hit==0)?0:((double)uptr->count/state.totals.t_hit)*100.0, uptr->count, 
         (state.totals.t_xfer==0)?0:((double)uptr->xfer/state.totals.t_xfer)*100.0, uptr->xfer/1024.,
			uptr->avgtime, uptr->maxtime
			);

      if (uptr->flag==OBJ_GRP) {
         // groups may contain XHTML, so output them as is
			fprintf(out_fp,"<data group=\"yes\"><value>%s", uptr->string.c_str());
      }
      else {
			xml_encoder_t xml_encode(xml_encoder, xml_encoder_t::append);

         dispurl = (uptr->hexenc) ? url_decode(uptr->string, str).c_str() : uptr->string.c_str();
         dispurl = xml_encode(dispurl, false);
         href = xml_encode(uptr->string, false);
         
         // check for log type and if the URL contains a URI scheme
         if(config.log_type == LOG_SQUID || strstr_ex(uptr->string, "://", 10, 3)) {
            fprintf(out_fp,"<data complete=\"yes\"%s url=\"%s\"><value>%s", uptr->target?" target=\"yes\"" : "", href, dispurl);
         }
			else {
            if(is_secure_url(uptr->urltype, config.use_https))
               fprintf(out_fp, "<data secure=\"yes\"%s url=\"%s\"><value>%s", uptr->target?" target=\"yes\"" : "", href, dispurl);
            else
               fprintf(out_fp, "<data%s url=\"%s\"><value>%s", uptr->target?" target=\"yes\"" : "", href, dispurl);
         }
		}
		fputs("</value></data>\n", out_fp);
		fputs("</row>\n", out_fp);
		
  		if(i < top_num)
         uptr++;
         
       i++;
   }
   
	fputs("</rows>\n", out_fp);
   fputs("</report>\n\n", out_fp);

   iter.close();
   grpiter.close();
   delete [] u_array;
}

void xml_output_t::write_top_entry(bool entry)
{
   uint64_t a_ctr;
   uint32_t tot_num;
   u_int i;
   unode_t unode;
   const char *href, *dispurl;
   unode_t *u_array;
   const unode_t *uptr;
   string_t str;

   // return if nothing to process
   if (state.totals.t_url == 0) return;

   // get the total number, including groups and hidden items
   if((a_ctr = (entry ? state.totals.u_entry : state.totals.u_exit)) == 0)
      return;

   /* get max to do... */
   if(entry)
      tot_num = (a_ctr > config.ntop_entry) ? config.ntop_entry : (uint32_t) a_ctr;
   else
      tot_num = (a_ctr > config.ntop_exit) ? config.ntop_exit : (uint32_t) a_ctr;

   // allocate as if there are no hidden items
   u_array = new unode_t[tot_num];

   i = 0;

   // traverse the entry/exit tables and populate the array
   database_t::reverse_iterator<unode_t> iter = state.database.rbegin_urls(entry ? "urls.entry" : "urls.exit");

   while(i < tot_num && iter.prev(u_array[i])) {
      if(u_array[i].flag == OBJ_REG && !config.hidden_urls.isinlistex(u_array[i].string, u_array[i].pathlen, true)) {
         // do not show entries with zero entry/exit values
         if(entry && u_array[i].entry || !entry && u_array[i].exit)
            i++;
      }
   }

   iter.close();

   // check if all items are hidden or have zero entry/exit counts
   if(i == 0) {
      delete [] u_array;
      return;
   }

   // adjust array size if it's not filled up
   if(i < tot_num)
      tot_num = i;

   fprintf(out_fp, "<report id=\"%s\" help=\"%s\" title=\"%s %u %s %" PRIu64 " %s\" top=\"%u\" total=\"%" PRIu64 "\">\n", 
                     entry ? "top_entry_urls" : "top_exit_urls",
                     entry ? "entry_url_report" : "exit_url_report",
                     config.lang.msg_top_top, tot_num, config.lang.msg_top_of,
                     entry ? state.totals.u_entry : state.totals.u_exit, 
                     entry ? config.lang.msg_top_en : config.lang.msg_top_ex,
                     tot_num, entry ? state.totals.u_entry : state.totals.u_exit);

   fputs("<columns>\n", out_fp);

   fputs("<col title=\"#\" class=\"counter\"/>\n", out_fp);
   fprintf(out_fp,"<col help=\"hits\" title=\"%s\" class=\"hits\" percent=\"yes\"/>\n", config.lang.msg_h_hits);
   fprintf(out_fp,"<col help=\"visits\" title=\"%s\" class=\"visits\" percent=\"yes\"/>\n", config.lang.msg_h_visits);
   fprintf(out_fp,"<col help=\"url\" title=\"%s\" class=\"item\"/>\n", config.lang.msg_h_url);

   fputs("</columns>\n", out_fp);

	fputs("<rows>\n", out_fp);

   uptr = &u_array[0];
   for(i = 0; i < tot_num; i++) {
	   xml_encoder_t xml_encode(xml_encoder, xml_encoder_t::append);

      fputs("<row>\n", out_fp);
      fprintf(out_fp,
          "<data><value>%d</value></data>\n"
          "<data><sum percent=\"%3.02f\">%" PRIu64 "</sum></data>\n"
          "<data><sum percent=\"%3.02f\">%" PRIu64 "</sum></data>\n",
          i+1,
          (state.totals.t_hit==0)?0:((double)uptr->count/state.totals.t_hit)*100.0, uptr->count,
          (entry)?((state.totals.t_entry==0)?0:((double)uptr->entry/state.totals.t_entry)*100.0) : 
            ((state.totals.t_exit==0)?0:((double)uptr->exit/state.totals.t_exit)*100.0),
          (entry)?uptr->entry : uptr->exit);

      dispurl = (uptr->hexenc) ? url_decode(uptr->string, str).c_str() : uptr->string.c_str();
      dispurl = xml_encode(dispurl, false);
      href = xml_encode(uptr->string, false);

      /* check for a service prefix (ie: http://) */
      if(strstr_ex(uptr->string, "://", 10, 3))
         fprintf(out_fp,"<data complete=\"yes\"%s url=\"%s\"><value>%s", uptr->target?" target=\"yes\"" : "", href, dispurl);
		else
      {
         if(is_secure_url(uptr->urltype, config.use_https))
            fprintf(out_fp, "<data secure=\"yes\"%s url=\"%s\"><value>%s", uptr->target?" target=\"yes\"" : "", href, dispurl);
         else
            fprintf(out_fp, "<data%s url=\"%s\"><value>%s", uptr->target?" target=\"yes\"" : "", href, dispurl);
	   }
      
      fputs("</value></data>\n", out_fp);
	   fputs("</row>\n", out_fp);
	   
      uptr++;
   }

   delete [] u_array;

	fputs("</rows>\n", out_fp);
	
	// output a note that robot activity is not included in this report
   if(state.totals.t_rhits)
      fprintf(out_fp,"<notes>\n<note>%s</note>\n</notes>\n", xml_encode(config.lang.msg_misc_robots));
	
   fputs("</report>\n\n", out_fp);
}

void xml_output_t::write_top_downloads(void)
{
   uint64_t a_ctr;
   uint32_t top_num;
   u_int i;
   dlnode_t *nptr;
   const char *cdesc;
   dlnode_t *dl_array;

   if((a_ctr = state.totals.t_downloads) == 0)
      return;

   /* get max to do... */
   top_num = (a_ctr > config.ntop_downloads) ? config.ntop_downloads : (uint32_t) a_ctr;

   // get top_num xfer-ordered nodes from the database (see write_top_referrers)
   dl_array = new dlnode_t[top_num+1];

   database_t::reverse_iterator<dlnode_t> iter = state.database.rbegin_downloads("downloads.xfer");

   for(i = 0; i < top_num; i++) {
      // state_t::unpack_dlnode_const_cb does not modify state
      if(!iter.prev(dl_array[i], state_t::unpack_dlnode_const_cb, const_cast<state_t*>(&state)))
         break;
   }

   if(i < top_num)
      fprintf(stderr, "Failed to retrieve download records (%d)", iter.get_error());

   // generate the report
   fprintf(out_fp, "<report id=\"top_downloads\" help=\"downloads_report\" colspan=\"%d\" title=\"%s %u %s %" PRIu64 " %s\" top=\"%d\" total=\"%" PRIu64 "\" max=\"%" PRIu64 "\">\n", config.ntop_ctrys?config.geoip_city?12:11:10, config.lang.msg_top_top, top_num, config.lang.msg_top_of, state.totals.t_downloads, config.lang.msg_h_downloads, top_num, state.totals.t_downloads, config.max_downloads);

   // output the top element for the top downloads section
   fputs("<top>", out_fp);
   for(i = 0; i < top_num; i++)
      fputs("<item/>", out_fp);
   fputs("</top>\n", out_fp);

   fputs("<columns>\n", out_fp);

   fputs("<col title=\"#\" class=\"counter\"/>\n", out_fp);
   fprintf(out_fp,"<col help=\"hits\" title=\"%s\" class=\"hits\" percent=\"yes\"/>\n", config.lang.msg_h_hits);
   fprintf(out_fp,"<col help=\"transfer\" title=\"%s\" class=\"xfer\" percent=\"yes\"/>\n", config.lang.msg_h_xfer);
   fprintf(out_fp,"<col help=\"download_time\" title=\"%s\" class=\"time\"><columns><col title=\"%s\"/><col title=\"%s\"/></columns></col>\n", config.lang.msg_h_time, config.lang.msg_h_avg, config.lang.msg_h_total);
   fprintf(out_fp,"<col help=\"download_count\" title=\"%s\" class=\"count\"/>\n", config.lang.msg_h_count);
   fprintf(out_fp,"<col help=\"download\" title=\"%s\" class=\"dlname\"/>\n", config.lang.msg_h_download);
   
   if(config.ntop_ctrys) {
      fprintf(out_fp,"<col help=\"country\" title=\"%s\" class=\"country\"/>\n", config.lang.msg_h_ctry);
      if(config.geoip_city)
         fprintf(out_fp,"<col help=\"city\" title=\"%s\" class=\"country\"/>\n", config.lang.msg_h_city);
   }
   
   fprintf(out_fp,"<col help=\"host\" title=\"%s\" class=\"item\"/>\n", config.lang.msg_h_hname);

   fputs("</columns>\n", out_fp);

	fputs("<rows>\n", out_fp);

   nptr = &dl_array[0];
   i = 0; 
   while(i < top_num || (config.all_downloads && i < config.max_downloads && iter.prev(*nptr, state_t::unpack_dlnode_const_cb, const_cast<state_t*>(&state)))) {
      if(!nptr->hnode)
         throw exception_t(0, string_t::_format("Missing host in a download node (ID: %" PRIu64 ")", nptr->nodeid));

      if(config.ntop_ctrys) {
         if(!nptr->hnode)
            cdesc = "";
         else
            cdesc = state.cc_htab.get_ccnode(nptr->hnode->get_ccode()).cdesc;
      }

      fputs("<row>\n", out_fp);
      
      fprintf(out_fp,
          "<data><value>%d</value></data>\n"
          "<data><sum percent=\"%3.02f\">%" PRIu64 "</sum></data>\n"
          "<data><sum percent=\"%3.02f\">%.0f</sum></data>\n"
          "<data><avg>%3.02f</avg><sum>%3.02f</sum></data>\n"
          "<data><value>%" PRIu64 "</value></data>\n"
          "<data><value>%s</value></data>\n",
          i+1,
          (state.totals.t_hit == 0) ? 0 : ((double)nptr->sumhits/state.totals.t_hit)*100.0, nptr->sumhits, 
          (state.totals.t_xfer == 0) ? 0 : ((nptr->sumxfer/1024.)/(state.totals.t_xfer / 1024.))*100.0, nptr->sumxfer/1024.,
          nptr->avgtime, nptr->sumtime, 
          nptr->count,
          nptr->string.c_str());
          
          if(config.ntop_ctrys) {
            fprintf(out_fp, "<data ccode=\"%s\"><value>%s</value></data>\n", nptr->hnode->get_ccode().c_str(), xml_encode(cdesc));
            if(config.geoip_city)
               fprintf(out_fp, "<data ccode=\"%s\"><value>%s</value></data>\n", nptr->hnode->get_ccode().c_str(), xml_encode(nptr->hnode->city.c_str()));
          }
          
          fprintf(out_fp, "<data ipaddr=\"%s\"><value>%s</value></data>\n", nptr->hnode->string.c_str(), xml_encode(nptr->hnode->hostname().c_str()));

      fputs("</row>\n", out_fp);
      
  		if(i < top_num)
         nptr++;
      i++;
   }
   fputs("</rows>\n", out_fp);
   fputs("</report>\n\n", out_fp);

   iter.close();
   delete [] dl_array;
}

void xml_output_t::write_top_errors(void)
{
   uint64_t a_ctr, top_num;
   u_int i;
   rcnode_t rcnode;
   const char *dispurl;
   string_t str;

   if(state.totals.t_err == 0) return;

   if((a_ctr = state.totals.t_err) == 0)
      return;

   /* get max to do... */
   top_num = (a_ctr > config.ntop_errors) ? config.ntop_errors : a_ctr;

   fprintf(out_fp, "<report id=\"top_errors\" help=\"errors_report\" title=\"%s %" PRIu64 " %s %" PRIu64 " %s\" top=\"%" PRIu64 "\" total=\"%" PRIu64 "\" max=\"%" PRIu64 "\">\n", config.lang.msg_top_top, top_num, config.lang.msg_top_of, state.totals.t_err, config.lang.msg_h_errors, top_num, state.totals.t_err, config.max_errors);

   // output the top element for the top errors section
   fputs("<top>", out_fp);
   for(i = 0; i < top_num; i++)
      fputs("<item/>", out_fp);
   fputs("</top>\n", out_fp);

	fputs("<columns>\n", out_fp);
	
   fputs("<col title=\"#\" class=\"counter\"/>\n", out_fp);
   fprintf(out_fp,"<col help=\"hits\" title=\"%s\" class=\"hits\" percent=\"yes\"/>\n", config.lang.msg_h_hits);
   fprintf(out_fp,"<col help=\"http_status_code\" title=\"%s\" class=\"errors\"/>\n", config.lang.msg_h_status);
   fprintf(out_fp,"<col help=\"http_method\" title=\"%s\" class=\"method\"/>\n", config.lang.msg_h_method);
   fprintf(out_fp,"<col help=\"url\" title=\"%s\" class=\"item\"/>\n", config.lang.msg_h_url);
   
	fputs("</columns>\n", out_fp);

   // get top top_num hit-ordered nodes from the state.database
   database_t::reverse_iterator<rcnode_t> iter = state.database.rbegin_errors("errors.hits");

	fputs("<rows>\n", out_fp);

   for(i=0; (i < top_num || config.all_errors && i < config.max_errors) && iter.prev(rcnode); i++) {
	   xml_encoder_t xml_encode(xml_encoder, xml_encoder_t::append);

      fputs("<row>\n", out_fp);

      fprintf(out_fp,
          "<data><value>%d</value></data>\n"
          "<data><sum percent=\"%3.02f\">%" PRIu64 "</sum></data>\n"
          "<data title=\"%s\"><value>%d</value></data>\n"
          "<data><value>%s</value></data>\n",
          i+1,
          (state.totals.t_hit == 0) ? 0: ((double)rcnode.count/state.totals.t_hit)*100.0,
          rcnode.count,
          xml_encode(config.lang.get_resp_code(rcnode.respcode).desc), 
          rcnode.respcode, 
          xml_encode(rcnode.method));

      dispurl = (rcnode.hexenc) ? url_decode(rcnode.string, str).c_str() : rcnode.string.c_str();
      dispurl = xml_encode(dispurl, false);
      fprintf(out_fp,"<data><value>%s</value></data>\n", dispurl);

      fputs("</row>\n", out_fp);
   }
	fputs("</rows>\n", out_fp);
   fputs("</report>\n\n", out_fp);

   iter.close();
}

void xml_output_t::write_top_hosts(bool s_xfer)
{
   uint64_t a_ctr;
   u_int i;
   uint32_t top_num, ntop_num;
   hnode_t *hptr;
   const char *cdesc;
   hnode_t *h_array;
   uint64_t max_hosts = s_xfer ? config.max_hosts_kb : config.max_hosts;
   
   multi_reverse_iterator<database_t::reverse_iterator, hnode_t> miter;

   // return if nothing to process
   if (state.totals.t_hosts == 0) return;

   // get the total number, including groups and hidden items
   if((a_ctr = state.totals.t_hosts + state.totals.t_grp_hosts) == 0)
      return;

   /* get max to do... */
   ntop_num = (s_xfer) ? config.ntop_sitesK : config.ntop_sites;
   top_num = (a_ctr > ntop_num) ? ntop_num : (uint32_t) a_ctr;

   // allocate as if there are no hidden items (see write_top_referrers)
   h_array = new hnode_t[top_num+1];

   i = 0;

   database_t::reverse_iterator<hnode_t> grpiter = state.database.rbegin_hosts(s_xfer ? "hosts.groups.xfer" : "hosts.groups.hits");

   // if groups are bundled, output them first
   if(config.bundle_groups) {

      while(i < top_num && grpiter.prev(h_array[i]))
         i++;

      // if there's more, add the iterator to the combined iterator
      if(i == top_num && config.all_hosts)
         miter.push(&grpiter);
   }

   database_t::reverse_iterator<hnode_t> iter = state.database.rbegin_hosts(s_xfer ? "hosts.xfer" : "hosts.hits");

   // populate the remainder of the array
   if(i < top_num) {
      while(i < top_num && iter.prev(h_array[i])) {
         if(h_array[i].flag == OBJ_REG) {
            // ignore hosts matching any of the hiding patterns
            if(config.hide_hosts || h_array[i].robot && config.hide_robots || config.hidden_hosts.isinlist(h_array[i].string) || config.hidden_hosts.isinlist(h_array[i].name))
               continue;
         }
         else if(h_array[i].flag == OBJ_GRP) {
            // ignore groups if we did them before
            if(config.bundle_groups)
               continue;
         }
            
         i++;   
      }
   }

   // check if all items are hidden
   if(i == 0) {
      delete [] h_array;
      return;
   }

   // if we need to output all hosts, add the iterator to the combined iterator
   if(i == top_num && config.all_hosts)
      miter.push(&iter);

   // adjust array size if it's not filled up
   if(i < top_num)
      top_num = (uint32_t) i;

   if(s_xfer)
      fprintf(out_fp, "<report id=\"top_host_xfer\" help=\"hosts_report\" colspan=\"%d\" title=\"%s %u %s %" PRIu64 " %s %s %s\" top=\"%u\" total=\"%" PRIu64 "\" max=\"%" PRIu64 "\" groups=\"%" PRIu64 "\">\n", config.ntop_ctrys?config.geoip_city?16:15:14, config.lang.msg_top_top, top_num, config.lang.msg_top_of, state.totals.t_hosts, config.lang.msg_top_s, config.lang.msg_h_by, config.lang.msg_h_xfer, top_num, state.totals.t_hosts, max_hosts, state.totals.t_grp_hosts);
   else
      fprintf(out_fp, "<report id=\"top_host_hits\" help=\"hosts_report\" colspan=\"%d\" title=\"%s %u %s %" PRIu64 " %s\" top=\"%u\" total=\"%" PRIu64 "\" max=\"%" PRIu64 "\" groups=\"%" PRIu64 "\">\n", config.ntop_ctrys?config.geoip_city?16:15:14, config.lang.msg_top_top, top_num, config.lang.msg_top_of, state.totals.t_hosts, config.lang.msg_top_s, top_num, state.totals.t_hosts, max_hosts, state.totals.t_grp_hosts);

   // output the top element for the top hosts section
   fputs("<top>", out_fp);
   for(i = 0; i < top_num; i++)
      fputs("<item/>", out_fp);
   fputs("</top>\n", out_fp);

   fputs("<columns>\n", out_fp);

   fputs("<col title=\"#\" class=\"counter\"/>\n", out_fp);
   fprintf(out_fp,"<col help=\"hits\" title=\"%s\" class=\"hits\" percent=\"yes\"/>\n", config.lang.msg_h_hits);
   fprintf(out_fp,"<col help=\"files\" title=\"%s\" class=\"files\" percent=\"yes\"/>\n", config.lang.msg_h_files);
   fprintf(out_fp,"<col help=\"pages\" title=\"%s\" class=\"pages\" percent=\"yes\"/>\n", config.lang.msg_h_pages);
   fprintf(out_fp,"<col help=\"transfer\" title=\"%s\" class=\"xfer\" percent=\"yes\"/>\n", config.lang.msg_h_xfer);
   fprintf(out_fp,"<col help=\"visits\" title=\"%s\" class=\"visits\" percent=\"yes\"/>\n", config.lang.msg_h_visits);
   fprintf(out_fp,"<col help=\"visit_duration\" title=\"%s\" class=\"duration\"><columns><col title=\"%s\"/><col title=\"%s\"/></columns></col>\n", config.lang.msg_h_duration, config.lang.msg_h_avg, config.lang.msg_h_max);
   
   if(config.ntop_ctrys) {
      fprintf(out_fp,"<col help=\"country\" title=\"%s\" class=\"country\"/>\n", config.lang.msg_h_ctry);
      if(config.geoip_city)
         fprintf(out_fp,"<col help=\"city\" title=\"%s\" class=\"country\"/>\n", config.lang.msg_h_city);
   }
      
   fprintf(out_fp,"<col help=\"host\" title=\"%s\" class=\"item\"/>\n", config.lang.msg_h_hname);

   fputs("</columns>\n", out_fp);

	fputs("<rows>\n", out_fp);

   hptr = &h_array[0];
   i = 0;
   while(i < top_num || (config.all_hosts && i < max_hosts && miter.prev(*hptr))) {

      if(i >= top_num) {
         if(hptr->flag == OBJ_REG) {
            // ignore hosts matching any of the hiding patterns
            if(config.hide_hosts || hptr->robot && config.hide_robots || config.hidden_hosts.isinlist(hptr->string) || config.hidden_hosts.isinlist(hptr->name))
               continue;
         }
         else if(hptr->flag == OBJ_GRP) {
            // see write_top_referrers
            if(config.bundle_groups && i >= state.totals.t_grp_hosts)
               continue;
         }
      }
   
      // if the country report is enabled, look up country name
      if(config.ntop_ctrys) {
         if(hptr->flag==OBJ_GRP)
            cdesc = "";
         else
            cdesc = state.cc_htab.get_ccnode(hptr->get_ccode()).cdesc;
      }

      fputs("<row>\n", out_fp);
      
      fprintf(out_fp,
           "<data><value>%u</value></data>\n"
           "<data><sum percent=\"%3.02f\">%" PRIu64 "</sum></data>\n"
           "<data><sum percent=\"%3.02f\">%" PRIu64 "</sum></data>\n"
           "<data><sum percent=\"%3.02f\">%" PRIu64 "</sum></data>\n"
           "<data><sum percent=\"%3.02f\">%.0f</sum></data>\n"
           "<data><sum percent=\"%3.02f\">%" PRIu64 "</sum></data>\n"
           "<data><avg>%.0f</avg><max>%.0f</max></data>\n",
           i+1,
           (state.totals.t_hit==0)?0:((double)hptr->count/state.totals.t_hit)*100.0, hptr->count, 
           (state.totals.t_file==0)?0:((double)hptr->files/state.totals.t_file)*100.0, hptr->files, 
           (state.totals.t_page==0)?0:((double)hptr->pages/state.totals.t_page)*100.0, hptr->pages, 
           (state.totals.t_xfer==0)?0:((double)hptr->xfer/state.totals.t_xfer)*100.0, hptr->xfer/1024.,
           (state.totals.t_visits==0)?0:((double)hptr->visits/state.totals.t_visits)*100.0, hptr->visits,
           hptr->visit_avg/60., hptr->visit_max/60.);

      if(hptr->flag == OBJ_GRP) {
         // no country information for groups
         fputs("<data><value/></data>\n", out_fp);
         fprintf(out_fp, "<data group=\"yes\"><value>%s</value></data>\n", hptr->string.c_str());
      }
      else {
         // country code and name
         if(config.ntop_ctrys) {
            fprintf(out_fp, "<data ccode=\"%s\"><value>%s</value></data>\n", hptr->get_ccode().c_str(), xml_encode(cdesc));
            if(config.geoip_city)
               fprintf(out_fp, "<data ccode=\"%s\"><value>%s</value></data>\n", hptr->get_ccode().c_str(), xml_encode(hptr->city.c_str()));
         }

         // output the span with the IP address as a title
         fprintf(out_fp, 
               "<data ipaddr=\"%s\"%s><value>",
               hptr->string.c_str(),
               hptr->spammer ? " spammer=\"yes\"" : hptr->robot ? " robot=\"yes\"" : hptr->visits_conv ? " converted=\"yes\"" : "");

         fputs(xml_encode(hptr->hostname().c_str()), out_fp);
         fputs("</value></data>\n", out_fp);
		}
      
  		fputs("</row>\n", out_fp);

  		// once we reached the end, keep the pointer at the extra node
  		if(i < top_num)
         hptr++;
      i++;
   }

	fputs("</rows>\n", out_fp);
   fputs("</report>\n\n", out_fp);

   grpiter.close();
   iter.close();
   delete [] h_array;
}

void xml_output_t::write_top_referrers(void)
{
   uint64_t a_ctr;
   uint32_t top_num;
   u_int i;
   rnode_t *rptr;
   rnode_t *r_array;
   const char *href, *dispurl, *cp1;
   string_t str;
   
   multi_reverse_iterator<database_t::reverse_iterator, rnode_t> miter;

   // return if nothing to process
   if (state.totals.t_ref==0) return;        

   // get the total number, including groups and hidden items
   if((a_ctr = state.totals.t_ref + state.totals.t_grp_refs) == 0)
      return;

   /* get max to do... */
   top_num = (a_ctr > config.ntop_refs) ? config.ntop_refs : (uint32_t) a_ctr;

   //
   // Allocate an array, so we could keep non-hidden items and bundled 
   // groups while iterating through the nodes in the database. One
   // extra node at the end of the array will be used as a current
   // node when iterating through the remaining items (if all items
   // was selected).
   //
   r_array = new rnode_t[top_num+1];

   i = 0;

   // create a reverse referrer group iterator
   database_t::reverse_iterator<rnode_t> grpiter = state.database.rbegin_referrers("referrers.groups.hits");
   
   // if groups are bundled, put them first
   if(config.bundle_groups) {
      while(i < top_num && grpiter.prev(r_array[i]))
         i++;
      
      // if there's more, add the iterator to the combined iterator
      if(i == top_num && config.all_refs)
         miter.push(&grpiter);
   }

   // create a reverse referrer iterator
   database_t::reverse_iterator<rnode_t> iter = state.database.rbegin_referrers("referrers.hits");
   
   // populate the remainder of the array
   if(i < top_num) {
      while(i < top_num && iter.prev(r_array[i])) {
         if(r_array[i].flag == OBJ_REG) {
            // ignore referrers matching any of the hiding patterns
            if(config.hidden_refs.isinlist(r_array[i].string))
               continue;
         }
         else if(r_array[i].flag == OBJ_GRP) {
            // ignore groups if we did them before
            if(config.bundle_groups)
               continue;
         }
            
         i++;   
      }
   }

   // check if all items are hidden
   if(i == 0) {
      delete [] r_array;
      return;
   }

   // if we need to output all referrers, add the iterator to the combined iterator
   if(i == top_num && config.all_refs)
      miter.push(&iter);

   // adjust array size if it's not filled up
   if(i < top_num)
      top_num = i;

   fprintf(out_fp, "<report id=\"top_referrers\" help=\"referrers_report\" title=\"%s %u %s %" PRIu64 " %s\" top=\"%u\" total=\"%" PRIu64 "\" max=\"%" PRIu64 "\" groups=\"%" PRIu64 "\">\n", config.lang.msg_top_top, top_num, config.lang.msg_top_of, state.totals.t_ref, config.lang.msg_top_r, top_num, state.totals.t_ref, config.max_refs, state.totals.t_grp_refs);

   // output the top element for the top referrers section
   fputs("<top>", out_fp);
   for(i = 0; i < top_num; i++)
      fputs("<item/>", out_fp);
   fputs("</top>\n", out_fp);

   fputs("<columns>\n", out_fp);

   fputs("<col title=\"#\" class=\"counter\"/>\n", out_fp);
   fprintf(out_fp,"<col help=\"hits\" title=\"%s\" class=\"hits\" percent=\"yes\"/>\n", config.lang.msg_h_hits);
   fprintf(out_fp,"<col help=\"visits\" title=\"%s\" class=\"visits\" percent=\"yes\"/>\n", config.lang.msg_h_visits);
   fprintf(out_fp,"<col help=\"referrer\" title=\"%s\" class=\"item\"/>\n", config.lang.msg_h_ref);

   fputs("</columns>\n", out_fp);

	fputs("<rows>\n", out_fp);

   rptr = &r_array[0]; 
   i = 0;

   //
   // Loop through the array of top nodes and if we need to output all 
   // referrers, and through the remaining nodes in the iterators inside 
   // the combined iterator.
   //
   while(i < top_num || (config.all_refs && i < config.max_refs && miter.prev(*rptr))) {

      if(i >= top_num) {
         if(rptr->flag == OBJ_REG) {
            // ignore referrers matching any of the hiding patterns
            if(config.hidden_refs.isinlist(rptr->string))
               continue;
         }
         else if(rptr->flag == OBJ_GRP) {
            //
            // If we bundle groups, skip group nodes after we are past the
            // total group count (i.e. after the group iterator inside the
            // combined iterator has reached the end of the node sequence)
            //
            if(config.bundle_groups && i >= state.totals.t_grp_refs)
               continue;
         }
      }

      fprintf(out_fp, "<row rowid=\"%d\">\n", i+1);

      fprintf(out_fp,
          "<data><value>%d</value></data>\n"
          "<data><sum percent=\"%3.02f\">%" PRIu64 "</sum></data>\n"
          "<data><sum percent=\"%3.02f\">%" PRIu64 "</sum></data>\n",
          i+1,
          (state.totals.t_hit==0)?0:((double)rptr->count/state.totals.t_hit)*100.0,
          rptr->count,
          (state.totals.t_visits==0)?0:((double)rptr->visits/state.totals.t_visits)*100.0,
          rptr->visits);

      if (rptr->flag==OBJ_GRP) {
         fprintf(out_fp,"<data group=\"yes\"><value>%s</value></data>\n", rptr->string.c_str());
      }
      else {
         if (rptr->string.isempty())
            fprintf(out_fp,"<data><value>%s</value></data>\n", config.lang.msg_ref_dreq);
         else {
	         xml_encoder_t xml_encode(xml_encoder, xml_encoder_t::append);

            dispurl = (rptr->hexenc) ? url_decode(rptr->string, str).c_str() : rptr->string.c_str();
            dispurl = xml_encode(dispurl, false);
            href = xml_encode(rptr->string, false);

            // make a link only if the scheme is http or https
            if(!strncasecmp(href, "http", 4) && 
                  (*(cp1 = &href[4]) == ':' || (*cp1 == 's' && *++cp1 == ':')) && *++cp1 == '/' && *++cp1 == '/')
               fprintf(out_fp,"<data url=\"%s\"><value>%s</value></data>\n", href, dispurl);
            else
               fprintf(out_fp,"<data><value>%s</value></data>\n", dispurl);
         }
      }
  		fputs("</row>\n", out_fp);
  		
  		// once we reached the end, keep the pointer at the extra node
  		if(i < top_num)
         rptr++;
      i++;
   }

	fputs("</rows>\n", out_fp);
	
   fputs("</report>\n\n", out_fp);

   grpiter.close();
   iter.close();
   delete [] r_array;
}

void xml_output_t::write_top_search_strings(void)
{
   uint64_t top_num, a_ctr;
   u_int i;
   snode_t snode;
   const snode_t *sptr;
   string_t type, str;
   const char *cp1;

   if(state.totals.t_srchits == 0)
      return;

   if((a_ctr = state.totals.t_search) == 0)
      return;

   top_num = (a_ctr > config.ntop_search) ? config.ntop_search : a_ctr;

   fprintf(out_fp, "<report id=\"top_search_strings\" help=\"search_strings_report\" title=\"%s %" PRIu64 " %s %" PRIu64 " %s\" top=\"%" PRIu64 "\" total=\"%" PRIu64 "\" max=\"%" PRIu64 "\">\n", config.lang.msg_top_top, top_num, config.lang.msg_top_of, a_ctr, config.lang.msg_top_sr, top_num, state.totals.t_search, config.max_search);

   // output the top element for the top search strings section
   fputs("<top>", out_fp);
   for(i = 0; i < top_num; i++)
      fputs("<item/>", out_fp);
   fputs("</top>\n", out_fp);

	fputs("<columns>\n", out_fp);
	
   fputs("<col title=\"#\" class=\"counter_th\"/>\n", out_fp);
   fprintf(out_fp,"<col help=\"hits\" title=\"%s\" class=\"hits\" percent=\"yes\"/>\n", config.lang.msg_h_hits);
   fprintf(out_fp,"<col help=\"visits\" title=\"%s\" class=\"visits\" percent=\"yes\"/>\n", config.lang.msg_h_visits);
   fprintf(out_fp,"<col help=\"search_string\" title=\"%s\" class=\"item\"/>\n", config.lang.msg_h_search);
   
	fputs("</columns>\n", out_fp);

   database_t::reverse_iterator<snode_t> iter = state.database.rbegin_search("search.hits");

	fputs("<rows>\n", out_fp);

   for(i = 0; (i < top_num || config.all_search && i < config.max_search) && iter.prev(snode); i++) {
      fputs("<row>\n", out_fp);
      
      sptr = &snode;
      fprintf(out_fp,
         "<data><value>%d</value></data>\n"
         "<data><sum percent=\"%3.02f\">%" PRIu64 "</sum></data>\n"
         "<data><sum percent=\"%3.02f\">%" PRIu64 "</sum></data>\n",
         i+1, 
         state.totals.t_srchits ? ((double)sptr->count/state.totals.t_srchits)*100.0 : 0.0,
         sptr->count,
         state.totals.t_visits ? ((double)sptr->visits/state.totals.t_visits)*100.0 : 0.0,
         sptr->visits);

      //
      // [6]Phrase[13]webalizer css[9]File Type[3]any
      //
      cp1 = sptr->string;
      if(sptr->termcnt) {
         fputs("<data><sequence>", out_fp);
         while((cp1 = cstr2str(cp1, type)) != NULL && (cp1 = cstr2str(cp1, str)) != NULL) {
            if(type.isempty())
               fprintf(out_fp,"<value>%s</value>", xml_encode(str));
            else
               fprintf(out_fp,"<value type=\"%s\">%s</value>", type.c_str(), xml_encode(str));
         }
         fputs("</sequence></data>\n", out_fp);
      } 
      else {
         // no search type info - just print the string
         fprintf(out_fp, "<data><value>%s</value></data>\n", xml_encode(cp1));
      }
      
      fputs("</row>\n", out_fp);
   }
	fputs("</rows>\n", out_fp);
   fputs("</report>\n\n", out_fp);

   iter.close();
}

void xml_output_t::write_top_users(void)
{
   uint64_t a_ctr=0, i_reg=0, i_grp=0, i_hid=0;
   uint32_t top_num;
   u_int i;
   inode_t *iptr;
   const char *dispuser;
   inode_t *i_array;
   string_t str;

   multi_reverse_iterator<database_t::reverse_iterator, inode_t> miter;

   // return if nothing to process
   if (state.totals.t_user == 0) return;

   // get the total number, including groups and hidden items
   if((a_ctr = state.totals.t_user + state.totals.t_grp_users) == 0)
      return;

   /* get max to do... */
   top_num = (a_ctr > config.ntop_users) ? config.ntop_users : (uint32_t) a_ctr;

   // allocate as if there are no hidden items (see write_top_referrers)
   i_array = new inode_t[top_num+1];

   i = 0;

   database_t::reverse_iterator<inode_t> grpiter = state.database.rbegin_users("users.groups.hits");

   // if groups are bundled, put them first
   if(config.bundle_groups) {

      while(i < top_num && grpiter.prev(i_array[i]))
         i++;

      // if there's more, add the iterator to the combined iterator
      if(i == top_num && config.all_refs)
         miter.push(&grpiter);
   }

   database_t::reverse_iterator<inode_t> iter = state.database.rbegin_users("users.hits");

   // populate the remainder of the array
   if(i < top_num) {
      while(i < top_num && iter.prev(i_array[i])) {
         if(i_array[i].flag == OBJ_REG) {
            // ignore referrers matching any of the hiding patterns
            if(config.hidden_users.isinlist(i_array[i].string))
               continue;
         }
         else if(i_array[i].flag == OBJ_GRP) {
            // ignore groups if we did them before
            if(config.bundle_groups)
               continue;
         }

         i++;
      }

   }

   // check if all items are hidden
   if(i == 0) {
      delete [] i_array;
      return;
   }

   // if we need to output all users, add the iterator to the combined iterator
   if(i == top_num && config.all_users)
      miter.push(&iter);

   // adjust array size if it's not filled up
   if(i < top_num)
      top_num = i;

   fprintf(out_fp, "<report id=\"top_users\" help=\"users_report\" title=\"%s %" PRIu64 " %s %" PRIu64 " %s\" top=\"%d\" total=\"%" PRIu64 "\" max=\"%" PRIu64 "\" groups=\"%" PRIu64 "\">\n", config.lang.msg_top_top, top_num, config.lang.msg_top_of, state.totals.t_user, config.lang.msg_top_i, top_num, state.totals.t_user, config.max_users, state.totals.t_grp_users);

   // output the top element for the top users section
   fputs("<top>", out_fp);
   for(i = 0; i < top_num; i++)
      fputs("<item/>", out_fp);
   fputs("</top>\n", out_fp);

	fputs("<columns>\n", out_fp);

   fputs("<col title=\"#\" class=\"counter\"/>\n", out_fp);
   fprintf(out_fp,"<col help=\"hits\" title=\"%s\" class=\"hits\" percent=\"yes\"/>\n", config.lang.msg_h_hits);
   fprintf(out_fp,"<col help=\"files\" title=\"%s\" class=\"files\" percent=\"yes\"/>\n", config.lang.msg_h_files);
   fprintf(out_fp,"<col help=\"transfer\" title=\"%s\" class=\"xfer\" percent=\"yes\"/>\n", config.lang.msg_h_xfer);
   fprintf(out_fp,"<col help=\"visits\" title=\"%s\" class=\"visits\" percent=\"yes\"/>\n", config.lang.msg_h_visits);
   fprintf(out_fp,"<col help=\"req_proc_time\" title=\"%s\" class=\"time\"><columns><col title=\"%s\"/><col title=\"%s\"/></columns></col>\n", config.lang.msg_h_time, config.lang.msg_h_avg, config.lang.msg_h_max);
   fprintf(out_fp,"<col help=\"user\" title=\"%s\" class=\"item\"/>\n", config.lang.msg_h_uname);
   
	fputs("</columns>\n", out_fp);

	fputs("<rows>\n", out_fp);

   iptr = &i_array[0]; 
   i = 0; 
   while(i < top_num || (config.all_users && i < config.max_users && miter.prev(*iptr))) {
      if(i >= top_num) {
         if(iptr->flag == OBJ_REG) {
            // ignore users matching any of the hiding patterns
            if(config.hidden_users.isinlist(iptr->string))
               continue;
         }
         else if(iptr->flag == OBJ_GRP) {
            // see write_top_referrers
            if(config.bundle_groups && i >= state.totals.t_grp_users)
               continue;
         }
      }
	   
	   fputs("<row>\n", out_fp);

      fprintf(out_fp,
           "<data><value>%d</value></data>\n"
           "<data><sum percent=\"%3.02f\">%" PRIu64 "</sum></data>\n"
           "<data><sum percent=\"%3.02f\">%" PRIu64 "</sum></data>\n"
           "<data><sum percent=\"%3.02f\">%.0f</sum></data>\n"
           "<data><sum percent=\"%3.02f\">%" PRIu64 "</sum></data>\n"
           "<data><avg>%0.3f</avg><max>%0.3f</max></data>\n",
           i+1,
           (state.totals.t_hit==0)?0:((double)iptr->count/state.totals.t_hit)*100.0, iptr->count,
           (state.totals.t_file==0)?0:((double)iptr->files/state.totals.t_file)*100.0, iptr->files,
           (state.totals.t_xfer==0)?0:((double)iptr->xfer/state.totals.t_xfer)*100.0, iptr->xfer/1024.,
           (state.totals.t_visits==0)?0:((double)iptr->visit/state.totals.t_visits)*100.0, iptr->visit,
           iptr->avgtime, iptr->maxtime);

      dispuser = url_decode(iptr->string, str).c_str();
      dispuser = xml_encode(dispuser, false);
      if(iptr->flag == OBJ_GRP && config.hlite_groups)
         fprintf(out_fp,"<data group=\"yes\"><value>%s</value></data>\n", dispuser); 
      else 
         fprintf(out_fp,"<data><value>%s</value></data>\n", dispuser);
         
      fputs("</row>\n", out_fp);

  		if(i < top_num)
         iptr++;
      i++;
   }

   fputs("</rows>\n", out_fp);
   fputs("</report>\n\n", out_fp);

   grpiter.close();
   iter.close();
   delete [] i_array;
}

void xml_output_t::write_top_agents(void)
{
   uint64_t a_ctr;
   uint32_t top_num;
   u_int i;
   anode_t *aptr;
   anode_t *a_array;

   multi_reverse_iterator<database_t::reverse_iterator, anode_t> miter;

   /* don't bother if we don't have any */
   if (state.totals.t_agent == 0) return;    

   // get the total number, including groups and hidden items
   if((a_ctr = state.totals.t_agent + state.totals.t_grp_agents) == 0)
      return;

   /* get max to do... */
   top_num = (a_ctr > config.ntop_agents) ? config.ntop_agents : (uint32_t) a_ctr;

   // allocate as if there are no hidden items
   a_array = new anode_t[top_num+1];

   i = 0;

   database_t::reverse_iterator<anode_t> grpiter = state.database.rbegin_agents("agents.groups.visits");

   // if groups are bundled, put them first
   if(config.bundle_groups) {

      while(i < top_num && grpiter.prev(a_array[i]))
         i++;

      // if there's more, add the iterator to the combined iterator
      if(i == top_num && config.all_agents)
         miter.push(&grpiter);
   }

   database_t::reverse_iterator<anode_t> iter = state.database.rbegin_agents("agents.visits");

   // populate the remainder of the array
   if(i < top_num) {
      while(i < top_num && iter.prev(a_array[i])) {
         if(a_array[i].flag == OBJ_REG) {
            // ignore agents matching any of the hiding patterns
            if(config.hide_robots  && a_array[i].robot || config.hidden_agents.isinlist(a_array[i].string))
               continue;
         }
         else if(a_array[i].flag == OBJ_GRP) {
            // ignore groups if we did them before
            if(config.bundle_groups)
               continue;
         }

         i++;
      }
   }

   // check if all items are hidden
   if(i == 0) {
      delete [] a_array;
      return;
   }

   // if we need to output all URLs, add the iterator to the combined iterator
   if(i == top_num && config.all_agents)
      miter.push(&iter);

   // adjust array size if it's not filled up
   if(i < top_num)
      top_num = i;

   fprintf(out_fp, "<report id=\"top_user_agents\" help=\"user_agents_report\" title=\"%s %u %s %" PRIu64 " %s\" top=\"%u\" total=\"%" PRIu64 "\" max=\"%" PRIu64 "\" groups=\"%" PRIu64 "\">\n", config.lang.msg_top_top, top_num, config.lang.msg_top_of, state.totals.t_agent, config.lang.msg_top_a, top_num, state.totals.t_agent, config.max_agents, state.totals.t_grp_agents);

   // output the top element for the top agents section
   fputs("<top>", out_fp);
   for(i = 0; i < top_num; i++)
      fputs("<item/>", out_fp);
   fputs("</top>\n", out_fp);

   fputs("<columns>\n", out_fp);
   
   fputs("<col title=\"#\" class=\"counter\"/>\n", out_fp);
   fprintf(out_fp,"<col help=\"hits\" title=\"%s\" class=\"hits\" percent=\"yes\"/>\n", config.lang.msg_h_hits);
   fprintf(out_fp,"<col help=\"transfer\" title=\"%s\" class=\"xfer\" percent=\"yes\"/>\n", config.lang.msg_h_xfer);
   fprintf(out_fp,"<col help=\"visits\" title=\"%s\" class=\"visits\" percent=\"yes\"/>\n", config.lang.msg_h_visits);
   fprintf(out_fp,"<col help=\"user_agent\" title=\"%s\" class=\"item\"/>\n", config.lang.msg_h_agent);
   
   fputs("</columns>\n", out_fp);

	fputs("<rows>\n", out_fp);

   aptr = &a_array[0];
   i = 0;
   while(i < top_num || (config.all_agents && i < config.max_agents && miter.prev(*aptr))) {

      if(i >= top_num) {
         if(aptr->flag == OBJ_REG) {
            // ignore agents matching any of the hiding patterns
            if(config.hide_robots  && aptr->robot || config.hidden_agents.isinlist(aptr->string))
               continue;
         }
         else if(aptr->flag == OBJ_GRP) {
            // see write_top_referrers
            if(config.bundle_groups && i >= state.totals.t_grp_agents)
               continue;
         }
      }

      fputs("<row>\n", out_fp);

      fprintf(out_fp,
          "<data><value>%d</value></data>\n"
          "<data><sum percent=\"%3.02f\">%" PRIu64 "</sum></data>\n"
          "<data><sum percent=\"%3.02f\">%.0f</sum></data>\n"
          "<data><sum percent=\"%3.02f\">%" PRIu64 "</sum></data>\n",
          i+1,
          (state.totals.t_hit==0)?0:((double)aptr->count/state.totals.t_hit)*100.0, aptr->count,
          (state.totals.t_xfer==0)?0:((double)aptr->xfer/state.totals.t_xfer)*100.0, aptr->xfer/1024.,
          (state.totals.t_visits==0)?0:((double)aptr->visits/state.totals.t_visits)*100.0, aptr->visits);

      if (aptr->flag == OBJ_GRP)
         fprintf(out_fp,"<data group=\"yes\"><value>%s</value></data>\n", aptr->string.c_str()); 
      else 
		   fprintf(out_fp,"<data%s><value>%s</value></data>\n", aptr->robot ? " robot=\"yes\"" : "", xml_encode(aptr->string.c_str()));

      fputs("</row>\n", out_fp);

  		if(i < top_num)
         aptr++;
      i++;
   }

   fputs("</rows>\n", out_fp);
   fputs("</report>\n\n", out_fp);

   iter.close();
   grpiter.close();
   delete [] a_array;
}

void xml_output_t::write_top_countries(void)
{
   u_int tot_num=0, tot_ctry=0;
   uint64_t i,j;
   uint64_t t_hit, t_file, t_page, t_visits, t_ohit, t_ofile, t_ovisits;
   uint64_t t_xfer, t_oxfer;
   string_t pie_title;
   string_t pie_fname;
   const ccnode_t **ccarray;
   const ccnode_t *tptr;
   uint64_t pie_data[10];
   const char *pie_legend[10];

   if(state.cc_htab.size() == 0)
      return;

   // exclude robot activity
   t_hit = state.totals.t_hit - state.totals.t_rhits;
   t_file = state.totals.t_file - state.totals.t_rfiles;
   t_page = state.totals.t_page - state.totals.t_rpages;
   t_xfer = state.totals.t_xfer - state.totals.t_rxfer;
   
   // only include human activity
   t_visits = state.totals.t_hvisits_end;

   // allocate and load the country array
   ccarray = new const ccnode_t*[state.cc_htab.size()];

   state.cc_htab.load_array(ccarray);

   // find the first node with a zero count
   for(i = 0; i < state.cc_htab.size(); i++, tot_ctry++) {
      if(ccarray[i]->count == 0) 
         break;
   }

   // swap the nodes with zero and non-zero counts
   for(j = i+1; j < state.cc_htab.size(); j++) {
      if(ccarray[j]->count) {           
         tptr = ccarray[i];
         ccarray[i++] = ccarray[j];    // the next one always has count == 0
         ccarray[j] = tptr;
         tot_ctry++;
      }
   }

   // sort those at the beginning of the array
   qsort(ccarray, tot_ctry, sizeof(ccnode_t*), qs_cc_cmpv);

   // select how many top entries to report
   tot_num = (tot_ctry > config.ntop_ctrys) ? config.ntop_ctrys : tot_ctry;

   //
   //
   //
   pie_title.format("%s %s %d",config.lang.msg_ctry_use, lang_t::l_month[state.totals.cur_tstamp.month-1],state.totals.cur_tstamp.year);
   pie_fname.format("ctry_usage_%04d%02d.png",state.totals.cur_tstamp.year,state.totals.cur_tstamp.month);

   if(config.html_ext_lang)
      pie_fname = pie_fname + '.' + config.lang.language_code;

   for (i=0;i<10;i++) {
      pie_data[i]=0;                             /* init data array      */
      pie_legend[i] = NULL;
   }
   j = MIN(tot_num, 10);                         /* ensure data size     */

   for(i = 0; i < j; i++) {
      pie_data[i]=ccarray[i]->visits;            /* load the array       */
      pie_legend[i]=ccarray[i]->cdesc;
   }

   // generate the image, if requested
   if(makeimgs)
      graph.pie_chart(pie_fname, pie_title, t_visits, pie_data, pie_legend);
      
   //
   //
   //
   fprintf(out_fp, "<report id=\"top_countries\" help=\"countries_report\" title=\"%s %d %s %d %s\" top=\"%d\" total=\"%d\">\n", config.lang.msg_top_top, tot_num, config.lang.msg_top_of, tot_ctry, config.lang.msg_top_c, tot_num, tot_ctry);

   // create and populate the graph element
   fprintf(out_fp, "<graph title=\"%s %s %d\">\n", config.lang.msg_ctry_use, lang_t::l_month[state.totals.cur_tstamp.month-1], state.totals.cur_tstamp.year);

   fprintf(out_fp, "<image filename=\"%s\" width=\"%d\" height=\"%d\"/>\n", pie_fname.c_str(), 512, 300);

   t_ohit = t_hit;
   t_ofile = t_file;
   t_oxfer = t_xfer;
   t_ovisits = t_visits;
   
   j = MIN(tot_num, 10);
   
   // output all slice elements and compute values for the last slice
   fputs("<slices>", out_fp);
   for(i = 0; i < j; i++) {
      t_ohit -= ccarray[i]->count;
      t_ofile -= ccarray[i]->files;
      t_ovisits -= ccarray[i]->visits;
      t_oxfer -= ccarray[i]->xfer;
      fputs("<slice/>", out_fp);
   }
   fputs("</slices>\n", out_fp);
   
   // if there is an extra slice, output it here
   if(t_ohit) {
      fputs("<other>\n", out_fp);
      
      // mimic the report row, so the same code can be used to process it
      fprintf(out_fp,
              "<data><value>0</value></data>\n"
              "<data><sum percent=\"%3.02f\">%" PRIu64 "</sum></data>\n"
              "<data><sum percent=\"%3.02f\">%" PRIu64 "</sum></data>\n"
              "<data><sum percent=\"%3.02f\">%.0f</sum></data>\n"
              "<data><sum percent=\"%3.02f\">%" PRIu64 "</sum></data>\n"
              "<data ccode=\"_\"><value>%s</value></data>\n",
              PCENT(t_ohit, t_hit), t_ohit,
              PCENT(t_ofile, t_file), t_ofile,
              PCENT(t_oxfer, t_xfer), t_oxfer/1024.,
              PCENT(t_ovisits, t_visits), t_ovisits,
              config.lang.msg_h_other);
      
	   fputs("</other>\n", out_fp);
   }
   
	fputs("</graph>\n", out_fp);

   // column descriptors
	fputs("<columns>\n", out_fp);
	
   fputs("<col title=\"#\" class=\"counter\"/>\n", out_fp);
   fprintf(out_fp,"<col help=\"hits\" title=\"%s\" class=\"hits\" percent=\"yes\"/>\n", config.lang.msg_h_hits);
   fprintf(out_fp,"<col help=\"files\" title=\"%s\" class=\"files\" percent=\"yes\"/>\n", config.lang.msg_h_files);
   fprintf(out_fp,"<col help=\"transfer\" title=\"%s\" class=\"xfer\" percent=\"yes\"/>\n", config.lang.msg_h_xfer);
   fprintf(out_fp,"<col help=\"visits\" title=\"%s\" class=\"visits\" percent=\"yes\"/>\n", config.lang.msg_h_visits);
   fprintf(out_fp,"<col help=\"country\" title=\"%s\" class=\"item\"/>\n", config.lang.msg_h_ctry);

	fputs("</columns>\n", out_fp);

	fputs("<rows>\n", out_fp);

   for(i = 0; i < tot_num; i++) {
      if(ccarray[i]->count != 0) {
         fprintf(out_fp, "<row rowid=\"%" PRIu64 "\">\n", i+1);
         
         fprintf(out_fp,
              "<data><value>%" PRIu64 "</value></data>\n"
              "<data><sum percent=\"%3.02f\">%" PRIu64 "</sum></data>\n"
              "<data><sum percent=\"%3.02f\">%" PRIu64 "</sum></data>\n"
              "<data><sum percent=\"%3.02f\">%.0f</sum></data>\n"
              "<data><sum percent=\"%3.02f\">%" PRIu64 "</sum></data>\n"
              "<data ccode=\"%s\"><value>%s</value></data>\n",
              i+1, 
              PCENT(ccarray[i]->count,t_hit), ccarray[i]->count,
              PCENT(ccarray[i]->files,t_file), ccarray[i]->files,
              PCENT(ccarray[i]->xfer,t_xfer), ccarray[i]->xfer/1024.,
              PCENT(ccarray[i]->visits,t_visits), ccarray[i]->visits,
              ccarray[i]->ccode.c_str(), ccarray[i]->cdesc.c_str());
              
         fputs("</row>\n", out_fp);
      }
   }
   delete [] ccarray;

	fputs("</rows>\n", out_fp);

   // output a note that robot activity is not included in this report 
   if(state.totals.t_rhits)
      fprintf(out_fp,"<notes>\n<note>%s</note>\n</notes>\n", xml_encode(config.lang.msg_misc_robots));

   fputs("</report>\n\n", out_fp);
}

int xml_output_t::write_monthly_report(void)
{
   string_t xml_fname, period;

   /* fill in filenames */
   xml_fname.format("usage_%04d%02d.xml",state.totals.cur_tstamp.year, state.totals.cur_tstamp.month);

   if((out_fp = open_out_file(xml_fname)) == NULL)
      return 1;

   write_xml_head(false);

   write_monthly_totals();
   
   write_daily_totals();

   write_hourly_totals();

   // top URL's (by hits)
   if(config.ntop_urls) 
      write_top_urls(false);

   // top URL's (by kbytes)
	if(config.ntop_urlsK)
      write_top_urls(true);

   // top entry pages
	if(config.ntop_entry)
      write_top_entry(true);

   // top exit pages
	if(config.ntop_exit)
      write_top_entry(false);

   // top downloads
   if(config.ntop_downloads) 
      write_top_downloads();

   // top errors
	if(config.ntop_errors) 
      write_top_errors();

   // top hosts (by hits)
	if (config.ntop_sites) 
      write_top_hosts(false); 

   // top hosts (by kbytes)
	if (config.ntop_sitesK)                      
		write_top_hosts(true);

   // top referrers
	if(config.ntop_refs) 
      write_top_referrers();   

   // top search strings table
	if (config.ntop_search) 
      write_top_search_strings(); 

   // top user names
	if (config.ntop_users) 
      write_top_users(); 

   // top user agents table
	if (config.ntop_agents) 
      write_top_agents(); 

   // top countries
   if(config.ntop_ctrys) 
      write_top_countries();

   write_xml_tail();
   
   fclose(out_fp);

   return 0;
}

void xml_output_t::write_xml_head(bool index)
{
   const hist_month_t *hptr;
   string_t date, time, tzname;

   fprintf(out_fp, "<?xml version=\"1.0\" encoding=\"utf-8\" standalone=\"%s\"?>\n", !config.use_ext_ent ? "yes" : "no");
   
   fputs("<!DOCTYPE sswdoc [<!ENTITY nbsp \"&#160;\">", out_fp);
   if(config.use_ext_ent && !config.help_file.isempty())
      fprintf(out_fp, "<!ENTITY help SYSTEM \"%s\">", xml_encode(config.help_file.c_str()));
   fputs("]>\n", out_fp);
   
   fprintf(out_fp, "<?xml-stylesheet type=\"text/xsl\" href=\"%s%s\" ?>\n", config.xsl_path.c_str(), index ? "index.xsl" : "usage.xsl");

   /* Standard header comments */
   fputs("<!-- Copyright (c) 2004-2016, Stone Steps Inc.\n", out_fp);
   fputs("             http://www.stonesteps.ca\n\n", out_fp);
   fputs("     Distributed under the GNU GPL, Version 2\n", out_fp);
   fputs("            Full text may be found at:\n", out_fp);
   fputs("      http://www.stonesteps.ca/legal/gpl.asp\n", out_fp);
   fputs("-->\n", out_fp);

   fprintf(out_fp, "<sswdoc version=\"1.0\" lang=\"%s\">\n", config.lang.language_code);
   
   // application info
   fputs("<application>\n", out_fp);
   
   // application version
   fprintf(out_fp,"<version>%s</version>\n", state_t::get_app_version().c_str());

   // application configuration
   fputs("<config>\n", out_fp);
   fprintf(out_fp,"<csspath>%s</csspath>\n", xml_encode(config.html_css_path));
   fprintf(out_fp,"<jspath>%s</jspath>\n", xml_encode(config.html_js_path));
   fprintf(out_fp,"<xslpath>%s</xslpath>\n", xml_encode(config.xsl_path));
   fprintf(out_fp,"<logtype>%s</logtype>\n", xml_encode(config.get_log_type_desc()));
   fprintf(out_fp,"<graphtype>%s</graphtype>\n", xml_encode(config.graph_type));

   // output graph colors defined in the configuration
   if(!graph.is_default_background_color()) fprintf(out_fp,"<color name=\"graph_bg_color\">%06X</color>\n", graph.get_background_color());
   if(!graph.is_default_gridline_color()) fprintf(out_fp,"<color name=\"graph_grid_color\">%06X</color>\n", graph.get_gridline_color());
   if(!graph.is_default_shadow_color()) fprintf(out_fp,"<color name=\"graph_shadow_color\">%06X</color>\n", graph.get_shadow_color());
   if(!graph.is_default_title_color()) fprintf(out_fp,"<color name=\"graph_title_color\">%06X</color>\n", graph.get_title_color());
   if(!graph.is_default_hits_color()) fprintf(out_fp,"<color name=\"graph_hits_color\">%06X</color>\n", graph.get_hits_color());
   if(!graph.is_default_files_color()) fprintf(out_fp,"<color name=\"graph_files_color\">%06X</color>\n", graph.get_files_color());
   if(!graph.is_default_pages_color()) fprintf(out_fp,"<color name=\"graph_pages_color\">%06X</color>\n", graph.get_pages_color());
   if(!graph.is_default_volume_color()) fprintf(out_fp,"<color name=\"graph_xfer_color\">%06X</color>\n", graph.get_volume_color());
   if(!graph.is_default_visits_color()) fprintf(out_fp,"<color name=\"graph_visits_color\">%06X</color>\n", graph.get_visits_color());
   if(!graph.is_default_hosts_color()) fprintf(out_fp,"<color name=\"graph_hosts_color\">%06X</color>\n", graph.get_hosts_color());
   
   fputs("</config>\n", out_fp);

   fputs("</application>\n", out_fp);

   // report summary
   fputs("<summary>\n", out_fp);

   // output when the report was generated
   fputs("<created>\n", out_fp);

   cur_time_ex(config.local_time, date, time, &tzname);
   fprintf(out_fp,"<date>%s</date>\n<time tzname=\"%s\">%s</time>\n", date.c_str(), xml_encode(tzname.c_str()), time.c_str());

   fputs("</created>\n", out_fp);

   // host name
   fprintf(out_fp,"<hostname>%s</hostname>\n", config.hname.c_str());
   
   // describe the period covered by the report
   if(index) {
      fprintf(out_fp, "<period months=\"%d\">\n", state.history.disp_length());

      if(state.history.size()) {
         // output the first month
         hptr = state.history.first();
         
         fputs("<start>\n", out_fp);
         fprintf(out_fp,"<year>%d</year>\n<month name=\"%s\">%d</month>\n", hptr->year, xml_encode(lang_t::l_month[hptr->month-1]), hptr->month);
         fputs("</start>\n", out_fp);

         // and the last
         hptr = state.history.last();
         
         fputs("<end>\n", out_fp);
         fprintf(out_fp,"<year>%d</year>\n<month name=\"%s\">%d</month>\n", hptr->year, xml_encode(lang_t::l_month[hptr->month-1]), hptr->month);
         fputs("</end>\n", out_fp);
      }
      
      fputs("</period>\n", out_fp);
   }
   else {
      fputs("<period months=\"1\">\n", out_fp);
      
      fputs("<start>\n", out_fp);
      fprintf(out_fp,"<year>%d</year>\n<month name=\"%s\">%d</month>\n<day>%d</day>\n", state.totals.cur_tstamp.year, xml_encode(lang_t::l_month[state.totals.cur_tstamp.month-1]), state.totals.cur_tstamp.month, state.totals.f_day);
      fputs("</start>\n", out_fp);
      
      fputs("<end>\n", out_fp);
      fprintf(out_fp,"<year>%d</year>\n<month name=\"%s\">%d</month>\n<day>%d</day>\n", state.totals.cur_tstamp.year, xml_encode(lang_t::l_month[state.totals.cur_tstamp.month-1]), state.totals.cur_tstamp.month, state.totals.l_day);
      fputs("</end>\n", out_fp);
      
      fputs("</period>\n", out_fp);
   }

   fputs("</summary>\n", out_fp);

   // create text dictionary
   fputs("<dictionary>\n", out_fp);
   
   // Generated
   fprintf(out_fp, "<text name=\"html_hdr_generated\">%s</text>\n", config.lang.msg_hhdr_gt);
   
   // Summary Period
   fprintf(out_fp, "<text name=\"html_hdr_summary_period\">%s</text>\n", config.lang.msg_hhdr_sp);

   // Last N Months
   fprintf(out_fp, "<text name=\"html_hdr_period_last\">%s</text>\n", config.lang.msg_main_plst);
   fprintf(out_fp, "<text name=\"html_hdr_period_months\">%s</text>\n", config.lang.msg_main_pmns);

   // Usage Statistics for
   fprintf(out_fp, "<text name=\"html_hdr_usage_stats_for\">%s</text>\n", config.rpt_title.c_str());
   
   // links
   fprintf(out_fp,"<text name=\"link_summary\">%s</text>\n", config.lang.msg_hlnk_sum);
   fprintf(out_fp,"<text name=\"link_daily_stats\">%s</text>\n", config.lang.msg_hlnk_ds);
   fprintf(out_fp,"<text name=\"link_hourly_stats\">%s</text>\n", config.lang.msg_hlnk_hs);
   fprintf(out_fp,"<text name=\"link_urls\">%s</text>\n", config.lang.msg_hlnk_u);
   fprintf(out_fp,"<text name=\"link_entry\">%s</text>\n", config.lang.msg_hlnk_en);
   fprintf(out_fp,"<text name=\"link_exit\">%s</text>\n", config.lang.msg_hlnk_ex);
   fprintf(out_fp,"<text name=\"link_search\">%s</text>\n", config.lang.msg_hlnk_sr);
   fprintf(out_fp,"<text name=\"link_downloads\">%s</text>\n", config.lang.msg_hlnk_dl);
   fprintf(out_fp,"<text name=\"link_errors\">%s</text>\n", config.lang.msg_hlnk_err);
   fprintf(out_fp,"<text name=\"link_hosts\">%s</text>\n", config.lang.msg_hlnk_s);
   fprintf(out_fp,"<text name=\"link_referrers\">%s</text>\n", config.lang.msg_hlnk_r);
   fprintf(out_fp,"<text name=\"link_users\">%s</text>\n", config.lang.msg_hlnk_i);
   fprintf(out_fp,"<text name=\"link_agents\">%s</text>\n", config.lang.msg_hlnk_a);
   fprintf(out_fp,"<text name=\"link_countries\">%s</text>\n", config.lang.msg_hlnk_c);

   // view all ...
   fprintf(out_fp,"<text name=\"html_view_all_hosts\">%s</text>\n", config.lang.msg_v_hosts);
   fprintf(out_fp,"<text name=\"html_view_all_urls\">%s</text>\n", config.lang.msg_v_urls);
   fprintf(out_fp,"<text name=\"html_view_all_referrers\">%s</text>\n", config.lang.msg_v_refs);
   fprintf(out_fp,"<text name=\"html_view_all_agents\">%s</text>\n", config.lang.msg_v_agents);
   fprintf(out_fp,"<text name=\"html_view_all_search\">%s</text>\n", config.lang.msg_v_search);
   fprintf(out_fp,"<text name=\"html_view_all_users\">%s</text>\n", config.lang.msg_v_users);
   fprintf(out_fp,"<text name=\"html_view_all_downloads\">%s</text>\n", config.lang.msg_v_downloads);
   fprintf(out_fp,"<text name=\"html_view_all_errors\">%s</text>\n", config.lang.msg_v_errors);

   fprintf(out_fp,"<text name=\"html_max_items\">%s</text>\n", config.lang.msg_max_items);
   
   fputs("</dictionary>\n", out_fp);

   // include language-specific help file
   if(config.use_ext_ent) {
      if(!config.help_file.isempty())
         fputs("&help;", out_fp);
   }
   else {
      if(!config.help_xml.isempty())
         fputs(config.help_xml, out_fp);
   }
   fputs("\n", out_fp);
}

void xml_output_t::write_xml_tail(void)
{
   fputs("</sswdoc>\n", out_fp);
}

#include "linklist_tmpl.cpp"
#include "encoder_tmpl.cpp"

//
// Instantiate linked list templates (see comments at the end of hashtab.cpp)
//
template class list_t<multi_reverse_iterator<database_t::reverse_iterator, unode_t>::itnode_t>;
template class list_t<multi_reverse_iterator<database_t::reverse_iterator, hnode_t>::itnode_t>;
template class list_t<multi_reverse_iterator<database_t::reverse_iterator, rnode_t>::itnode_t>;
template class list_t<multi_reverse_iterator<database_t::reverse_iterator, inode_t>::itnode_t>;
template class list_t<multi_reverse_iterator<database_t::reverse_iterator, anode_t>::itnode_t>;
template class list_t<multi_reverse_iterator<database_t::reverse_iterator, dlnode_t>::itnode_t>;

// XML encoder
template class buffer_encoder_t<encode_xml_char>;
