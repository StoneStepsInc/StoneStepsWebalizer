/*
    webalizer - a web server log analysis program

    Copyright (c) 2004-2014, Stone Steps Inc. (www.stonesteps.ca)

    See COPYING and Copyright files for additional licensing and copyright information

    logrec.h
*/

#ifndef __LOGREC_H
#define __LOGREC_H

#include "tstring.h"
#include "tstamp.h"

#ifdef _WIN32
#include <winsock2.h>
#else
#include <netinet/in.h>                // needed for in_addr structure definition
#endif

#define MAXURL   4096                  // Max HTTP request/URL field size
#define MAXMETHOD 16                   // HTTP method

/* Log types */
enum log_type_t {
   LOG_CLF     = 0,                 // CLF/combined log type
   LOG_FTP     = 1,                 // wu-ftpd xferlog type
   LOG_SQUID   = 2,                 // squid proxy log
   LOG_IIS     = 3,                 // IIS (W3C, with some deviations)
   LOG_APACHE  = 4,                 // Apache
   LOG_W3C     = 5                  // W3C
};

//
// log record structure
//
struct  log_struct  {
      string_t   hostname;             // hostname
      string_t   method;               // HTTP method
      string_t   url;                  // raw request field
      string_t   refer;                // referrer
      string_t   agent;                // user agent (browser)
      string_t   srchargs;             // search string
      string_t   ident;                // ident string (user)
      string_t   xsrchstr;             // referrer search string
      uint64_t     xfer_size;            // xfer size in bytes
      uint64_t     proc_time;            // URL processing time (ms)
      tstamp_t   tstamp;               // time stamp
      u_short    port;					   // HTTP port
      u_short    resp_code;            // HTTP response code

   public:
      log_struct(void);

      void reset(void);

      void normalize(u_int log_type);
};

#endif // __LOGREC_H
