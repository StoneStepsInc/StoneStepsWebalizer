/*
   webalizer - a web server log analysis program

   Copyright (c) 2004-2013, Stone Steps Inc. (www.stonesteps.ca)

   See COPYING and Copyright files for additional licensing and copyright information 

   xml_output.h
*/
#ifndef __XML_OUTPUT_H
#define __XML_OUTPUT_H

#include "output.h"
#include "graphs.h"

//
//
//
class config_t;
class state_t;
class history_t;
class database_t;

// -----------------------------------------------------------------------
// dump_output_t
//
//
// -----------------------------------------------------------------------
class xml_output_t : public output_t {
   private:
      FILE *out_fp;
      
      char *encptr;

      graph_t graph;
      
   private:
      void write_monthly_totals(void);
      void write_daily_totals(void);
      void write_hourly_totals(void);
      void write_top_urls(bool s_xfer);
      void write_top_entry(bool entry);
      void write_top_downloads(void);
      void write_top_errors(void);
      void write_top_hosts(bool s_xfer);
      void write_top_referrers(void);
      void write_top_search_strings(void);
      void write_top_users(void);
      void write_top_agents(void);
      void write_top_countries(void);
   
      void write_xml_head(bool index);
      void write_xml_tail(void);

      char *xml_encode(const char *str, bool multiline = false);
      char *xml_encode_ex(const char *str, bool multiline = false);
      
      void xml_encode_reset(void);

   public:
      xml_output_t(const config_t& config, const state_t& state);

      ~xml_output_t(void);

      virtual const char *get_output_type(void) const {return "XML";}
      virtual bool is_main_index(void) const {return true;}

      virtual bool init_output_engine(void);
      virtual void cleanup_output_engine(void);

      virtual int write_main_index();
      virtual int write_monthly_report(void);
      
      virtual bool graph_support(void) const {return true;}
};

#endif // __XML_OUTPUT_H
