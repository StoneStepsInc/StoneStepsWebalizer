/*
   webalizer - a web server log analysis program

   Copyright (c) 2004-2021, Stone Steps Inc. (www.stonesteps.ca)
   Copyright (C) 1997-2001  Bradford L. Barrett (brad@mrunix.net)

   See COPYING and Copyright files for additional licensing and copyright information 

   json_output.h
*/
#ifndef JSON_OUTPUT_H
#define JSON_OUTPUT_H

#include "output.h"

#include "encoder.h"
#include "formatter.h"

//
//
//
class config_t;
class state_t;
class history_t;
class database_t;

///
/// @brief  JSON output
///
class json_output_t : public output_t {
   private:
      string_t::char_buffer_t buffer;

      buffer_formatter_t buffer_formatter;

   private:
      const char *json_encode(const char *str);
      const char *json_encode(const char *str, size_t len);

      static uint64_t get_json_id(u_int year, u_int day, uint64_t node_id);

      void dump_totals(void);
      void dump_daily(void);
      void dump_hourly(void);

      void dump_all_hosts(void);
      void dump_all_urls(void);
      void dump_all_refs(void);
      void dump_all_downloads(void);
      void dump_all_errors(void);
      void dump_all_agents(void);
      void dump_all_users(void);
      void dump_all_search(void);
      void dump_all_cities(void);
      void dump_all_countries(void);
      void dump_all_asn(void);

   public:
      json_output_t(const config_t& config, const state_t& state);

      ~json_output_t(void);
      
      virtual const char *get_output_type(void) const {return "JSON";}
      virtual bool is_main_index(void) const {return false;}
      
      virtual bool init_output_engine(void);
      virtual void cleanup_output_engine(void);

      virtual int write_main_index();
      virtual int write_monthly_report(void);
};

#endif // JSON_OUTPUT_H
