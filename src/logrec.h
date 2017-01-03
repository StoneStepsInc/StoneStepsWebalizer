/*
    webalizer - a web server log analysis program

    Copyright (c) 2004-2016, Stone Steps Inc. (www.stonesteps.ca)

    See COPYING and Copyright files for additional licensing and copyright information

    logrec.h
*/

#ifndef LOGREC_H
#define LOGREC_H

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
   LOG_SQUID   = 2,                 // squid proxy log
   LOG_IIS     = 3,                 // IIS (W3C, with some deviations)
   LOG_APACHE  = 4,                 // Apache
   LOG_W3C     = 5                  // W3C
};

//
// log record
//
// 1. URL strings can only contain ASCII characters. Non-ASCII characters must be 
// converted to UTF-8 before they are percent-encoded. However, some applications
// in the past used arbitrary character encodings before percent-encoding characters,
// which may lead to bad characters. For example, the JavaScript escape() function
// encodes the copyright symbol as %A9 (Latin1) instead of %C2%A9 (UTF-8). If a URL 
// containing such Latin1 character is found in the log, it cannot be interpreted 
// as a UTF-8 character sequence.
//
// 2. User agent string consists of product name and version characters, which are
// expected to contai only ASCII characters. Product comments in parentheses, on the 
// other hand, may contain practically any characters. However, RFC-2616 specifies 
// that if any of these characters are not Latin1 characers, they are expected to be 
// encoded as specified in RFC-2047 (i.e. =?charset?encoding?text?=). The latter is 
// not interpreted in any way in this code.
//
// 3. It is not clear what character set is used for user identification.
//
struct  log_struct  {
      string_t   hostname;             // client IP address (may be host name)
      string_t   method;               // HTTP method
      string_t   url;                  // requested URL path, without the query
      string_t   refer;                // referrer URL, without the query
      string_t   agent;                // user agent
      string_t   srchargs;             // request query
      string_t   ident;                // user identification
      string_t   xsrchstr;             // referrer query

      uint64_t   xfer_size;            // transfer size, in bytes
      uint64_t   proc_time;            // request processing time (ms)

      tstamp_t   tstamp;               // request time stamp

      u_short    port;                 // HTTP port
      u_short    resp_code;            // HTTP response code

   public:
      log_struct(void);

      void reset(void);
};

#endif // LOGREC_H
