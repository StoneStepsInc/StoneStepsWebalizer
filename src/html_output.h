/*
   webalizer - a web server log analysis program

   Copyright (c) 2004-1013, Stone Steps Inc. (www.stonesteps.ca)
   Copyright (C) 1997-2001  Bradford L. Barrett (brad@mrunix.net)

   See COPYING and Copyright files for additional licensing and copyright information 

   html_output.h
*/
#ifndef __HTML_OUTPUT_H
#define __HTML_OUTPUT_H

#include "output.h"
#include "graphs.h"
#include "encoder.h"

//
//
//
class config_t;
class state_t;
class history_t;
class database_t;

//
//
//
class html_output_t : public output_t {
   public:
      enum page_type_t {page_index, page_usage, page_all_items};

   private:
      typedef buffer_encoder_t< ::html_encode> html_encoder_t;

   private:
      FILE *out_fp;

      graph_t graph;

      html_encoder_t html_encoder;          // HTML encoder (may be used for copying in local scopes)
      html_encoder_t& html_encode;          // local scopes may define a new instance of html_encode

   private:
      void write_html_head(const char *period, FILE *out_fp, page_type_t page_type);
      void write_html_tail(FILE *out_fp);

      void write_highcharts_head(FILE *out_fp);

      void write_url_report(void);
      void write_download_report(void);
      void write_error_report(void);
      void write_host_report(void);
      void write_referrer_report(void);
      void write_search_report(void);
      void write_user_report(void);
      void write_user_agent_report(void);
      void write_country_report(void);

      void month_links(void);
      void month_total_table(void);
      void daily_total_table(void);
      void hourly_total_table(void);

      void top_hosts_table(int flag);
      void top_urls_table(int flag);
      void top_entry_table(int flag);
      void top_refs_table(void);
      void top_dl_table(void);
      void top_err_table(void);
      void top_agents_table(void);
      void top_search_table(void);
      void top_users_table(void);
      void top_ctry_table(void);

      int all_errors_page(void);
      int all_refs_page(void);
      int all_downloads_page(void);
      int all_hosts_page(void);
      int all_urls_page(void);
      int all_agents_page(void);
      int all_search_page(void);
      int all_users_page(void);

   public:
      html_output_t(const config_t& config, const state_t& state);

      ~html_output_t(void);

      virtual const char *get_output_type(void) const {return "HTML";}
      virtual bool is_main_index(void) const {return true;}

      virtual bool init_output_engine(void);
      virtual void cleanup_output_engine(void);

      virtual int write_main_index();
      virtual int write_monthly_report(void);

      virtual bool graph_support(void) const {return true;}
};

#endif // __HTML_OUTPUT_H
