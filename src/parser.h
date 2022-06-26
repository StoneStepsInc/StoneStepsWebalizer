/*
   webalizer - a web server log analysis program

   Copyright (c) 2004-2022, Stone Steps Inc. (www.stonesteps.ca)
   Copyright (C) 1997-2001  Bradford L. Barrett (brad@mrunix.net)

   See COPYING and Copyright files for additional licensing and copyright information 

   parser.h
*/

#ifndef PARSER_H
#define PARSER_H

#include <cstdlib>

#include "config.h"

#include <vector>

/* Parse codes */
#define PARSE_CODE_ERROR      0
#define PARSE_CODE_OK         1
#define PARSE_CODE_IGNORE     2

//
//
//
struct log_struct;
struct field_desc;

///
/// @brief  A log file parser class
///
class parser_t {
   private:
      enum TLogFieldId {
         eISOLocalTime,             // local time in the ISO-8601 format
         eClfTime,
         eDate,
         eTime,
         eClientIpAddress,
         eUserName,
         eWebsiteName,
         eWebsiteIpAddress,
         eWebsitePort,
         eHttpMethod,
         eUriStem,                  // a part of the URI before the query
         eUriQuery,
         eUri,                      // a complete URI, including the query
         eHttpStatus,
         eBytesReceived,
         eBytesSent,
         eBytesTotal,            // total bytes transferred
         eTimeTaken,               // milliseconds (IIS) or seconds (W3C)
         eProcTimeS,               // seconds
         eProcTimeMcS,            // microseconds
         eUserAgent,
         eReferrer,
         eCookie,
         eHttpRequestLine,          // GET /a?b=c HTTP/1.1
         eRemoteLoginName,         // remote logname (from identd, if supplied). 
         eUnknown = -1            // must be last
      };

      ///
      /// @brief  A log record field value descriptor.
      /// 
      /// The meaning of the field is defined positionally by the value
      /// of the `log_rec_fields` element.
      /// 
      struct field_desc {
         const char  *field;     // a pointer to a log record field
         size_t      length;     // field length

         public:
            field_desc(void) :
                  field(nullptr),
                  length(0)
            {
            }
      };

      static constexpr u_int SQUID_FIELD_COUNT = 10;

   private:
      const config_t& config;

      std::vector<TLogFieldId> log_rec_fields;

      field_desc *fields;

      tstamp_t iis_tstamp;

      static const char *log_month[12];

   private:
      const char *parse_http_req_line(const char *cp1, log_struct& log_rec);

      static char *proc_bsesc_seq(char *cp1);

      bool parse_clf_tstamp(const char *dt, tstamp_t& ts);

      bool fmt_logrec(char *buffer, bool noparen, bool noquotes, bool bsesc, size_t fieldcnt);

      int parse_record_clf(char *buffer, size_t reclen, log_struct& log_rec);

      static std::vector<TLogFieldId> parse_apache_log_format(const char *format);
      int parse_record_apache(char *buffer, size_t reclen, log_struct& log_rec);

      int parse_w3c_log_directive(const char *buffer);
      int parse_record_w3c(char *buffer, size_t reclen, log_struct& log_rec, bool iis);

      int parse_record_squid(char *buffer, size_t reclen, log_struct& log_rec);

      static std::vector<TLogFieldId> parse_nginx_log_format(const char *format);

      int parse_record_nginx(char *buffer, size_t reclen, log_struct& log_rec);

   public:
      parser_t(const config_t& _config);

      ~parser_t(void);

      bool init_parser(int logtype);

      void cleanup_parser(void);

      int parse_record(char *buffer, size_t reclen, log_struct& log_rec);
};

#endif  // PARSER_H
