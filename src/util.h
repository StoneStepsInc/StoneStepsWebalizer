/*
   webalizer - a web server log analysis program

   Copyright (c) 2004-2017, Stone Steps Inc. (www.stonesteps.ca)
   Copyright (C) 1997-2001  Bradford L. Barrett (brad@mrunix.net)

   See COPYING and Copyright files for additional licensing and copyright information 
   
   util.h
*/

#ifndef UTIL_H
#define UTIL_H

#include <stddef.h>
#include <time.h>
#include <stdlib.h>

#include "types.h"
#include "tstring.h"

#ifdef _WIN32
#define F_OK 00      // existance
#define W_OK 02      // write access
#define R_OK 04      // read access
#else
#include <unistd.h>
#endif

//
//
//
#ifndef MAX
#define MAX(a,b) ((a) > (b) ? (a) : (b))
#endif

#ifndef MIN
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#endif

#ifdef _WIN32
#define PATH_MAX _MAX_PATH
#else
#ifndef PATH_MAX
#define PATH_MAX 255
#endif
#endif

#ifdef _WIN32
#define PATH_SEP_CHAR    '\\'
#else
#define PATH_SEP_CHAR    '/'
#endif

//
//
//
/* Response code defines as per draft ietf HTTP/1.1 rev 6 */
#define RC_CONTINUE           100
#define RC_SWITCHPROTO        101
#define RC_OK                 200
#define RC_CREATED            201
#define RC_ACCEPTED           202
#define RC_NONAUTHINFO        203
#define RC_NOCONTENT          204
#define RC_RESETCONTENT       205
#define RC_PARTIALCONTENT     206
#define RC_MULTIPLECHOICES    300
#define RC_MOVEDPERM          301
#define RC_MOVEDTEMP          302
#define RC_SEEOTHER           303
#define RC_NOMOD              304
#define RC_USEPROXY           305
#define RC_MOVEDTEMPORARILY   307
#define RC_BAD                400
#define RC_UNAUTH             401
#define RC_PAYMENTREQ         402
#define RC_FORBIDDEN          403
#define RC_NOTFOUND           404
#define RC_METHODNOTALLOWED   405
#define RC_NOTACCEPTABLE      406
#define RC_PROXYAUTHREQ       407
#define RC_TIMEOUT            408
#define RC_CONFLICT           409
#define RC_GONE               410
#define RC_LENGTHREQ          411
#define RC_PREFAILED          412
#define RC_REQENTTOOLARGE     413
#define RC_REQURITOOLARGE     414
#define RC_UNSUPMEDIATYPE     415
#define RC_RNGNOTSATISFIABLE  416
#define RC_EXPECTATIONFAILED  417
#define RC_SERVERERR          500
#define RC_NOTIMPLEMENTED     501
#define RC_BADGATEWAY         502
#define RC_UNAVAIL            503
#define RC_GATEWAYTIMEOUT     504
#define RC_BADHTTPVER         505

/* Request types (bit values) */
#define URL_TYPE_UNKNOWN      0x00
#define URL_TYPE_HTTP         0x01
#define URL_TYPE_HTTPS        0x02
#define URL_TYPE_MIXED        0x03        /* HTTP and HTTPS (nothing else) */
#define URL_TYPE_OTHER        0x04

//
// Boyer-Moore-Horspool delta table (see strstr_ex for details)
//
class bmh_delta_table {
   private:
      size_t                *deltas;

      static const size_t   table_size;

   public:
      bmh_delta_table(void) {deltas = NULL;}

      bmh_delta_table(const string_t& str) : deltas(NULL) {reset(str);}

      ~bmh_delta_table(void) {if(deltas) delete [] deltas;}

      void reset(const string_t& str);

      bool isvalid(void) const {return !(deltas == NULL);}

      size_t operator [] (size_t chr) const {return deltas[chr];}
};

//
//
//
inline double AVG(double curavg, double value, uint64_t newcnt) {return curavg+(value-curavg)/(double)newcnt;}
inline double AVG(double curavg, uint64_t value, uint64_t newcnt) {return AVG(curavg, (double) value, newcnt);}

inline double AVG2(double a1, uint64_t n1, double a2, uint64_t n2) {return a1+(a2-a1)*((double)n2/((double)n1+(double)n2));}

inline double PCENT(double val, double max) {return max ? (val/max)*100.0 : 0.0;}
inline double PCENT(uint64_t val, uint64_t max) {return PCENT((double) val, (double) max);}

uint32_t ceilp2(uint32_t x);
uint32_t tzbits(uint32_t x);

uint64_t usec2msec(uint64_t usec);

bool is_http_redirect(size_t respcode);
bool is_http_error(size_t respcode);

const char *strstr_ex(const char *str1, const char *str2, size_t l1, size_t l2 = 0, const bmh_delta_table *delta = NULL);

size_t strncpy_ex(char *dest, size_t destsize, const char *src, size_t srclen);

int strncmp_ex(const char *str1, size_t slen1, const char *str2, size_t slen2);

string_t& url_decode(const string_t& str, string_t& out);

char from_hex(char);                           /* convert hex to dec  */
const char *from_hex(const char *cp1, char *cp2);

const char *cstr2str(const char *cp, string_t& str);

size_t ul2str(uint64_t value, char *str);
uint64_t str2ul(const char *str, const char **eptr = NULL, size_t len = ~0);
int64_t str2l(const char *str, const char **eptr = NULL, size_t len = ~0);

string_t cur_time(bool local_time);

uint64_t ctry_idx(const char *);
string_t idx_ctry(uint64_t idx);

const char *get_domain(const char *, size_t);       /* return domain name  */
string_t& get_url_host(const char *url, string_t& domain);
string_t::const_char_buffer_t get_url_host(const char *url, size_t slen);

bool is_abs_path(const char *path);
string_t get_cur_path(void);
string_t make_path(const char *base, const char *path);

bool is_ipv4_address(const char *str);
bool is_ipv6_address(const char *str);
bool is_ip_address(const char *str);

uint64_t elapsed(uint64_t stime, uint64_t etime);

size_t fmt_vprintf(string_t::char_buffer_t& buffer, const char *fmt, va_list args);
size_t fmt_hr_num(string_t::char_buffer_t& buffer, uint64_t num, const char *sep, const char *msg_unit_pfx[], const char *unit, bool decimal);

bool isinstrex(const char *str, const char *cp, size_t slen, size_t cplen, bool substr, const bmh_delta_table *deltas);

template <typename char_t>
const char_t *strptr(const char_t *str, const char_t *defstr = NULL);

// avoid any locale-specific checks and conversions
inline bool isalphaex(int ch) {return ch >= 'A' && ch <= 'Z' || ch >= 'a' && ch <= 'z';}
inline bool isdigitex(int ch) {return ch >= '0' && ch <= '9';}
inline bool isxdigitex(int ch) {return ch >= '0' && ch <= '9' || ch >= 'A' && ch <= 'F' || ch >= 'a' && ch <= 'f';}
inline bool isspaceex(int ch) {return ch == ' ' || ch == '\t';}
inline bool iswspaceex(int ch) {return isspaceex(ch) || ch == '\r' || ch == '\n';}

#endif // UTIL_H
