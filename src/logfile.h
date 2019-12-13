/*
   webalizer - a web server log analysis program

   Copyright (c) 2004-2019, Stone Steps Inc. (www.stonesteps.ca)

   See COPYING and Copyright files for additional licensing and copyright information 
   
   logfile.h
*/
#ifndef LOGFILE_H
#define LOGFILE_H

#include <zlib.h>
#include <cstdio>

#include "tstring.h"
#include "types.h"

///
/// @brief  A class that opens and reads a log file line by line
///
class logfile_t {
   private:
      string_t    log_fname;              ///< A log file name and path (relative or absolute).
      
      bool        gz_log;                 ///< Indicates whether the log file is compressed or not.
      
      FILE        *log_fp;                ///> A handle to an opened non-compressed log file.
      gzFile      gzlog_fp;               ///< A handle to a gzip-compressed log file.
      
      long        reopen_offset;          ///< A file offset to move the file pointer to after the 
                                          ///< file is opened.
                                             
      u_int       id;                     ///< A file identifier used for reporting purposes.
   
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
      
      int get_line(char *buffer, u_int bufsize, int *errnum = nullptr) const;
      
      bool is_readable(void) const;
      
      bool is_open(void) const {return gz_log && gzlog_fp || log_fp;}

      void set_id(u_int fileid) {id = fileid;}

      u_int get_id(void) const {return id;}
};

#endif // LOGFILE_H
