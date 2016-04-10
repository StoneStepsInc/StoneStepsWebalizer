/*
   webalizer - a web server log analysis program

   Copyright (c) 2004-2016, Stone Steps Inc. (www.stonesteps.ca)

   See COPYING and Copyright files for additional licensing and copyright information 
   
   logfile.cpp
*/
#include "pch.h"
#include "logfile.h"
#include "util.h"
#include <errno.h>

#ifdef _WIN32
#include <io.h>
#endif

logfile_t::logfile_t(const string_t& fname) : log_fname(fname)
{
   log_fp = NULL;
   gzlog_fp = NULL;
   
   reopen_offset = 0;

   gz_log = log_fname.length() > 3 && !strcasecmp((log_fname.c_str()+log_fname.length()-3), ".gz");
}

logfile_t::~logfile_t(void)
{
}

int logfile_t::open(void)
{
   if(log_fname.isempty()) {
      log_fp = stdin;
      return 0;
   }

   if(gz_log) {
      if(!gzlog_fp && (gzlog_fp = gzopen(log_fname,"rb")) == Z_NULL)
         return errno;
   }
   else {
      if(!log_fp && (log_fp = fopen(log_fname,"r")) == NULL)
         return errno;
   }
   
   // check if we need to return to the previous read position
   if(reopen_offset > 0) {
      if(gz_log) {
         if(gzseek(gzlog_fp, reopen_offset, SEEK_SET) == -1L)
            return errno;
      }
      else {
         if(fseek(log_fp, reopen_offset, SEEK_SET) == -1)
            return errno;
      }
   }
   
   return 0;
}

int logfile_t::close(void)
{
   int errnum = 0;
   
   // nothing to close
   if(log_fp == stdin)
      return 0;
   
   if(gz_log && gzlog_fp) {
      errnum = gzclose(gzlog_fp);
      gzlog_fp = NULL;
   }
   else if(log_fp) { 
      errnum = fclose(log_fp);
      log_fp = NULL;
   }
      
   return errnum;
}

long logfile_t::set_reopen_offset(void)
{
   if(gz_log && gzlog_fp)
      reopen_offset = gztell(gzlog_fp);
   else if(log_fp) 
      reopen_offset = ftell(log_fp);
   
   return reopen_offset;
}

int logfile_t::get_line(char *buffer, u_int bufsize, int *errnum) const
{
   if(buffer)
      *buffer = 0;

   if(gz_log) {
      if(gzlog_fp && gzgets(gzlog_fp, buffer, bufsize) == Z_NULL) {
         if(errnum)
            *errnum = errno;  // $$$ zlib error is lost !!!
         return gzeof(gzlog_fp) ? 0 : -1;
      }
   }
   else {
      if(log_fp && fgets(buffer, bufsize, log_fp) == NULL) {
         if(errnum)
            *errnum = errno;
         return feof(log_fp) ? 0 : -1;
      }
   }
   
   if(errnum)
      *errnum = 0;
   
   // the caller is expected to veryfy if the buffer is full
   return (int) strlen(buffer);
}

bool logfile_t::is_readable(void) const 
{
   // stdin (empty file name) is always readable
   if(log_fname.isempty())
      return true;
   
   // otherwise check if we can read the file   
   return !access(log_fname, R_OK);
}
