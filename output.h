/*
    webalizer - a web server log analysis program

    Copyright (c) 2004-2014, Stone Steps Inc. (www.stonesteps.ca)

    See COPYING and Copyright files for additional licensing and copyright information

    logrec.h
*/

#ifndef _OUTPUT_H
#define _OUTPUT_H

#include "hashtab_nodes.h"

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
class output_t {
   public:
      struct graphinfo_t {
         u_int       usage_width;
         u_int       usage_height;
         
         graphinfo_t(void) {usage_width = usage_height = 0;}
      };

   protected:
      const config_t&   config;
      const state_t&    state;

      char *buffer;                    // buffer for formatting, encoding, etc
      
      graphinfo_t *graphinfo;          // shared graph information 

   public:      
      bool makeimgs;                   // generate graph images (graphinfo owner if true)

   protected:
      FILE *open_out_file(const char *filename) const;
      
      static int qs_cc_cmpv(const void *, const void *);

   public:
      output_t(const config_t& config, const state_t& state);

      virtual ~output_t(void);
      
      virtual const char *get_output_type(void) const = 0;
      virtual bool is_main_index(void) const = 0;

      virtual bool init_output_engine(void) = 0;
      virtual void cleanup_output_engine(void) = 0;

      virtual int write_main_index() = 0;
      virtual int write_monthly_report(void) = 0;
      
      virtual bool graph_support(void) const {return false;}
      
      graphinfo_t *alloc_graphinfo(void);
      void set_graphinfo(graphinfo_t *ginfo) {graphinfo = ginfo;}
};

#endif  /* _OUTPUT_H */
