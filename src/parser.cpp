/*
   webalizer - a web server log analysis program

   Copyright (c) 2004-2021, Stone Steps Inc. (www.stonesteps.ca)
   Copyright (C) 1997-2001  Bradford L. Barrett (brad@mrunix.net)

   See COPYING and Copyright files for additional licensing and copyright information 
   
   parser.cpp
*/
#include "pch.h"

#ifndef _WIN32
#include <unistd.h>                           /* normal stuff             */
#endif

#include "lang.h"
#include "parser.h"
#include "cp1252.h"
#include "unicode.h"
#include "util_url.h"
#include "util_time.h"

#include <vector>

#include <ctime>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cctype>

//
// Global data
//

/* month names used for parsing logfile (shouldn't be lang specific) */
const char *parser_t::log_month[12] = { "jan", "feb", "mar", "apr", "may", "jun", "jul", "aug", "sep", "oct", "nov", "dec"};

//
//
//

parser_t::parser_t(const config_t& _config) : config(_config)
{
   fields = nullptr;
}

parser_t::~parser_t(void)
{
}

///
/// @brief  Initializes the log file parser.
///
/// This method reports all errors to the stderr stream and returns `false` if the 
/// log file parser could not be initialized.
///
bool parser_t::init_parser(int logtype)
{
   switch(logtype) {
      case LOG_SQUID:
         fields = new field_desc[SQUID_FIELD_COUNT];
         break;

      case LOG_APACHE:
         log_rec_fields = parse_apache_log_format(config.apache_log_format);

         if(!log_rec_fields.empty())
            fields = new field_desc[log_rec_fields.size()];
         else {
            fprintf(stderr, "%s\n", config.lang.msg_afm_err);
            return false;
         }
         break;

      // these parse log record format directives in the log files
      case LOG_IIS:
      case LOG_W3C:
         break;

      // CLF has a fixed layout and the fields vector is not used
      case LOG_CLF:
         break;

      case LOG_NGINX:
         log_rec_fields = parse_nginx_log_format(config.nginx_log_format);

         if(!log_rec_fields.empty())
            fields = new field_desc[log_rec_fields.size()];
         else {
            fprintf(stderr, "%s\n", config.lang.msg_nfm_err);
            return false;
         }
         break;
   }

   return true;
}

void parser_t::cleanup_parser(void)
{
   if(fields)
      delete [] fields;
   fields = nullptr;
}

//
// Apache and Nginx (by default) encode quotes, back slashes, binary
// octets and some other bytes with a back slash. This function leaves
// characters that were not escaped unchanged and otherwise modifies
// characters in place and returns the pointer to the first character
// that should be included in the output. The caller must fill in the
// gap by moving characters left.
// 
//    \\     ->      \
//    \"     ->      "
// 
//    \xhh   ->      \%hh
//    ^--cp1          ^--returned
// 
inline char *parser_t::proc_bsesc_seq(char *cp1)
{
   switch(*(cp1+1)) {
      case '\\':
      case '"':
         return cp1;
      case 'x':
         *++cp1 = '%';
         return cp1;
   }
   return cp1;
}

//
// fmt_logrec
//
// This function works in two modes: with simple log file formats, such as 
// CLF or Squid, fmt_logrec looks for common delimiters and replaces them 
// with zeros; for those log file types that support format specifications,
// such as IIS or Apache, the function saves field information (start and 
// length) for each field, which improves overall processing speed.
//
// The function returns false if the number of fields found in the log line 
// does not match fieldcnt (which is derived from the log format specification).
//
// On Windows, fgets converts the sequence \r\n to a single \n character in 
// the text mode. When IIS logs are analized on Linux, fmt_logrec replaces 
// the \r\n sequence with two zeros. If \r is not followed by \n, it is 
// replaced by an underscore.
//
// buffer      - pointer to a log line
// noparen     - ignore parenthesis
// noquotes    - ignore quotes
// bsesc       - process backslash escape sequences
// fieldcnt    - number of fields; if zero, fields are not processed
//
bool parser_t::fmt_logrec(char *buffer, bool noparen, bool noquotes, bool bsesc, size_t fieldcnt)
{
   char *cp1 = buffer, *cp2;
   int q = 0, b = 0, p = 0;
   u_int slen = 0, index = 0;

   cp2 = cp1;
   while(*cp1 && (!fieldcnt || index < fieldcnt)) {
      /* break record up, terminate fields with '\0' */
      switch (*cp1) {
         case '\\': if(bsesc) cp1 = proc_bsesc_seq(cp1); break;
         case '\t':
         case ' ': if (b || q || p) break; *cp1 = 0; break;
         case '"': if(noquotes) break; q^=1;  break;
         case '[': if(noparen) break; if (q) break; b++; break;
         case ']': if(noparen) break; if (q) break; if(b > 0) b--; break;
         case '(': if(noparen) break; if (q) break; p++; break;
         case ')': if(noparen) break; if (q) break; if(p > 0) p--; break;
         case '\r': if(q) {*cp1 = '_'; break;} 
            *cp1++ = 0; if(*cp1 == '\n') *cp1-- = 0; else *--cp1 = '_'; break;
         case '\n': 
            if(q) *cp1 = '_'; else *cp1 = 0; break; 
      }
      
      if(fieldcnt) {
         if(*cp1) {
            if(slen == 0)
               fields[index].field = cp2;
            slen++;
         } else {
            if(slen) {
               fields[index++].length = slen; 
               slen = 0;
            } 
         }
      }
      
      // if processing escape sequences and found any
      if(bsesc && cp1 != cp2)
         *cp2++ = *cp1++;           // move the character
      else
         cp1++, cp2++;              // otherwise, just advance both pointers
   }

   //
   // The last field may have trailing whitespace, which may be a result
   // of having a few spaces in a log format line. We already have the
   // null terminator in place of the first space and just need to make
   // sure there are no other fields following the last one.
   //
   while(*cp1 == ' ' || *cp1 == '\t' || *cp1 == '\r' || *cp1 == '\n') cp1++;

   return (*cp1 == '\x0' && (!fieldcnt || index == fieldcnt));
}

//
//           1         2
// 0123456789012345678901234567
// [07/Dec/2004:21:30:21 -0500]
//
bool parser_t::parse_clf_tstamp(const char *dt, tstamp_t& ts)
{
   int offset; // UTC offset (+/-hhmm)
   int month;

   if(dt == nullptr || *dt == 0)
      return false;

   if(dt[0] != '[' || dt[27] != ']' || dt[28] != 0)
      return false;

   month = 0;

   for(int index = 0; index < 12; index++) {
      if(string_t::compare_ci(log_month[index], &dt[4], 3) == 0) { 
         month = index + 1; 
         break; 
      }
   }

   if(month == 0)
      return false;

   offset = atoi(&dt[22]);

   // set up the time stamp as local time
   ts.reset(atoi(&dt[8]), month, atoi(&dt[1]), atoi(&dt[13]), atoi(&dt[16]), atoi(&dt[19]), (offset/100) * 60 + offset % 100);

   if(ts.year < 1900 || ts.month > 12 || ts.hour > 23 || ts.min > 59 || ts.sec > 59)
      return false;
   
   return true;
}

const char *parser_t::parse_http_req_line(const char *cp1, log_struct& log_rec)
{
   const char *cptr;

   if(cp1 == nullptr)
      return nullptr;

   //
   // "GET / HTTP/1.0"
   //

   // skip the initial quote, if any
   if(*cp1 && *cp1 == '"') cp1++;

   // find the first space after the method
   cptr = cp1;
   while(*cp1 && *cp1 != ' ') 
      cp1++;
   
   // if there's no method, return
   if(cp1-cptr == 0)
      return nullptr;
   
   // copy the method   
   log_rec.method.assign(cptr, cp1-cptr);

   // check if there is no first space
   if(*cp1 != ' ')
      return nullptr;

   // skip spaces 
   while(*cp1 && *cp1 == ' ') cp1++;

   // copy the URL
   cptr = cp1;
   while(*cp1 && *cp1 != ' ') {
      if(*cp1 == '?')
         break;
      cp1++;
   }
   if(cp1 <= cptr)
      return nullptr;
   log_rec.url.assign(cptr, cp1-cptr);

   // check for query strings
   if(*cp1 == '?') {
      cp1++;
      cptr = cp1;
      while(*cp1 && *cp1 != ' ') cp1++;
      if(cp1 > cptr)
         log_rec.srchargs.assign(cptr, cp1-cptr);
   }

   // check if there is no second space
   if(*cp1 != ' ')
      return nullptr;

   // skip to the next field
   while(*cp1) cp1++;

   return cp1;
}
       
/*********************************************/
/* PARSE_RECORD - uhhh, you know...          */
/*********************************************/

int parser_t::parse_record(char *buffer, size_t reclen, log_struct& log_rec)
{
   int retval = PARSE_CODE_ERROR;

   /* clear out structure */
   log_rec.reset();

   /* call appropriate handler */
   switch (config.log_type) {
      default:
      case LOG_W3C:
         retval = parse_record_w3c(buffer, reclen, log_rec, false);
         break;
      case LOG_IIS:
         retval = parse_record_w3c(buffer, reclen, log_rec, true);
         break;
      case LOG_CLF:
         retval = parse_record_clf(buffer, reclen, log_rec);
         break;
      case LOG_APACHE:
         retval = parse_record_apache(buffer, reclen, log_rec);
         break;
      case LOG_SQUID: 
         retval = parse_record_squid(buffer, reclen, log_rec);
         break;
      case LOG_NGINX:
         retval = parse_record_nginx(buffer, reclen, log_rec);
         break;
   }

   // if the record is good, convert all domain names to lower case
   if(retval == PARSE_CODE_OK) {
      //
      // Normalize all strings that may have URL encoding
      //
      string_t::char_buffer_t strbuf;

      norm_url_str(log_rec.url, strbuf);
      norm_url_str(log_rec.refer, strbuf);
      norm_url_str(log_rec.agent, strbuf);
      norm_url_str(log_rec.srchargs, strbuf);
      norm_url_str(log_rec.ident, strbuf);
      norm_url_str(log_rec.xsrchstr, strbuf);

      // normalized URLs contain only valid UTF-8 characters and may be converted to lower case
      if(config.conv_url_lower_case) {
         log_rec.url.tolower();
         log_rec.srchargs.tolower();
      }

      if(config.log_type == LOG_SQUID) {
         // convert the scheme and the host name to lower case
         string_t::const_char_buffer_t host = get_url_host(log_rec.url.c_str(), log_rec.url.length());
         if(!host.isempty())
            log_rec.url.tolower(0, host - log_rec.url.c_str() + host.capacity());
      } else {
         // convert the scheme and the host name in the referrer to lower case
         string_t::const_char_buffer_t host = get_url_host(log_rec.refer.c_str(), log_rec.refer.length());
         if(!host.isempty())
            log_rec.refer.tolower(0, host - log_rec.refer.c_str() + host.capacity());
      }

      // convert possible host names and IPv6 addresses to lower case
      log_rec.hostname.tolower();
   }

   return retval;
}

/*********************************************/
/* parse_record_clf - web log handler        */
/*********************************************/

//
// %h %l %u %t \"%r\" %>s %b
// %h %l %u %t \"%r\" %>s %b \"%{Referer}i\" \"%{User-agent}i\"
//
int parser_t::parse_record_clf(char *buffer, size_t reclen, log_struct& log_rec)
{
   char *cp1, *cp2, *cpx, *eob;

   if(!buffer || !*buffer) {
      if (config.verbose>1) printf("%s\n",config.lang.msg_ign_nscp);
      return PARSE_CODE_ERROR;
   }
   
   //
   // Netscape log files had a format line, which could be parsed to figure out 
   // log record fields. It's pointless now, so just ignore this line and hope 
   // that the log file is formatted as CLF (used to be done in webalizer.cpp,
   // except that old code also checked that it was the first record)
   //
   if(!strncmp(buffer,"format=",7))
      return PARSE_CODE_IGNORE;

   eob = buffer + reclen;                      /* calculate end of buffer     */
   fmt_logrec(buffer, false, false, false, 0); /* separate fields with 0's    */

   //
   // IP address
   //
   cp1 = buffer; 
   cp1 += log_rec.hostname.assign(cp1).length();
   if(++cp1 >= eob) return PARSE_CODE_ERROR;

   //
   // skip next field (ident)
   //
   while(*cp1 && cp1 < eob) cp1++;
   if(++cp1 >= eob) return PARSE_CODE_ERROR;

   //
   // auth user field
   //
   cp2 = cp1;

   /* restore spaces that that may have been converted to zeros */
   while(*cp2 != '[' && cp2 < eob) {
      if(*cp2 == 0)
         *cp2++ = ' ';
      else
         cp2++;
   }
   if(cp2 == cp1 || cp2 >= eob) return PARSE_CODE_ERROR;

   cpx = cp2--;
   while(*cp2 == ' ') *cp2-- = 0;
   cp2++;
   while(*cp1 == ' ') cp1++;

   if(cp2 <= cp1) return PARSE_CODE_ERROR;

   // assign only if it's not an empty field
   if(cp2-cp1 && (*cp1 != '-' || *(cp1+1)))
      log_rec.ident.assign(cp1, cp2-cp1);

   cp1 = cpx;

   //
   // date/time string 
   //
   cp2 = cp1;
   while(*cp2 && cp2 < eob) cp2++;
   if(cp2 >= eob) return PARSE_CODE_ERROR;

   if(!parse_clf_tstamp(cp1, log_rec.tstamp))
      return PARSE_CODE_ERROR;

   cp1 += cp2-cp1;
   if(++cp1 >= eob) return PARSE_CODE_ERROR;

   //
   // HTTP request
   //
   if((cp1 = (char*) parse_http_req_line(cp1, log_rec)) == nullptr)
      return PARSE_CODE_ERROR;
   if(++cp1 >= eob) return PARSE_CODE_ERROR;

   /* response code */
   log_rec.resp_code = (u_short) atoi(cp1);
   while(*cp1 && cp1 < eob) cp1++;
   if(++cp1 >= eob) return PARSE_CODE_ERROR;

   /* xfer size */
   if(*cp1 < '0' || *cp1 > '9') 
      log_rec.xfer_size=0;
   else 
      log_rec.xfer_size = strtoul(cp1, nullptr, 10);
   while(*cp1 && cp1 < eob) cp1++;

   /* done with CLF record */
   if(cp1++ >= eob) return PARSE_CODE_OK;

   //
   // get the referrer, if present
   //
   cp2 = cp1;

   while(*cp2 && cp2 < eob && *cp2 != '?' && *cp2 != '\n') cp2++;
   if (cp2 >= eob) return PARSE_CODE_OK;
   if(*cp1 == '"') {cp1++; if(*cp2 != '?') cp2--;}

   // assign only if it's not an empty field
   if(cp2-cp1 && (*cp1 != '-' || *(cp1+1)))
      log_rec.refer.assign(cp1, cp2-cp1);

   if(*cp2 == '?') {
      cp1 = ++cp2;
      while(*cp2 && cp2 < eob && *cp2 != '\n') cp2++;
      if(*--cp2 != '"') cp2++;
      log_rec.xsrchstr.assign(cp1, cp2-cp1);
   }
   while(*cp2 && cp2 < eob) cp2++;
   if(++cp2 >= eob) return PARSE_CODE_OK;

   //
   // user agent, if present
   //
   cp1 = cp2;
   while(*cp2 && cp2 < eob) cp2++;
   if (cp2 >= eob) return PARSE_CODE_OK;
   if(*cp1 == '"') {cp1++; cp2--;}

   // assign only if it's not an empty field
   if(cp2-cp1 && (*cp1 != '-' || *(cp1+1)))
      log_rec.agent.assign(cp1, cp2-cp1);

   return PARSE_CODE_OK;     /* maybe a valid record, return with TRUE */
}

std::vector<parser_t::TLogFieldId> parser_t::parse_apache_log_format(const char *format)
{
   const char *cptr = format;
   const char *param;
   u_int paramlen;
   std::vector<TLogFieldId> log_rec_fields;

   if(format == nullptr || *format == 0)
      return log_rec_fields;

   while(*cptr && (*cptr == ' ' || *cptr == '\t')) cptr++;

   while(*cptr) {
      paramlen = 0;
      param = nullptr;

      while(*cptr && (*cptr != '%')) cptr++;

      if(*cptr == 0)
         break;

      //
      // Skip logging condition characters (comma-separated status codes, 
      // optionally prefixed by an exclamation point). For example:
      //
      //      %400,501{User-agent}i
      //      %!200,304,302{Referer}i
      //
      while(strchr("<>!,0123456789", *++cptr));

      //
      // Assign a pointer to the parameter inside parenthesis
      //
      if(*cptr == '{') {
         param = ++cptr;
         while(*cptr && (*cptr != '}')) {cptr++; paramlen++;}
         if(*cptr++ != '}')
            goto errexit;
      }

      switch(*cptr) {
         case 'a':
         case 'h':
            // h is a host!!!
            log_rec_fields.push_back(eClientIpAddress);
            break;

         case 'l':
            log_rec_fields.push_back(eRemoteLoginName);
            break;

         case 'u':
            log_rec_fields.push_back(eUserName);
            break;

         case 't':
            if(paramlen) {
               fprintf(stderr, "Error parsing Apache log format - \"%%{format}t\" is not supported\n");
               goto errexit;
            }
            log_rec_fields.push_back(eClfTime);
            break;

         case 'r':
            log_rec_fields.push_back(eHttpRequestLine);
            break;

         case 's':
            log_rec_fields.push_back(eHttpStatus);
            break;

         case 'b':
         case 'B':
         case 'O':
            log_rec_fields.push_back(eBytesSent);
            break;

         case 'T':
            log_rec_fields.push_back(eProcTimeS);
            break;

         case 'D':
            log_rec_fields.push_back(eProcTimeMcS);
            break;

         case 'A':
            log_rec_fields.push_back(eWebsiteIpAddress);
            break;

         case 'p':
            log_rec_fields.push_back(eWebsitePort);
            break;

         case 'm':
            log_rec_fields.push_back(eHttpMethod);
            break;

         case 'U':
            log_rec_fields.push_back(eUriStem);
            break;

         case 'q':
            log_rec_fields.push_back(eUriQuery);
            break;

         case 'I':
            log_rec_fields.push_back(eBytesReceived);
            break;

         case 'v':
         case 'V':
            log_rec_fields.push_back(eWebsiteName);
            break;

         case 'i':
            // %{NAME}i must have header name
            if(!param)
               log_rec_fields.push_back(eUnknown);
            else {
               if(string_t::compare_ci(param, "Referer", 7) == 0)
                  log_rec_fields.push_back(eReferrer);
               else if(string_t::compare_ci(param, "User-Agent", 10) == 0)
                  log_rec_fields.push_back(eUserAgent);
               else
                  log_rec_fields.push_back(eUnknown);
            }
            break;

         default:
            log_rec_fields.push_back(eUnknown);
            break;
      }
      cptr++;
   }

   return log_rec_fields;

errexit:
   log_rec_fields.clear();
   return log_rec_fields;
}

int parser_t::parse_record_apache(char *buffer, size_t reclen, log_struct& log_rec)
{
   size_t slen;
   size_t fldindex = 0, fieldcnt;
   const char *cp1, *cp2;

   if(buffer == nullptr || *buffer == 0)
      return PARSE_CODE_ERROR;

   fieldcnt = log_rec_fields.size();

   if(fields == nullptr || fieldcnt == 0)
      return PARSE_CODE_ERROR;

   if(!fmt_logrec(buffer, false, false, true, fieldcnt))
      return PARSE_CODE_ERROR;

   while(fldindex < fieldcnt) {
      slen = fields[fldindex].length;
      cp1 = fields[fldindex].field;

      if(!cp1 || !slen)
         return PARSE_CODE_ERROR;

      if(*cp1 == '"' && slen >= 2) {
         cp1++; slen -= 2;
      }
      switch (log_rec_fields[fldindex]) {
         case eClientIpAddress:
            log_rec.hostname.assign(cp1, slen);
            break;

         case eUserName:
            if(slen && (slen > 1 || *cp1 != '-'))
               log_rec.ident.assign(cp1, slen);
            break;

         case eClfTime:
            if(!parse_clf_tstamp(cp1, log_rec.tstamp))
               return PARSE_CODE_ERROR;

            break;

         case eHttpRequestLine:
            if((cp1 = parse_http_req_line(cp1, log_rec)) == nullptr)
               return PARSE_CODE_ERROR;

            break;

         case eHttpStatus:
            /* response code */
            log_rec.resp_code = (u_short) atoi(cp1);
            break;

         case eBytesReceived:
            if(config.upstream_traffic)
               log_rec.xfer_size += strtoul(cp1,nullptr,10);
            break;

         case eBytesSent:
            /* xfer size */
            log_rec.xfer_size += strtoul(cp1,nullptr,10);
            break;

         case eReferrer:
            if(slen && (slen > 1 || *cp1 != '-')) {
               if((cp2 = strchr(cp1, '?')) != nullptr) {
                  log_rec.refer.assign(cp1, cp2-cp1); cp2++;
                  log_rec.xsrchstr.assign(cp2, slen - (cp2-cp1));
               }
               else
                  log_rec.refer.assign(cp1, slen);
            }
            break;

         case eUserAgent:
            if(slen && (slen > 1 || *cp1 != '-'))
               log_rec.agent.assign(cp1, slen);
            break;

         case eUriStem:
            log_rec.url.assign(cp1, slen);

            break;

         case eUriQuery:
            if(*cp1 == '?') {
               cp1++; slen--;
            }

            if(slen && (slen > 1 || *cp1 != '-'))
               log_rec.srchargs.assign(cp1, slen);

            break;

         case eHttpMethod:
            log_rec.method.assign(cp1, slen);
            break;

         case eWebsitePort:
            log_rec.port = (u_short) atoi(cp1);
            break;

         case eProcTimeMcS:
            log_rec.proc_time = usec2msec(strtoul(cp1, nullptr, 10));
            break;

         case eProcTimeS:
            log_rec.proc_time = strtoul(cp1, nullptr, 10) * 1000;
            break;

         default:
            break;
      }

      fldindex++;
   }

   return PARSE_CODE_OK;     
}

//
// Example of an IIS log format header
//
// #Software: Microsoft Internet Information Services 5.0
// #Version: 1.0
// #Date: 2004-09-19 00:00:02
// #Fields: date time c-ip cs-username s-sitename s-port cs-method cs-uri-stem cs-uri-query sc-status sc-bytes cs-bytes time-taken 
//
int parser_t::parse_w3c_log_directive(const char *buffer)
{
   const char *cptr = buffer, *cp2 = nullptr;
   size_t slen, i;
   size_t bytes_i = SIZE_MAX, cs_bytes_i = SIZE_MAX, sc_bytes_i = SIZE_MAX;
   
   static struct {
      const char     *name;
      size_t          nlen;
      TLogFieldId    field_id;
   } iis_fields[] = {
      {"date",             4,    eDate},
      {"time",             4,    eTime},
      {"c-ip",             4,    eClientIpAddress},
      {"cs-username",      11,   eUserName},
      {"s-sitename",       10,   eWebsiteName},
      {"s-ip",             4,    eWebsiteIpAddress},
      {"s-port",           6,    eWebsitePort},
      {"cs-method",        9,    eHttpMethod},
      {"cs-uri-stem",      11,   eUriStem},
      {"cs-uri-query",     12,   eUriQuery},
      {"sc-status",        9,    eHttpStatus},
      {"sc-bytes",         8,    eBytesSent},
      {"cs-bytes",         8,    eBytesReceived},
      {"bytes",            5,    eBytesTotal},
      {"time-taken",       10,   eTimeTaken},
      {"cs(Cookie)",       10,   eCookie},
      {"cs(User-Agent)",   14,   eUserAgent},
      {"cs(Referer)",      11,   eReferrer}
   };
   
   // initialize to indicate that neither is found
   bytes_i = cs_bytes_i = sc_bytes_i = SIZE_MAX;

   if(buffer == nullptr || *buffer == 0)
      return PARSE_CODE_ERROR;

   if(*cptr++ != '#')
      return PARSE_CODE_ERROR;
   
   // if it's the date directive, store it in case the log doesn't have dates
   if(!string_t::compare_ci(cptr, "Date:", 5)) {
      cptr += 5;
      while(*cptr && (*cptr == ' ' || *cptr == '\t')) cptr++;
      
      if(!*cptr) 
         return PARSE_CODE_ERROR;
         
      iis_tstamp.parse(cptr);
      return PARSE_CODE_IGNORE;
   }

   if(string_t::compare_ci(cptr, "Fields:", 7))
      return PARSE_CODE_IGNORE;
   cptr += 7;

   if(fields)
      delete [] fields;
   fields = nullptr;

   log_rec_fields.clear();
   
   // skip whitespace
   while(*cptr && (*cptr == ' ' || *cptr == '\t')) cptr++;

   // and parse the remainder of the line
   while(*cptr && *cptr != '\r' && *cptr != '\n') {

      // find the end of the field name
      for(cp2 = cptr; *cp2 != ' ' && *cp2 && *cp2 != '\t' && *cp2 != '\r' && *cp2 != '\n'; cp2++);
      
      slen = cp2 - cptr;

      // find the field by name
      for(i = 0; i < sizeof(iis_fields)/sizeof(iis_fields[0]); i++) {
         if(slen == iis_fields[i].nlen && !string_t::compare_ci(cptr, iis_fields[i].name, slen)) {
            
            // if we saw any of the bytes fields, remember the index
            switch (iis_fields[i].field_id) {
               case eBytesReceived: cs_bytes_i = log_rec_fields.size(); break;
               case eBytesSent: sc_bytes_i = log_rec_fields.size(); break;
               case eBytesTotal: bytes_i = log_rec_fields.size(); break;
            }
            
            // store the field identifier
            log_rec_fields.push_back(iis_fields[i].field_id);            
            
            break;
         }
      }
      
      // if we didn't find one, push eUnknown so we can skip it
      if(i == sizeof(iis_fields)/sizeof(iis_fields[0]))
         log_rec_fields.push_back(eUnknown);

      // move onto the next field and skip the whitespace
      cptr = cp2;
      while(*cptr && (*cptr == ' ' || *cptr == '\t')) cptr++;
   }

   // allocate storage for the field descriptors
   fields = new field_desc[log_rec_fields.size()];
   
   //
   // Make sure we don't double-count transfer amounts. Based on how many 
   // transfer amount fields we saw, and whether upsetream traffic is
   // counted or not, ignore some of the fields we just identified. 
   //
   if(bytes_i != SIZE_MAX) {
      if(cs_bytes_i != SIZE_MAX && sc_bytes_i != SIZE_MAX) {
         if(config.upstream_traffic) {
            // if we want both amounts, ignore sent/received
            log_rec_fields[cs_bytes_i] = eUnknown;
            log_rec_fields[sc_bytes_i] = eUnknown;
         }
         else {
            // if we want just downstream traffic, ignore total/received
            log_rec_fields[bytes_i] = eUnknown;
            log_rec_fields[cs_bytes_i] = eUnknown;
         }
      }
      else if(sc_bytes_i != SIZE_MAX) {
         if(config.upstream_traffic)
            log_rec_fields[sc_bytes_i] = eUnknown;
         else
            log_rec_fields[bytes_i] = eUnknown;
      }
      else if(cs_bytes_i != SIZE_MAX) {
         //
         // It's odd to see just total and received amounts and while we could
         // figure out sent amounts from these two, it will make parsing more 
         // complex and slower for no good reason. Count total bytes in this
         // case, regardless of upstream_traffic. 
         //
         log_rec_fields[cs_bytes_i] = eUnknown;
      }
   }
   else if(cs_bytes_i != SIZE_MAX && sc_bytes_i != SIZE_MAX) {
      // ignore received amounts if upstream traffic is not tracked
      if(!config.upstream_traffic)
         log_rec_fields[cs_bytes_i] = eUnknown;
   }
   
   return PARSE_CODE_IGNORE;
}

//
// parse_record_iis
//
// Returns:
//
//      PARSE_CODE_OK      - success 
//      PARSE_CODE_ERROR   - failure 
//      PARSE_CODE_IGNORE - ignore log record (e.g. header line)
//
int parser_t::parse_record_w3c(char *buffer, size_t reclen, log_struct& log_rec, bool iis)
{
   size_t slen;
   size_t fldindex = 0, fieldcnt;
   const char *cp1, *cp2;
   bool tsdate = false, tstime = false;
   u_int year = 0, month = 0, day = 0, hour = 0, min = 0, sec = 0;

   if(buffer == nullptr || *buffer == 0)
      return PARSE_CODE_ERROR;

   //
   // process log file header fields
   //
   if(*buffer == '#')
      return parse_w3c_log_directive(buffer);

   fieldcnt = log_rec_fields.size();

   if(fields == nullptr || fieldcnt == 0)
      return PARSE_CODE_ERROR;

   if(!fmt_logrec(buffer, true, true, false, fieldcnt))
      return PARSE_CODE_ERROR;

   while(fldindex < fieldcnt) {
      cp1 = fields[fldindex].field;
      slen = fields[fldindex].length;

      if(!cp1 || !slen)
         return PARSE_CODE_ERROR;

      switch (log_rec_fields[fldindex]) {
         case eDate:
            // <date>  = 4<digit> "-" 2<digit> "-" 2<digit>
            year = (u_int) str2ul(cp1, &cp1);
            if(!cp1 || *cp1++ != '-') return PARSE_CODE_ERROR;
            month = (u_int) str2ul(cp1, &cp1);
            if(!cp1 || *cp1++ != '-') return PARSE_CODE_ERROR;
            day = (u_int) str2ul(cp1);
            tsdate = true;
            break;

         case eTime:
            // <time>  = 2<digit> ":" 2<digit> [":" 2<digit> ["." *<digit>]
            hour = (u_int) str2ul(cp1, &cp1);
            if(!cp1 || *cp1++ != ':') return PARSE_CODE_ERROR;
            min = (u_int) str2ul(cp1, &cp1);
            sec = (cp1 && *cp1 == ':') ? (u_int) str2ul(++cp1) : 0;
            tstime = true;
            break;

         case eClientIpAddress:
            log_rec.hostname.assign(cp1, slen);
            break;

         case eUserName:
            if(slen && (slen > 1 || *cp1 != '-'))
               log_rec.ident.assign(cp1, slen);
            break;

         case eWebsiteName:
         case eWebsiteIpAddress:
            break;

         case eHttpMethod:
            log_rec.method.assign(cp1, slen);
            break;

         case eWebsitePort:
            log_rec.port = (u_short) atoi(cp1);
            break;

         case eUriStem:
            log_rec.url.assign(cp1, slen);

            break;

         case eUriQuery:
            if(slen && (slen > 1 || *cp1 != '-'))
               log_rec.srchargs.assign(cp1, slen);

            break;

         case eHttpStatus:
            log_rec.resp_code = (u_short) atoi(cp1);
            break;

         // see comments in parse_w3c_log_directive
         case eBytesReceived:
         case eBytesSent:
         case eBytesTotal:
            log_rec.xfer_size += strtoul(cp1, nullptr, 10);
            break;

         case eTimeTaken:
            if(iis) 
               log_rec.proc_time = strtoul(cp1, nullptr, 10);                    // IIS logs time in milliseconds
            else
               log_rec.proc_time = (uint64_t) (strtod(cp1, nullptr) * 1000. + .5); // W3C requires time logged in seconds
            break;

         case eUserAgent:
            if(slen && (slen > 1 || *cp1 != '-')) {
               log_rec.agent.assign(cp1, slen);
               log_rec.agent.replace('+', ' ');
            }
            break;

         case eReferrer:
            if(slen && (slen > 1 || *cp1 != '-')) {
               if((cp2 = strchr(cp1, '?')) != nullptr) {
                  log_rec.refer.assign(cp1, cp2-cp1); cp2++;
                  log_rec.xsrchstr.assign(cp2, slen - (cp2-cp1));
               }
               else
                  log_rec.refer.assign(cp1, slen);
            }
            break;

         case eCookie:
            break;

         case eUnknown:
            break;
      }

      fldindex++;
   }

   // check if we got both, date and time
   if(tsdate && tstime)
      log_rec.tstamp.reset(year, month, day, hour, min, sec);
   else if(tstime && !tsdate && !iis_tstamp.null) {
      // if there's time, but no date, use the one from the header
      log_rec.tstamp.reset(iis_tstamp.year, iis_tstamp.month, iis_tstamp.day, hour, min, sec);
   }
   else
      return PARSE_CODE_ERROR;
      
   return fldindex == log_rec_fields.size() ? PARSE_CODE_OK : PARSE_CODE_ERROR;
}

/*********************************************/
/* PARSE_RECORD_SQUID - squid log handler    */
/*********************************************/

int parser_t::parse_record_squid(char *buffer, size_t reclen, log_struct& log_rec)
{
   u_int fldindex = 0;
   time_t i;
   const char *cp1, *cpx, *eob;

   if(fields == nullptr)
      return PARSE_CODE_ERROR;

   // calculate end of buffer
   eob = buffer + reclen;                         

   // separate fields with \0's
   if(!fmt_logrec(buffer, true, true, false, SQUID_FIELD_COUNT)) 
      return PARSE_CODE_ERROR;

   /* date/time */
   cp1 = fields[fldindex].field;
   i=atoi(cp1);      /* get timestamp */

   /* format date/time field */
   log_rec.tstamp.reset(i);

   /* processing time */
   cp1 = fields[++fldindex].field;
   log_rec.proc_time = strtoul(cp1, nullptr, 10);

   /* HOSTNAME */
   fldindex++;
   log_rec.hostname.assign(fields[fldindex].field, fields[fldindex].length);

   /* skip cache status */
   cp1 = fields[++fldindex].field;
   while (*cp1!=0 && cp1<eob && *cp1!='/') cp1++;
   cp1++;

   /* response code */
   log_rec.resp_code = (u_short) atoi(cp1);

   /* xfer size */
   cp1 = fields[++fldindex].field;
   if (*cp1<'0'||*cp1>'9') log_rec.xfer_size=0;
   else log_rec.xfer_size = strtoul(cp1,nullptr,10);

   // HTTP request type
   cp1 = fields[++fldindex].field;
   cpx = cp1;
   while(*cp1 && *cp1 != ' ' && *cp1 != '"')
      cp1++;
   log_rec.method.assign(cpx, cp1-cpx);

   // URL path
   cp1 = fields[++fldindex].field;
   while(*cp1 && *cp1 != ' ') {
      if(*cp1 == '?')
         break;
      cp1++;
   }
   log_rec.url.assign(fields[fldindex].field, cp1-fields[fldindex].field);

   // query strings (if any)
   if(*cp1 == '?') {
      cp1++;
      cpx = cp1;
      while(*cp1 && *cp1 != ' ')
         cp1++;
      if(cp1 > cpx)
         log_rec.srchargs.assign(cpx, cp1-cpx);
   }

   /* IDENT (authuser) field */
   fldindex++;

   // assign only if it's not an empty field
   if(fields[fldindex].length && (fields[fldindex].length > 1 || *fields[fldindex].field != '-'))
      log_rec.ident.assign(fields[fldindex].field, fields[fldindex].length);

   /* we have no interest in the remaining fields */
   return PARSE_CODE_OK;
}

std::vector<parser_t::TLogFieldId> parser_t::parse_nginx_log_format(const char *format)
{
   static const struct nginx_log_field_t {
      const string_t    nginx_var;
      TLogFieldId       field_type;
   } nginx_log_fields[] = {
      {string_t("time_iso8601"), eISOLocalTime},
      {string_t("time_local"), eClfTime},
      {string_t("remote_addr"), eClientIpAddress},
      {string_t("remote_user"), eUserName},
      {string_t("server_port"), eWebsitePort},
      {string_t("request_method"), eHttpMethod},
      {string_t("request_uri"), eUri},
      {string_t("uri"), eUriStem},
      {string_t("args"), eUriQuery},
      {string_t("query_string"), eUriQuery},
      {string_t("status"), eHttpStatus},
      {string_t("request_length"), eBytesReceived},
      {string_t("bytes_received"), eBytesReceived},
      {string_t("bytes_sent"), eBytesSent},
      {string_t("request_time"), eTimeTaken},
      {string_t("http_user_agent"), eUserAgent},
      {string_t("http_referer"), eReferrer},
      {string_t("request"), eHttpRequestLine},
   };

   std::vector<TLogFieldId> log_rec_fields;

   if(!format || !*format)
      return log_rec_fields;

   const char *ecp = format;

   //
   // Nginx log format allows any characters between fields and a
   // full-blown parser would track chunks of text between fields.
   // This simplified parser, however, will attempt to parse only
   // common characters and sequences and will fail with more
   // complex formats.
   //
   while(*ecp) {
      // skip whitespaces and characters fmt_logrec is aware of
      while(*ecp && (*ecp == ' ' || *ecp == '\t' || *ecp == '\"' || *ecp == '[')) ecp++;

      const char *scp = ecp;

      // advance to the end of the field, looking for whitespace or what fmt_logrec considers a delimeter
      while(*ecp && (*ecp != ' ' && *ecp != '\t' && *ecp != '\"' && *ecp != ']')) ecp++;

      if(*scp != '$')
         log_rec_fields.push_back(eUnknown);
      else {
         size_t i;

         scp++;

         // look for a field by name
         for(i = 0; i < sizeof(nginx_log_fields)/sizeof(nginx_log_fields[0]); i++) {
            if(nginx_log_fields[i].nginx_var.length() == ecp-scp && !nginx_log_fields[i].nginx_var.compare(scp, ecp-scp)) {
               log_rec_fields.push_back(nginx_log_fields[i].field_type);
               break;
            }
         }

         // if none was found, track it as unknown type
         if(i == sizeof(nginx_log_fields)/sizeof(nginx_log_fields[0]))
            log_rec_fields.push_back(eUnknown);
      }

      ecp++;
   }

   return log_rec_fields;
}

int parser_t::parse_record_nginx(char *buffer, size_t reclen, log_struct& log_rec)
{
   size_t slen;
   size_t fldindex = 0, fieldcnt;
   const char *cp1;
   u_int year = 0, month = 0, day = 0, hour = 0, min = 0, sec = 0, offset = 0;

   if(!buffer || !*buffer)
      return PARSE_CODE_ERROR;

   fieldcnt = log_rec_fields.size();

   if(!fields || !fieldcnt)
      return PARSE_CODE_ERROR;

   if(!fmt_logrec(buffer, false, false, true, fieldcnt))
      return PARSE_CODE_ERROR;

   while(fldindex < fieldcnt) {
      slen = fields[fldindex].length;
      cp1 = fields[fldindex].field;

      if(!cp1 || !slen)
         return PARSE_CODE_ERROR;

      // unwrap double quotes and adjust the length
      if(*cp1 == '"' && slen >= 2) {
         cp1++; slen -= 2;
      }

      switch (log_rec_fields[fldindex]) {
         case eISOLocalTime:
            //
            // Nginx always records ISO-8601 time stamps in local time, so
            // the time offset component is always present and we can avoid
            // checking for the UTC indicator at the end. These are the
            // expected formats (the top one is for systems running in UTC).
            // 
            // 2022-06-25T21:13:03+00:00
            // 2022-06-25T12:24:17-04:00
            //
            year = (u_int) str2ul(cp1, &cp1);
            if(!cp1 || *cp1++ != '-') return PARSE_CODE_ERROR;
            month = (u_int) str2ul(cp1, &cp1);
            if(!cp1 || *cp1++ != '-') return PARSE_CODE_ERROR;
            day = (u_int) str2ul(cp1, &cp1);

            if(!cp1 || *cp1++ != 'T') return PARSE_CODE_ERROR;

            hour = (u_int) str2ul(cp1, &cp1);
            if(!cp1 || *cp1++ != ':') return PARSE_CODE_ERROR;
            min = (u_int) str2ul(cp1, &cp1);
            if(!cp1 || *cp1++ != ':') return PARSE_CODE_ERROR;
            sec = (u_int) str2ul(cp1, &cp1);

            if(!cp1 || (*cp1++ != '-' && *cp1++ != '+')) return PARSE_CODE_ERROR;
            offset = (u_int) str2ul(cp1, &cp1) * 60;
            if(!cp1 || (*cp1++ != ':')) return PARSE_CODE_ERROR;
            offset += (u_int) str2ul(cp1, &cp1);

            log_rec.tstamp.reset(year, month, day, hour, min, sec, offset);
            break;
         case eClfTime:
            // [25/Jun/2022:12:24:17 -0400]
            if(!parse_clf_tstamp(cp1, log_rec.tstamp))
               return PARSE_CODE_ERROR;
            break;
         case eClientIpAddress:
            if(slen > 1 || *cp1 != '-')
               log_rec.hostname.assign(cp1, slen);
            break;
         case eUserName:
            if(slen > 1 || *cp1 != '-')
               log_rec.ident.assign(cp1, slen);
            break;
         case eWebsitePort:
            log_rec.port = (u_short) atoi(cp1);
            break;
         case eHttpMethod:
            log_rec.method.assign(cp1, slen);
            break;
         case eUri: {
            const char *cp2 = strchr(cp1, '?');
            if(cp2)
               log_rec.url.assign(cp1, cp2-cp1);
            else
               log_rec.url.assign(cp1, slen);
            }
            break;
         case eUriStem:
            log_rec.url.assign(cp1, slen);
            break;
         case eUriQuery:
            if(slen > 1 || *cp1 != '-')
               log_rec.srchargs.assign(cp1, slen);
            break;
         case eHttpStatus:
            log_rec.resp_code = (u_short) atoi(cp1);
            break;
         case eBytesReceived:
            if(config.upstream_traffic)
               log_rec.xfer_size += strtoul(cp1,nullptr,10);
            break;
         case eBytesSent:
            log_rec.xfer_size += strtoul(cp1, nullptr, 10);
            break;
         case eTimeTaken:
            log_rec.proc_time = (uint64_t) (strtod(cp1, nullptr) * 1000. + .5);
            break;
         case eUserAgent:
            if(slen > 1 || *cp1 != '-')
               log_rec.agent.assign(cp1, slen);
            break;
         case eReferrer:
            if(slen > 1 || *cp1 != '-')
               log_rec.refer.assign(cp1, slen);
            break;
         case eHttpRequestLine:
            if((cp1 = parse_http_req_line(cp1, log_rec)) == nullptr)
               return PARSE_CODE_ERROR;
            break;
      }

      fldindex++;
   }

   return fldindex == log_rec_fields.size() ? PARSE_CODE_OK : PARSE_CODE_ERROR;
}
