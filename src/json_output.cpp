/*
   webalizer - a web server log analysis program

   Copyright (c) 2004-2021, Stone Steps Inc. (www.stonesteps.ca)
   Copyright (C) 1997-2001  Bradford L. Barrett (brad@mrunix.net)

   See COPYING and Copyright files for additional licensing and copyright information 
   
   json_output.cpp
*/
#include "pch.h"

#include <ctime>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cctype>

#ifndef _WIN32
#include <unistd.h>
#endif

#include <sys/types.h>

#include "lang.h"
#include "hashtab.h"
#include "linklist.h"
#include "util_http.h"
#include "util_url.h"
#include "util_math.h"
#include "exception.h"
#include "json_output.h"
#include "preserve.h"

#include <stdexcept>

json_output_t::json_output_t(const config_t& config, const state_t& state) :
      output_t(config, state),
      buffer(BUFSIZE),
      buffer_formatter(buffer, BUFSIZE, buffer_formatter_t::overwrite)
{
}

json_output_t::~json_output_t(void)
{
}

bool json_output_t::init_output_engine(void)
{
   return true;
}

void json_output_t::cleanup_output_engine(void)
{
}

const char *json_output_t::json_encode(const char *str)
{
   return buffer_formatter.format(encode_string<encode_char_js>, str);
}

const char *json_output_t::json_encode(const char *str, size_t len)
{
   return buffer_formatter.format(encode_string_len<encode_char_js>, str, len);
}

uint64_t json_output_t::get_json_id(u_int year, u_int day, uint64_t node_id)
{
   //
   // The largest integer in JavaScript can be represented with 52
   // bits, which is the significand of a double-precision floating
   // point number. Mongo DB, on the other hand, can read signed
   // 64-bit numbers via Extended JSON, as described on this page:
   // 
   // https://docs.mongodb.com/manual/reference/mongodb-extended-json/#mongodb-bsontype-Int64
   // 
   // The high-order sign bit must remain zero and we need 12 bits
   // for year and 4 for month, which leaves us with 47 bits we can
   // use for the node ID.
   // 
   // Throwing an exception and preventing reports from being generated,
   // doesn't make much sense at this point, so just report that the
   // node ID was overflown.
   //
   if(node_id & (~ (uint64_t) 0 << 47))
      fprintf(stderr, "Node ID %" PRIu64  " exceeds 48 bits in size and was mangled\n", node_id);

   // 1 bit (sign, always cleared) 12 bits (year) 4 bits (month) and 47 bits (node_id)
   return ((uint64_t) year << (32 + 15 + 4)) + ((uint64_t) day << (32 + 15)) + node_id;
}

int json_output_t::write_main_index()
{
   return 0;
}

int json_output_t::write_monthly_report(void)
{
   dump_totals();
   dump_daily();
   dump_hourly();

   dump_all_downloads();
   dump_all_urls();     
   dump_all_errors();    
   dump_all_hosts();   
   dump_all_refs();    
   dump_all_search();  
   dump_all_users();  
   dump_all_agents();
   dump_all_countries();
   dump_all_cities();
   dump_all_asn();

   return 0;
}

void json_output_t::dump_totals(void)
{
   u_int days_in_month;
   uint64_t max_files=0,max_hits=0,max_visits=0,max_pages=0;
   uint64_t max_xfer=0;
   FILE     *out_fp;
   string_t filename;

   days_in_month = (state.totals.l_day - state.totals.f_day) + 1;

   for(size_t i = 0; i < 31 ; i++) {
      if (state.t_daily[i].tm_hits > max_hits)     max_hits  = state.t_daily[i].tm_hits;
      if (state.t_daily[i].tm_files > max_files)   max_files = state.t_daily[i].tm_files;
      if (state.t_daily[i].tm_pages > max_pages)   max_pages = state.t_daily[i].tm_pages;
      if (state.t_daily[i].tm_visits > max_visits) max_visits= state.t_daily[i].tm_visits;
      if (state.t_daily[i].tm_xfer > max_xfer)     max_xfer  = state.t_daily[i].tm_xfer;
   }

   // name this "usage" for consistency, even though it's vague as to what it contains
   filename.format("%s/usage_%04d%02d.json",
      (!config.dump_path.isempty())? config.dump_path.c_str() : ".", state.totals.cur_tstamp.year,state.totals.cur_tstamp.month);

   if((out_fp = open_out_file(filename)) == nullptr) {
      fprintf(stderr, "%s %s\n", config.lang.msg_no_open, filename.c_str());
      return;
   }

   fputs("{\n", out_fp);

   // there's plenty of room in a 32-bit integer for a simple combined year/month identifier
   fprintf(out_fp, "\"_id\": %u,\n", state.totals.cur_tstamp.year * 100 + state.totals.cur_tstamp.month);

   fprintf(out_fp, "\"year\": %u,\n", state.totals.cur_tstamp.year);
   fprintf(out_fp, "\"month\": %u,\n", state.totals.cur_tstamp.month);

   fputs("\"totals\": {\n", out_fp);

   fprintf(out_fp,"\"hits\": {\"$numberLong\": \"%" PRIu64 "\"},\n", state.totals.t_hit);
   fprintf(out_fp,"\"files\": {\"$numberLong\": \"%" PRIu64 "\"},\n", state.totals.t_file);
   fprintf(out_fp,"\"pages\": {\"$numberLong\": \"%" PRIu64 "\"},\n", state.totals.t_page);
   fprintf(out_fp,"\"visits\": {\"$numberLong\": \"%" PRIu64 "\"},\n", state.totals.t_visits);
   fprintf(out_fp,"\"xfer\": {\"$numberLong\": \"%" PRIu64 "\"},\n", state.totals.t_xfer);
   fprintf(out_fp,"\"downloads\": {\"$numberLong\": \"%" PRIu64 "\"},\n", state.totals.t_downloads);

   fprintf(out_fp,"\"hosts\": {\"$numberLong\": \"%" PRIu64 "\"},\n", state.totals.t_hosts);
   fprintf(out_fp,"\"urls\": {\"$numberLong\": \"%" PRIu64 "\"},\n", state.totals.t_url);
   fprintf(out_fp,"\"referrers\": {\"$numberLong\": \"%" PRIu64 "\"},\n", state.totals.t_ref);
   fprintf(out_fp,"\"users\": {\"$numberLong\": \"%" PRIu64 "\"},\n", state.totals.t_user);
   fprintf(out_fp,"\"agents\": {\"$numberLong\": \"%" PRIu64 "\"}\n", state.totals.t_agent);

   fputs("},\n", out_fp);

   // output human totals if robot or spammer filters are configured
   if(config.spam_refs.size() || config.robots.size()) {
      fputs("\"humans\": {\n", out_fp);

      fprintf(out_fp,"\"hits\": {\"$numberLong\": \"%" PRIu64 "\"},\n", state.totals.t_hit - state.totals.t_rhits - state.totals.t_spmhits);
      fprintf(out_fp,"\"files\": {\"$numberLong\": \"%" PRIu64 "\"},\n", state.totals.t_file - state.totals.t_rfiles - state.totals.t_sfiles);
      fprintf(out_fp,"\"pages\": {\"$numberLong\": \"%" PRIu64 "\"},\n", state.totals.t_page - state.totals.t_rpages - state.totals.t_spages);
      fprintf(out_fp,"\"xfer\": {\"$numberLong\": \"%" PRIu64 "\"},\n", state.totals.t_xfer - state.totals.t_rxfer - state.totals.t_sxfer);

      fprintf(out_fp,"\"hosts\": {\"$numberLong\": \"%" PRIu64 "\"},\n", state.totals.t_hosts - state.totals.t_rhosts - state.totals.t_shosts);
      fprintf(out_fp,"\"visits\": {\"$numberLong\": \"%" PRIu64 "\"},\n", state.totals.t_hvisits_end);

      // output the conversion section only if target URLs or downloads are configured
      if(config.target_urls.size() || config.downloads.size()) {
         fputs("\"converted\": {\n", out_fp);

         fprintf(out_fp,"\"hosts\": {\"$numberLong\": \"%" PRIu64 "\"},\n", state.totals.t_hosts_conv);
         fprintf(out_fp,"\"visits\": {\"$numberLong\": \"%" PRIu64 "\"},\n", state.totals.t_visits_conv);
         fprintf(out_fp,"\"rate\": %.6f\n", (double)state.totals.t_hosts_conv*100./(state.totals.t_hosts - state.totals.t_rhosts - state.totals.t_shosts));

         fputs("},\n", out_fp);
      }
      
      // output human per-visit totals if there are ended human visits
      if(state.totals.t_hvisits_end) {
         fputs("\"visit\": {\n", out_fp);  // $$$ better name

         fprintf(out_fp,"\"hits\": {\"avg\": %.6f, \"max\": {\"$numberLong\": \"%" PRIu64 "\"}},\n", (double) (state.totals.t_hit - state.totals.t_rhits - state.totals.t_spmhits)/state.totals.t_hvisits_end, state.totals.max_hv_hits);
         fprintf(out_fp,"\"files\": {\"avg\": %.6f, \"max\": {\"$numberLong\": \"%" PRIu64 "\"}},\n", (double) (state.totals.t_file - state.totals.t_rfiles - state.totals.t_sfiles)/state.totals.t_hvisits_end, state.totals.max_hv_files);
         fprintf(out_fp,"\"pages\": {\"avg\": %.6f, \"max\": {\"$numberLong\": \"%" PRIu64 "\"}},\n", (double) (state.totals.t_page - state.totals.t_rpages - state.totals.t_spages)/state.totals.t_hvisits_end, state.totals.max_hv_pages);
         fprintf(out_fp,"\"xfer\": {\"avg\": %.6f, \"max\": {\"$numberLong\": \"%" PRIu64 "\"}},\n", (double) (state.totals.t_xfer - state.totals.t_rxfer - state.totals.t_sxfer)/state.totals.t_hvisits_end, state.totals.max_hv_xfer);

         fprintf(out_fp,"\"duration\": {\"avg\": %.6f, \"max\": %.6f}", state.totals.t_visit_avg/60., state.totals.t_visit_max/60.);

         if(state.totals.t_visits_conv)
            fprintf(out_fp,",\n\"duration_converted\": {\"avg\": %.6f, \"max\": %.6f}", state.totals.t_vconv_avg/60., state.totals.t_vconv_max/60.);
            
         fputs("\n}\n", out_fp);
      }

      // humans: {}
      fputs("},\n", out_fp);
   }

   if(state.totals.t_rhits) {
      fputs("\"robots\": {\n", out_fp);

      fprintf(out_fp,"\"hits\": {\"$numberLong\": \"%" PRIu64 "\"},\n", state.totals.t_rhits);
      fprintf(out_fp,"\"files\": {\"$numberLong\": \"%" PRIu64 "\"},\n", state.totals.t_rfiles);
      fprintf(out_fp,"\"pages\": {\"$numberLong\": \"%" PRIu64 "\"},\n", state.totals.t_rpages);
      fprintf(out_fp,"\"errors\": {\"$numberLong\": \"%" PRIu64 "\"},\n", state.totals.t_rerrors);
      fprintf(out_fp,"\"xfer\": {\"$numberLong\": \"%" PRIu64 "\"},\n", state.totals.t_rxfer);
      fprintf(out_fp,"\"visits\": {\"$numberLong\": \"%" PRIu64 "\"},\n", state.totals.t_rvisits_end);
      fprintf(out_fp,"\"hosts\": {\"$numberLong\": \"%" PRIu64 "\"}\n", state.totals.t_rhosts);

      fputs("},\n", out_fp);
   }

   if(state.totals.t_spmhits) {
      fputs("\"spammers\": {\n", out_fp);

      fprintf(out_fp,"\"hits\": {\"$numberLong\": \"%" PRIu64 "\"},\n", state.totals.t_spmhits);
      fprintf(out_fp,"\"xfer\": {\"$numberLong\": \"%" PRIu64 "\"},\n", state.totals.t_sxfer);
      fprintf(out_fp,"\"visits\": {\"$numberLong\": \"%" PRIu64 "\"},\n", state.totals.t_svisits_end);
      fprintf(out_fp,"\"hosts\": {\"$numberLong\": \"%" PRIu64 "\"}\n", state.totals.t_shosts);

      fputs("},\n", out_fp);
   }
      
   fputs("\"per\": {\n", out_fp);

   fputs("\"hour\": {\n", out_fp);
   fprintf(out_fp,"\"hits\": {\"avg\": %.6f, \"max\": {\"$numberLong\": \"%" PRIu64 "\"}}\n", (double) state.totals.t_hit/(24*days_in_month), state.totals.hm_hit);
   fputs("},\n", out_fp);

   fputs("\"day\": {\n", out_fp);
   fprintf(out_fp,"\"hits\": {\"avg\": %.6f, \"max\": {\"$numberLong\": \"%" PRIu64 "\"}},\n", (double) state.totals.t_hit/days_in_month, max_hits);
   fprintf(out_fp,"\"files\": {\"avg\": %.6f, \"max\": {\"$numberLong\": \"%" PRIu64 "\"}},\n", (double) state.totals.t_file/days_in_month, max_files);
   fprintf(out_fp,"\"pages\": {\"avg\": %.6f, \"max\": {\"$numberLong\": \"%" PRIu64 "\"}},\n", (double) state.totals.t_page/days_in_month, max_pages);
   fprintf(out_fp,"\"visits\": {\"avg\": %.6f, \"max\": {\"$numberLong\": \"%" PRIu64 "\"}},\n", (double) state.totals.t_visits/days_in_month, max_visits);
   fprintf(out_fp,"\"xfer\": {\"avg\": %.6f, \"max\": {\"$numberLong\": \"%" PRIu64 "\"}}\n", (double) state.totals.t_xfer/days_in_month, max_xfer);
   fputs("},\n", out_fp);

   if(state.totals.t_visits) {
      fputs("\"visit\": {\n", out_fp);

      fprintf(out_fp,"\"hits\": {\"avg\": %.6f, \"max\": {\"$numberLong\": \"%" PRIu64 "\"}},\n", (double) state.totals.t_hit/state.totals.t_visits, state.totals.max_v_hits);
      fprintf(out_fp,"\"files\": {\"avg\": %.6f, \"max\": {\"$numberLong\": \"%" PRIu64 "\"}},\n", (double) state.totals.t_file/state.totals.t_visits, state.totals.max_v_files);
      fprintf(out_fp,"\"pages\": {\"avg\": %.6f, \"max\": {\"$numberLong\": \"%" PRIu64 "\"}},\n", (double) state.totals.t_page/state.totals.t_visits, state.totals.max_v_pages);
      fprintf(out_fp,"\"xfer\": {\"avg\": %.6f, \"max\": {\"$numberLong\": \"%" PRIu64 "\"}},\n", (double) state.totals.t_xfer/state.totals.t_visits, state.totals.max_v_xfer);
      fprintf(out_fp,"\"duration\": {\"avg\": %.6f, \"max\": %.6f},\n", state.totals.t_visit_avg/60., state.totals.t_visit_max/60.);
      if(state.totals.t_visits_conv)
         fprintf(out_fp,"\"duration_converted\": {\"avg\": %.6f, \"max\": %.6f}\n", state.totals.t_vconv_avg/60., state.totals.t_vconv_max/60.);

      fputs("}\n", out_fp);
   }

   fputs("},\n", out_fp);

   fputs("\"response\": {\n", out_fp);

   if(state.totals.m_hitptime) {
      fputs("\"timing\": {\n", out_fp);

      fprintf(out_fp,"\"hits\": {\"avg\": %.6f, \"max\": %.6f},\n", state.totals.a_hitptime, state.totals.m_hitptime);
      fprintf(out_fp,"\"files\": {\"avg\": %.6f, \"max\": %.6f},\n", state.totals.a_fileptime, state.totals.m_fileptime);
      fprintf(out_fp,"\"pages\": {\"avg\": %.6f, \"max\": %.6f}\n", state.totals.a_pageptime, state.totals.m_pageptime);

      fputs("},\n", out_fp);
   }

   fputs("\"codes\": [\n", out_fp);
   size_t count = 0;
   for(size_t i = 0; i < state.response.size(); i++) {
      if (state.response[i].count != 0) {
         if(count++)
            fputs(",\n", out_fp);
         fprintf(out_fp,"{\"code\": %u, \"description\": \"%s\", \"hits\": {\"$numberLong\": \"%" PRIu64 "\"}}", state.response[i].get_scode(), json_encode(config.lang.get_resp_code(state.response[i].get_scode()).desc), state.response[i].count);
      }
   }
   fputs("\n]\n", out_fp);
   fputs("}\n", out_fp);

   fputs("}\n", out_fp);

   fclose(out_fp);
}

void json_output_t::dump_daily(void)
{
   u_int wday;
   u_int i;
   const hist_month_t *hptr;
   FILE     *out_fp;
   string_t filename;
   size_t count = 0;

   filename.format("%s/daily_%04d%02d.json",
      (!config.dump_path.isempty())? config.dump_path.c_str() : ".", state.totals.cur_tstamp.year,state.totals.cur_tstamp.month);

   if((out_fp = open_out_file(filename)) == nullptr) {
      fprintf(stderr, "%s %s\n", config.lang.msg_no_open, filename.c_str());
      return;
   }

   if((hptr = state.history.find_month(state.totals.cur_tstamp.year, state.totals.cur_tstamp.month)) == nullptr)
      return;
      
   wday = tstamp_t::wday(state.totals.cur_tstamp.year, state.totals.cur_tstamp.month, 1);

   /* skip beginning blank days in a month */
   for(i = 0; i < hptr->lday; i++) {
      if (state.t_daily[i].tm_hits != 0)
         break;
   }

   if(i == hptr->lday)
      i=0;

   fputs("[\n", out_fp);

   for(; i < hptr->lday; i++) {
      if(count++)
         fputs(",\n", out_fp);

      fputs("{\n", out_fp);

      fprintf(out_fp, "\"_id\": %u,\n", state.totals.cur_tstamp.year * 10000 + state.totals.cur_tstamp.month * 100 + (i+1));

      fprintf(out_fp,"\"day\": %d,\n\"weekend\": %s,\n", i+1, ((wday + i) % 7 == 6 || (wday + i) % 7 == 0) ? "true" : "false");
      fprintf(out_fp,"\"hits\": {\n\"total\": {\"$numberLong\": \"%" PRIu64 "\"},\n\"percent\": %.6f,\n\"avg\": %.6f,\n\"max\": {\"$numberLong\": \"%" PRIu64 "\"}},\n", state.t_daily[i].tm_hits, PCENT(state.t_daily[i].tm_hits, state.totals.t_hit), state.t_daily[i].h_hits_avg, state.t_daily[i].h_hits_max);
      fprintf(out_fp,"\"files\": {\n\"total\": {\"$numberLong\": \"%" PRIu64 "\"},\n\"percent\": %.6f,\n\"avg\": %.6f,\n\"max\": {\"$numberLong\": \"%" PRIu64 "\"}},\n", state.t_daily[i].tm_files, PCENT(state.t_daily[i].tm_files, state.totals.t_file), state.t_daily[i].h_files_avg, state.t_daily[i].h_files_max);
      fprintf(out_fp,"\"pages\": {\n\"total\": {\"$numberLong\": \"%" PRIu64 "\"},\n\"percent\": %.6f,\n\"avg\": %.6f,\n\"max\": {\"$numberLong\": \"%" PRIu64 "\"}},\n", state.t_daily[i].tm_pages, PCENT(state.t_daily[i].tm_pages, state.totals.t_page), state.t_daily[i].h_pages_avg, state.t_daily[i].h_pages_max);
      fprintf(out_fp,"\"visits\": {\n\"total\": {\"$numberLong\": \"%" PRIu64 "\"},\n\"percent\": %.6f,\n\"avg\": %.6f,\n\"max\": {\"$numberLong\": \"%" PRIu64 "\"}},\n", state.t_daily[i].tm_visits, PCENT(state.t_daily[i].tm_visits, state.totals.t_visits), state.t_daily[i].h_visits_avg, state.t_daily[i].h_visits_max);
      fprintf(out_fp,"\"hosts\": {\n\"total\": {\"$numberLong\": \"%" PRIu64 "\"},\n\"percent\": %.6f,\n\"avg\": %.6f,\n\"max\": {\"$numberLong\": \"%" PRIu64 "\"}},\n", state.t_daily[i].tm_hosts, PCENT(state.t_daily[i].tm_hosts, state.totals.t_hosts), state.t_daily[i].h_hosts_avg, state.t_daily[i].h_hosts_max);
      fprintf(out_fp,"\"xfer\": {\n\"total\": {\"$numberLong\": \"%" PRIu64 "\"}, \"percent\": %.6f,\n\"avg\": %.6f,\n\"max\": {\"$numberLong\": \"%" PRIu64 "\"}}\n", state.t_daily[i].tm_xfer, PCENT(state.t_daily[i].tm_xfer, state.totals.t_xfer), state.t_daily[i].h_xfer_avg, state.t_daily[i].h_xfer_max);

      fputs("}", out_fp);
   }
   fputs("\n]\n", out_fp);

   fclose(out_fp);
}

void json_output_t::dump_hourly(void)
{
   u_int i, days_in_month;
   FILE     *out_fp;
   string_t filename;
   size_t count = 0;

   filename.format("%s/hourly_%04d%02d.json",
      (!config.dump_path.isempty())? config.dump_path.c_str() : ".", state.totals.cur_tstamp.year,state.totals.cur_tstamp.month);

   if((out_fp = open_out_file(filename)) == nullptr) {
      fprintf(stderr, "%s %s\n", config.lang.msg_no_open, filename.c_str());
      return;
   }

   days_in_month=(state.totals.l_day-state.totals.f_day)+1;

   fputs("[\n", out_fp);

   for(i = 0; i < 24; i++) {
      if(count++)
         fputs(",\n", out_fp);

      fputs("{\n", out_fp);

      fprintf(out_fp, "\"_id\": %u,\n", state.totals.cur_tstamp.year * 10000 + state.totals.cur_tstamp.month * 100 + i);

      fprintf(out_fp,"\"hour\": %d,\n", i);
      fprintf(out_fp, "\"hits\": {\n\"total\": {\"$numberLong\": \"%" PRIu64 "\"},\n\"percent\": %.6f,\n\"avg\": %.6f},\n", state.t_hourly[i].th_hits, PCENT(state.t_hourly[i].th_hits, state.totals.t_hit), (double) state.t_hourly[i].th_hits/days_in_month);
      fprintf(out_fp, "\"files\": {\"total\": {\"$numberLong\": \"%" PRIu64 "\"},\n\"percent\": %.6f,\n\"avg\":%.6f},\n", state.t_hourly[i].th_files, PCENT(state.t_hourly[i].th_files, state.totals.t_file), (double) state.t_hourly[i].th_files/days_in_month);
      fprintf(out_fp, "\"pages\": {\"total\": {\"$numberLong\": \"%" PRIu64 "\"},\n\"percent\": %.6f,\n\"avg\": %.6f},\n", state.t_hourly[i].th_pages, PCENT(state.t_hourly[i].th_pages, state.totals.t_page), (double) state.t_hourly[i].th_pages/days_in_month);
      fprintf(out_fp, "\"xfer\": {\"total\": {\"$numberLong\": \"%" PRIu64 "\"},\n\"percent\": %.6f,\n\"avg\": %.6f}\n", state.t_hourly[i].th_xfer, PCENT(state.t_hourly[i].th_xfer, state.totals.t_xfer), (double) state.t_hourly[i].th_xfer/days_in_month);
        
      fputs("}", out_fp);
   }
   fputs("\n]\n", out_fp);

   fclose(out_fp);
}

void json_output_t::dump_all_hosts()
{
   storable_t<hnode_t> hnode;
   FILE     *out_fp;
   string_t filename;

   filename.format("%s/host_%04d%02d.json",
      (!config.dump_path.isempty())? config.dump_path.c_str() : ".", state.totals.cur_tstamp.year,state.totals.cur_tstamp.month);

   if((out_fp = open_out_file(filename)) == nullptr) {
      fprintf(stderr, "%s %s\n", config.lang.msg_no_open, filename.c_str());
      return;
   }

   size_t count = 0;
   database_t::iterator<hnode_t> iter = state.database.begin_hosts(nullptr);

   fputs("[\n", out_fp);

   while (iter.next(hnode)) {
      const buffer_formatter_t::scope_t& fmt_scope = buffer_formatter.set_scope_mode(buffer_formatter_t::append);

      if(count++)
         fputs(",\n", out_fp);

      fprintf(out_fp,
         "{\n"
            "\"_id\": {\"$numberLong\": \"%" PRIu64 "\"},\n"
            "\"item_id\": {\"$numberLong\": \"%" PRIu64 "\"},\n"
            "\"year\": %u,\n"
            "\"month\": %u,\n"
            "\"group\": %s,\n"
            "\"hits\": {\"$numberLong\": \"%" PRIu64 "\"},\n"
            "\"files\": {\"$numberLong\": \"%" PRIu64 "\"},\n"
            "\"pages\": {\"$numberLong\": \"%" PRIu64 "\"},\n"
            "\"xfer\": {\"$numberLong\": \"%" PRIu64 "\"},\n"
            "\"visits\": {\"$numberLong\": \"%" PRIu64 "\"},\n"
            "\"visits_conv\": {\"$numberLong\": \"%" PRIu64 "\"},\n"
            "\"visit_avg\": %.6lf,\n"
            "\"visit_max\": {\"$numberLong\": \"%" PRIu64 "\"},\n"
            "\"max_v_hits\": {\"$numberLong\": \"%" PRIu64 "\"},\n"
            "\"max_v_files\": {\"$numberLong\": \"%" PRIu64 "\"},\n"
            "\"max_v_xfer\": {\"$numberLong\": \"%" PRIu64 "\"},\n"
            "\"ccode\": \"%s\",\n"
            "\"country\": \"%s\",\n"
            "\"city\": \"%s\",\n"
            "\"spammer\": %s\n"
            "\"robot\": %s\n"
            "\"latitude\": %.6lf,\n"
            "\"longitude\": %.6lf,\n"
            "\"geoname_id\": %" PRIu32 ",\n"
            "\"as_num\": %" PRIu32 ",\n"
            "\"as_org\": \"%s\",\n"
            "\"ipaddr\": \"%s\",\n"
            "\"hostname\": \"%s\"\n"
         "}",
         get_json_id(state.totals.cur_tstamp.year, state.totals.cur_tstamp.month, hnode.nodeid),
         hnode.nodeid,
         state.totals.cur_tstamp.year, state.totals.cur_tstamp.month,
         hnode.flag == OBJ_GRP ? "true" : "false",
         hnode.count, hnode.files, hnode.pages, hnode.xfer,
         hnode.visits, hnode.visits_conv, hnode.visit_avg, hnode.visit_max,
         hnode.max_v_hits, hnode.max_v_files, hnode.max_v_xfer,
         hnode.ccode,
         json_encode(state.cc_htab.get_ccnode(hnode.get_ccode()).cdesc.c_str()),
         json_encode(hnode.city.c_str()),
         hnode.spammer ? "true" : "false",
         hnode.robot ? "true" : "false", 
         hnode.latitude, hnode.longitude,
         hnode.geoname_id,
         hnode.as_num, json_encode(hnode.as_org.c_str()),
         hnode.string.c_str(), json_encode(hnode.hostname().c_str()));
   }
   iter.close();

   fputs("\n]\n", out_fp);

   fclose(out_fp);
}

void json_output_t::dump_all_urls()
{
   storable_t<unode_t> unode;
   FILE     *out_fp;
   string_t filename;

   filename.format("%s/url_%04d%02d.json",
      (!config.dump_path.isempty()) ? config.dump_path.c_str() : ".", state.totals.cur_tstamp.year,state.totals.cur_tstamp.month);

   if((out_fp = open_out_file(filename)) == nullptr) {
      fprintf(stderr, "%s %s\n", config.lang.msg_no_open, filename.c_str());
      return;
   }

   const string_t *title = nullptr;
   size_t count = 0;
   database_t::iterator<unode_t> iter = state.database.begin_urls(nullptr);

   fputs("[\n", out_fp);

   while (iter.next(unode)) {
      const buffer_formatter_t::scope_t& fmt_scope = buffer_formatter.set_scope_mode(buffer_formatter_t::append);

      if(count++)
         fputs(",\n", out_fp);

      if(config.page_titles.size()) {
         if(unode.flag == OBJ_REG)
            title = config.page_titles.isinglist(unode.string.c_str(), unode.string.length(), false);
         else if(title)
            title = nullptr;
      }

      fprintf(out_fp,
         "{\n"
            "\"_id\": {\"$numberLong\": \"%" PRIu64 "\"},\n"
            "\"item_id\": {\"$numberLong\": \"%" PRIu64 "\"},\n"
            "\"year\": %u,\n"
            "\"month\": %u,\n"
            "\"group\": %s,\n"
            "\"hits\": {\"$numberLong\": \"%" PRIu64 "\"},\n"
            "\"xfer\": {\"$numberLong\": \"%" PRIu64 "\"},\n"
            "\"avg_time\": %.6lf,\n"
            "\"max_time\": %.6lf,\n"
            "\"entry\": {\"$numberLong\": \"%" PRIu64 "\"},\n"
            "\"exit\": {\"$numberLong\": \"%" PRIu64 "\"},\n"
            "\"type\": %hu,\n"
            "\"title\": \"%s\",\n"
            "\"path\": \"%s\",\n"
            "\"query\": \"%s\"\n"
         "}",
         get_json_id(state.totals.cur_tstamp.year, state.totals.cur_tstamp.month, unode.nodeid),
         unode.nodeid,
         state.totals.cur_tstamp.year, state.totals.cur_tstamp.month,
         unode.flag == OBJ_GRP ? "true" : "false",
         unode.count, unode.xfer,
         unode.avgtime, unode.maxtime,
         unode.entry, unode.exit,
         unode.urltype,
         title ? json_encode(title->c_str()) : "",
         json_encode(unode.string.c_str(), unode.pathlen),
         json_encode(unode.get_query(), unode.get_query_len()));
   }
   iter.close();

   fputs("\n]\n", out_fp);

   fclose(out_fp);
}

void json_output_t::dump_all_refs()
{
   storable_t<rnode_t> rnode;
   FILE     *out_fp;
   string_t filename;

   filename.format("%s/ref_%04d%02d.json",
      (!config.dump_path.isempty()) ? config.dump_path.c_str() : ".", state.totals.cur_tstamp.year,state.totals.cur_tstamp.month);

   if((out_fp = open_out_file(filename)) == nullptr) {
      fprintf(stderr, "%s %s\n", config.lang.msg_no_open, filename.c_str());
      return;
   }

   size_t count = 0;
   database_t::iterator<rnode_t> iter = state.database.begin_referrers(nullptr);

   fputs("[\n", out_fp);

   while(iter.next(rnode)) {
      if(count++)
         fputs(",\n", out_fp);

      // $$$ document that query strings are dropped
      // $$$ document that URLs are normalized for display and may be invalid for copy-and-paste as URLs
      fprintf(out_fp,
         "{\n"
            "\"_id\": {\"$numberLong\": \"%" PRIu64 "\"},\n"
            "\"item_id\": {\"$numberLong\": \"%" PRIu64 "\"},\n"
            "\"year\": %u,\n"
            "\"month\": %u,\n"
            "\"group\": %s,\n"
            "\"hits\": {\"$numberLong\": \"%" PRIu64 "\"},\n"
            "\"visits\": {\"$numberLong\": \"%" PRIu64 "\"},\n"
            "\"referrer\": \"%s\"\n"
         "}",
         get_json_id(state.totals.cur_tstamp.year, state.totals.cur_tstamp.month, rnode.nodeid),
         rnode.nodeid,
         state.totals.cur_tstamp.year, state.totals.cur_tstamp.month,
         rnode.flag == OBJ_GRP ? "true" : "false",
         rnode.count, rnode.visits,
         json_encode(rnode.string.c_str())
         );
   }
   iter.close();

   fputs("\n]\n", out_fp);

   fclose(out_fp);

   return;
}

void json_output_t::dump_all_downloads(void)
{
   storable_t<dlnode_t> dlnode;
   const dlnode_t *nptr;
   FILE     *out_fp;
   string_t filename;

   filename.format("%s/dl_%04d%02d.json",
      (!config.dump_path.isempty()) ? config.dump_path.c_str() : ".", state.totals.cur_tstamp.year,state.totals.cur_tstamp.month);

   if((out_fp = open_out_file(filename)) == nullptr) {
      fprintf(stderr, "%s %s\n", config.lang.msg_no_open, filename.c_str());
      return;
   }

   size_t count = 0;
   database_t::iterator<dlnode_t> iter = state.database.begin_downloads(nullptr);

   fputs("[\n", out_fp);

   storable_t<hnode_t> hnode;
   while(iter.next<void *, storable_t<hnode_t>&>(dlnode, &state_t::unpack_dlnode_and_host_cb, const_cast<state_t*>(&state), hnode)) {
      const buffer_formatter_t::scope_t& fmt_scope = buffer_formatter.set_scope_mode(buffer_formatter_t::append);

      dlnode.set_host(&hnode);

      nptr = &dlnode;

      if(count++)
         fputs(",\n", out_fp);

      fprintf(out_fp,
         "{\n"
            "\"_id\": {\"$numberLong\": \"%" PRIu64 "\"},\n"
            "\"item_id\": {\"$numberLong\": \"%" PRIu64 "\"},\n"
            "\"year\": %u,\n"
            "\"month\": %u,\n"
            "\"sumhits\": {\"$numberLong\": \"%" PRIu64 "\"},\n"
            "\"sumxfer\": {\"$numberLong\": \"%" PRIu64 "\"},\n"
            "\"avgtime\": %.6f,\n"
            "\"sumtime\": %.6f,\n"
            "\"count\": {\"$numberLong\": \"%" PRIu64 "\"},\n"
            "\"name\": \"%s\",\n"
            "\"ccode\": \"%s\",\n"
            "\"country\": \"%s\",\n"
            "\"city\": \"%s\",\n"
            "\"latitude\": %.6lf,\n"
            "\"longitude\": %.6lf,\n"
            "\"geoname_id\": %" PRIu32 ",\n"
            "\"as_num\": %" PRIu32 ",\n"
            "\"as_org\": \"%s\",\n"
            "\"ipaddr\": \"%s\",\n"
            "\"hostname\": \"%s\"\n"
         "}",
         get_json_id(state.totals.cur_tstamp.year, state.totals.cur_tstamp.month, nptr->nodeid),
         nptr->nodeid,
         state.totals.cur_tstamp.year, state.totals.cur_tstamp.month,
         nptr->sumhits, nptr->sumxfer, 
         nptr->avgtime, nptr->sumtime, 
         nptr->count,
         json_encode(nptr->name.c_str()),
         hnode.ccode, 
         json_encode(state.cc_htab.get_ccnode(hnode.get_ccode()).cdesc.c_str()),
         json_encode(hnode.city.c_str()),
         hnode.latitude, hnode.longitude,
         hnode.geoname_id,
         hnode.as_num, json_encode(hnode.as_org.c_str()),
         hnode.string.c_str(), json_encode(hnode.hostname().c_str()));
   }
   iter.close();

   fputs("\n]\n", out_fp);

   fclose(out_fp);
   return;
}

void json_output_t::dump_all_errors(void)
{
   storable_t<rcnode_t> rcnode;
   FILE     *out_fp;
   string_t filename;

   filename.format("%s/err_%04d%02d.json",
      (!config.dump_path.isempty()) ? config.dump_path.c_str() : ".", state.totals.cur_tstamp.year,state.totals.cur_tstamp.month);

   if((out_fp = open_out_file(filename)) == nullptr) {
      fprintf(stderr, "%s %s\n", config.lang.msg_no_open, filename.c_str());
      return;
   }

   size_t count = 0;
   database_t::iterator<rcnode_t> iter = state.database.begin_errors(nullptr);

   fputs("[\n", out_fp);

   while(iter.next(rcnode)) {
      const buffer_formatter_t::scope_t& fmt_scope = buffer_formatter.set_scope_mode(buffer_formatter_t::append);

      if(count++)
         fputs(",\n", out_fp);

      fprintf(out_fp,
         "{\n"
            "\"_id\": {\"$numberLong\": \"%" PRIu64 "\"},\n"
            "\"item_id\": {\"$numberLong\": \"%" PRIu64 "\"},\n"
            "\"year\": %u,\n"
            "\"month\": %u,\n"
            "\"count\": {\"$numberLong\": \"%" PRIu64 "\"},\n"
            "\"respcode\": %hu,\n"
            "\"method\": \"%s\",\n"
            "\"url\": \"%s\"\n"
         "}",
         get_json_id(state.totals.cur_tstamp.year, state.totals.cur_tstamp.month, rcnode.nodeid),
         rcnode.nodeid,
         state.totals.cur_tstamp.year, state.totals.cur_tstamp.month,
         rcnode.count,
         rcnode.respcode,
         json_encode(rcnode.method.c_str()), // from a request - can be anything
         json_encode(rcnode.url.c_str())
      );
   }

   iter.close();

   fputs("\n]\n", out_fp);

   fclose(out_fp);
   return;
}

void json_output_t::dump_all_agents()
{
   storable_t<anode_t> anode;
   FILE     *out_fp;
   string_t filename;

   filename.format("%s/agent_%04d%02d.json",
      (!config.dump_path.isempty()) ? config.dump_path.c_str() : ".", state.totals.cur_tstamp.year,state.totals.cur_tstamp.month);

   if((out_fp = open_out_file(filename)) == nullptr) {
      fprintf(stderr, "%s %s\n", config.lang.msg_no_open, filename.c_str());
      return;
   }

   size_t count = 0;
   database_t::iterator<anode_t> iter = state.database.begin_agents(nullptr);

   fputs("[\n", out_fp);

   while(iter.next(anode)) {
      const buffer_formatter_t::scope_t& fmt_scope = buffer_formatter.set_scope_mode(buffer_formatter_t::append);

      if(count++)
         fputs(",\n", out_fp);

      fprintf(out_fp,
         "{\n"
            "\"_id\": {\"$numberLong\": \"%" PRIu64 "\"},\n"
            "\"item_id\": {\"$numberLong\": \"%" PRIu64 "\"},\n"
            "\"year\": %u,\n"
            "\"month\": %u,\n"
            "\"hits\": {\"$numberLong\": \"%" PRIu64 "\"},\n"
            "\"xfer\": {\"$numberLong\": \"%" PRIu64 "\"},\n"
            "\"robot\": %s,\n"
            "\"agent\": \"%s\"\n"
         "}",
         get_json_id(state.totals.cur_tstamp.year, state.totals.cur_tstamp.month, anode.nodeid),
         anode.nodeid,
         state.totals.cur_tstamp.year, state.totals.cur_tstamp.month,
         anode.count, anode.xfer,
         anode.robot ? "true" : "false",
         json_encode(anode.string.c_str())
      );
   }
   iter.close();

   fputs("\n]\n", out_fp);

   fclose(out_fp);
   return;
}

void json_output_t::dump_all_users()
{
   storable_t<inode_t> inode;
   FILE     *out_fp;
   string_t filename;

   filename.format("%s/user_%04d%02d.json",
      (!config.dump_path.isempty()) ? config.dump_path.c_str() : ".", state.totals.cur_tstamp.year,state.totals.cur_tstamp.month);

   if((out_fp = open_out_file(filename)) == nullptr) {
      fprintf(stderr, "%s %s\n", config.lang.msg_no_open, filename.c_str());
      return;
   }

   size_t count = 0;
   database_t::iterator<inode_t> iter = state.database.begin_users(nullptr);

   fputs("[\n", out_fp);

   while(iter.next(inode)) {
      const buffer_formatter_t::scope_t& fmt_scope = buffer_formatter.set_scope_mode(buffer_formatter_t::append);

      if(count++)
         fputs(",\n", out_fp);

      fprintf(out_fp,
         "{\n"
            "\"_id\": {\"$numberLong\": \"%" PRIu64 "\"},\n"
            "\"item_id\": {\"$numberLong\": \"%" PRIu64 "\"},\n"
            "\"year\": %u,\n"
            "\"month\": %u,\n"
            "\"hits\": {\"$numberLong\": \"%" PRIu64 "\"},\n"
            "\"files\": {\"$numberLong\": \"%" PRIu64 "\"},\n"
            "\"xfer\": {\"$numberLong\": \"%" PRIu64 "\"},\n"
            "\"visits\": {\"$numberLong\": \"%" PRIu64 "\"},\n"
            "\"avgtime\": %.6f,\n"
            "\"maxtime\": %.6f,\n"
            "\"user\": \"%s\"\n"
         "}",
         get_json_id(state.totals.cur_tstamp.year, state.totals.cur_tstamp.month, inode.nodeid),
         inode.nodeid,
         state.totals.cur_tstamp.year, state.totals.cur_tstamp.month,
         inode.count, inode.files, inode.xfer,
         inode.visit, 
         inode.avgtime, inode.maxtime,
         json_encode(inode.string.c_str())
      );
   }
   iter.close();

   fputs("\n]\n", out_fp);

   fclose(out_fp);
}

void json_output_t::dump_all_search()
{
   storable_t<snode_t> snode;
   FILE     *out_fp;
   string_t filename;

   filename.format("%s/search_%04d%02d.json",
      (!config.dump_path.isempty()) ? config.dump_path.c_str() : ".", state.totals.cur_tstamp.year,state.totals.cur_tstamp.month);

   if((out_fp = open_out_file(filename)) == nullptr) {
      fprintf(stderr, "%s %s\n", config.lang.msg_no_open, filename.c_str());
      return;
   }

   string_t type, term;
   size_t count = 0;
   database_t::iterator<snode_t> iter = state.database.begin_search(nullptr);

   fputs("[\n", out_fp);

   while(iter.next(snode)) {
      if(count++)
         fputs(",\n", out_fp);

      fprintf(out_fp,
         "{\n"
            "\"_id\": {\"$numberLong\": \"%" PRIu64 "\"},\n"
            "\"item_id\": {\"$numberLong\": \"%" PRIu64 "\"},\n"
            "\"year\": %u,\n"
            "\"month\": %u,\n"
            "\"hits\": {\"$numberLong\": \"%" PRIu64 "\"},\n"
            "\"visits\": {\"$numberLong\": \"%" PRIu64 "\"},\n",
         get_json_id(state.totals.cur_tstamp.year, state.totals.cur_tstamp.month, snode.nodeid),
         snode.nodeid,
         state.totals.cur_tstamp.year, state.totals.cur_tstamp.month,
         snode.count, snode.visits
      );
      
      //
      // Search terms may be in one of these forms, depending on the value
      // of termcnt:
      //
      //  * termcnt == 0: webalizer css
      //  * termcnt == 1: [0][13]webalizer css
      //  * termcnt == 1: [9]All Words[13]webalizer css
      //  * termcnt  > 1: [6]Phrase[13]webalizer css[9]File Type[3]any...
      // 
      // For the first two forms, generate a field `search` with the search
      // string. For the third form, add the search type, such as `All Words`
      // above. For the last form, generate an array of search term objects,
      // each with a search term and an optional term type.
      //
      const char *cp1 = snode.string;
      if(snode.termcnt == 0)
         fprintf(out_fp, "\"search\": \"%s\"\n", json_encode(cp1));
      else if(snode.termcnt == 1) {
         cstr2str(cstr2str(cp1, type), term);
         if(!type.isempty())
            fprintf(out_fp, "\"type\": \"%s\",\n", json_encode(type.c_str()));

         fprintf(out_fp, "\"search\": \"%s\"\n", json_encode(term.c_str()));
      }
      else {
         size_t termidx = 0;
         fprintf(out_fp, "\"termcnt\": %hu,\n", snode.termcnt);
         fputs("\"search\": [\n", out_fp);
         while((cp1 = cstr2str(cp1, type)) != nullptr && (cp1 = cstr2str(cp1, term)) != nullptr) {
            if(termidx++)
               fputs(",\n", out_fp);

            fputs("{\n", out_fp);

            if(!type.isempty())
               fprintf(out_fp, "\"type\": \"%s\",\n", json_encode(type.c_str()));

            fprintf(out_fp, "\"term\": \"%s\"\n", json_encode(term.c_str()));
            fputs("}\n", out_fp);
         }
         fputs("]\n", out_fp);
      } 

      fputs("}", out_fp);
   }
   iter.close();

   fputs("\n]\n", out_fp);

   fclose(out_fp);
}

void json_output_t::dump_all_cities()
{
   storable_t<ctnode_t> ctnode;
   FILE *out_fp;
   string_t filename;

   filename.format("%s/city_%04d%02d.json",
         !config.dump_path.isempty() ? config.dump_path.c_str() : config.out_dir.c_str(), 
         state.totals.cur_tstamp.year, state.totals.cur_tstamp.month);

   if((out_fp = open_out_file(filename)) == nullptr) {
      fprintf(stderr,"%s %s!\n",config.lang.msg_no_open, filename.c_str());
      return;
   }

   size_t count = 0;
   database_t::iterator<ctnode_t> iter = state.database.begin_cities(nullptr);

   fputs("[\n", out_fp);

   while(iter.next(ctnode)) {
      const buffer_formatter_t::scope_t& fmt_scope = buffer_formatter.set_scope_mode(buffer_formatter_t::append);

      if(count++)
         fputs(",\n", out_fp);

      fprintf(out_fp,
         "{\n"
            "\"_id\": {\"$numberLong\": \"%" PRIu64 "\"},\n"
            "\"item_id\": {\"$numberLong\": \"%" PRIu64 "\"},\n"
            "\"year\": %u,\n"
            "\"month\": %u,\n"
            "\"hits\": {\"$numberLong\": \"%" PRIu64 "\"},\n"
            "\"files\": {\"$numberLong\": \"%" PRIu64 "\"},\n"
            "\"pages\": {\"$numberLong\": \"%" PRIu64 "\"},\n"
            "\"xfer\": {\"$numberLong\": \"%" PRIu64 "\"},\n"
            "\"visits\": {\"$numberLong\": \"%" PRIu64 "\"},\n"
            "\"geoname_id\": %" PRIu32 ",\n"
            "\"ccode\": \"%s\",\n"
            "\"country\": \"%s\",\n"
            "\"city\": \"%s\"\n"
         "}",
         get_json_id(state.totals.cur_tstamp.year, state.totals.cur_tstamp.month, ctnode.compact_nodeid()),
         ctnode.nodeid,
         state.totals.cur_tstamp.year, state.totals.cur_tstamp.month,
         ctnode.hits, ctnode.files, ctnode.pages, 
         ctnode.xfer, ctnode.visits, 
         ctnode.geoname_id(), 
         ctnode.ccode.c_str(),
         json_encode(state.cc_htab.get_ccnode(ctnode.ccode).cdesc.c_str()),
         json_encode(ctnode.city.c_str())
      );
   }
   iter.close();

   fputs("\n]\n", out_fp);

   fclose(out_fp);
}

void json_output_t::dump_all_asn()
{
   storable_t<asnode_t> asnode;
   FILE *out_fp;
   string_t filename;

   filename.format("%s/asn_%04d%02d.json",
         !config.dump_path.isempty() ? config.dump_path.c_str() : config.out_dir.c_str(), 
         state.totals.cur_tstamp.year, state.totals.cur_tstamp.month);

   if((out_fp = open_out_file(filename)) == nullptr) {
      fprintf(stderr,"%s %s!\n",config.lang.msg_no_open, filename.c_str());
      return;
   }

   size_t count = 0;
   database_t::iterator<asnode_t> iter = state.database.begin_asn(nullptr);

   fputs("[\n", out_fp);

   while(iter.next(asnode)) {
      const buffer_formatter_t::scope_t& fmt_scope = buffer_formatter.set_scope_mode(buffer_formatter_t::append);

      if(count++)
         fputs(",\n", out_fp);

      fprintf(out_fp,
         "{\n"
            "\"_id\": {\"$numberLong\": \"%" PRIu64 "\"},\n"
            "\"item_id\": %" PRIu32 ",\n"
            "\"year\": %u,\n"
            "\"month\": %u,\n"
            "\"hits\": {\"$numberLong\": \"%" PRIu64 "\"},\n"
            "\"files\": {\"$numberLong\": \"%" PRIu64 "\"},\n"
            "\"pages\": {\"$numberLong\": \"%" PRIu64 "\"},\n"
            "\"xfer\": {\"$numberLong\": \"%" PRIu64 "\"},\n"
            "\"visits\": {\"$numberLong\": \"%" PRIu64 "\"},\n"
            "\"as_org\": \"%s\"\n"
         "}",
         get_json_id(state.totals.cur_tstamp.year, state.totals.cur_tstamp.month, asnode.nodeid),
         asnode.nodeid,
         state.totals.cur_tstamp.year, state.totals.cur_tstamp.month,
         asnode.hits, asnode.files, asnode.pages, 
         asnode.xfer, asnode.visits, 
         json_encode(asnode.as_org.c_str())
      );
   }
   iter.close();

   fputs("\n]\n", out_fp);

   fclose(out_fp);
}

void json_output_t::dump_all_countries()
{
   storable_t<ccnode_t> ccnode;
   FILE *out_fp;
   string_t filename;

   filename.format("%s/country_%04d%02d.json",
         !config.dump_path.isempty() ? config.dump_path.c_str() : config.out_dir.c_str(), 
         state.totals.cur_tstamp.year, state.totals.cur_tstamp.month);

   if((out_fp = open_out_file(filename)) == nullptr) {
      fprintf(stderr,"%s %s!\n",config.lang.msg_no_open, filename.c_str());
      return;
   }

   size_t count = 0;
   database_t::iterator<ccnode_t> iter = state.database.begin_countries(nullptr);

   fputs("[\n", out_fp);

   while(iter.next(ccnode)) {
      const buffer_formatter_t::scope_t& fmt_scope = buffer_formatter.set_scope_mode(buffer_formatter_t::append);

      if(count++)
         fputs(",\n", out_fp);

      fprintf(out_fp,
         "{\n"
            "\"_id\": {\"$numberLong\": \"%" PRIu64 "\"},\n"
            "\"item_id\": {\"$numberLong\": \"%" PRIu64 "\"},\n"
            "\"year\": %u,\n"
            "\"month\": %u,\n"
            "\"hits\": {\"$numberLong\": \"%" PRIu64 "\"},\n"
            "\"files\": {\"$numberLong\": \"%" PRIu64 "\"},\n"
            "\"pages\": {\"$numberLong\": \"%" PRIu64 "\"},\n"
            "\"xfer\": {\"$numberLong\": \"%" PRIu64 "\"},\n"
            "\"visits\": {\"$numberLong\": \"%" PRIu64 "\"},\n"
            "\"ccode\": \"%s\",\n"
            "\"country\": \"%s\"\n"
         "}",
         get_json_id(state.totals.cur_tstamp.year, state.totals.cur_tstamp.month, ccnode.nodeid),
         ccnode.nodeid,
         state.totals.cur_tstamp.year, state.totals.cur_tstamp.month,
         ccnode.count, ccnode.files, ccnode.pages, 
         ccnode.xfer, ccnode.visits, 
         ccnode.ccode.c_str(), json_encode(state.cc_htab.get_ccnode(ccnode.ccode).cdesc.c_str())
      );
   }
   iter.close();

   fputs("\n]\n", out_fp);

   fclose(out_fp);
}

#include "formatter_tmpl.cpp"
#include "database_tmpl.cpp"
