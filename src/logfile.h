/*
   webalizer - a web server log analysis program

   Copyright (c) 2004-2017, Stone Steps Inc. (www.stonesteps.ca)

   See COPYING and Copyright files for additional licensing and copyright information 
   
   logfile.h
*/
#ifndef _LOGFILE_H
#define _LOGFILE_H

#include <zlib.h>
#include <cstdio>

#include "tstring.h"
#include "types.h"

class logfile_t {
   private:
      string_t    log_fname;
      
      bool        gz_log;                  // flag for zipped log
      
      FILE        *log_fp;                 // regular logfile pointer
      gzFile      gzlog_fp;                // gzip logfile pointer
      
      long        reopen_offset;
   
   public:
      logfile_t(const string_t& fname);
      
      ~logfile_t(void);
      
      void set_fname(const string_t& fname) {log_fname = fname;}
      
      const string_t& get_fname(void) const {return log_fname;}
      
      int open(void);
      
      int close(void);
      
      long set_reopen_offset(void);
      
      void clear_reopen_offset(void) {reopen_offset = 0;}
      
      bool is_gzip(void) const {return gz_log;}
      
      int get_line(char *buffer, u_int bufsize, int *errnum = NULL) const;
      
      bool is_readable(void) const;
      
      bool is_open(void) const {return gz_log && gzlog_fp || log_fp;}
};

#endif // _LOGFILE_H
