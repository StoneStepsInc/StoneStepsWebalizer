/*
   webalizer - a web server log analysis program

   Copyright (c) 2004-2015, Stone Steps Inc. (www.stonesteps.ca)
   Copyright (C) 1997-2001  Bradford L. Barrett (brad@mrunix.net)

   See COPYING and Copyright files for additional licensing and copyright information 

   dump_output.h
*/
#ifndef __DUMP_OUTPUT_H
#define __DUMP_OUTPUT_H

#include "output.h"

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
class dump_output_t : public output_t {
   private:
      FILE *out_fp;
      
   private:
      void dump_all_hosts(void);
      void dump_all_urls(void);
      void dump_all_refs(void);
      void dump_all_downloads(void);
      void dump_all_errors(void);
      void dump_all_agents(void);
      void dump_all_users(void);
      void dump_all_search(void);

   public:
      dump_output_t(const config_t& config, const state_t& state);

      ~dump_output_t(void);
      
      virtual const char *get_output_type(void) const {return "TSV";}
      virtual bool is_main_index(void) const {return false;}
      
      virtual bool init_output_engine(void);
      virtual void cleanup_output_engine(void);

      virtual int write_main_index();
      virtual int write_monthly_report(void);
};

#endif // __DUMP_OUTPUT_H
