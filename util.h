/*
   webalizer - a web server log analysis program

   Copyright (c) 2004-2014, Stone Steps Inc. (www.stonesteps.ca)
   Copyright (C) 1997-2001  Bradford L. Barrett (brad@mrunix.net)

   See COPYING and Copyright files for additional licensing and copyright information 
   
   util.h
*/

#ifndef __UTIL_H
#define __UTIL_H

#include <stddef.h>
#include <time.h>

#include "types.h"
#include "tstring.h"

#ifdef _WIN32
#define F_OK 00      // existance
#define W_OK 02      // write access
#define R_OK 04      // read access
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

//
// Boyer-Moore-Horspool delta table (see strstr_ex for details)
//
class bmh_delta_table {
   private:
      u_int                *deltas;

      static const u_int   table_size;

   public:
      bmh_delta_table(void) {deltas = NULL;}

      bmh_delta_table(const char *str, u_int slen = 0) : deltas(NULL) {reset(str, slen);}

      ~bmh_delta_table(void) {if(deltas) delete [] deltas;}

      void reset(const char *str, u_int slen = 0);

      bool isvalid(void) const {return !(deltas == NULL);}

      u_int operator [] (u_int chr) const {return deltas[chr];}
};

//
// [Unicode v5 ch.3 p.103]
//
// Scalar Value        First Byte  Second Byte Third Byte
// 00000000 0xxxxxxx   0xxxxxxx
// 00000yyy yyxxxxxx   110yyyyy    10xxxxxx
// zzzzyyyy yyxxxxxx   1110zzzz    10yyyyyy    10xxxxxx
//
// Returns the number of output bytes and, if out is not NULL, converts
// the UCS-2 character to a UTF-8 sequence.
//
inline u_int ucs2utf8(wchar_t wchar, char *out)
{
   // 1-byte character
	if(wchar <= 0x7F) {
	   if(out)
		   *out++ = wchar & 0x7F;
		return 1;
	}
	
	// 2-byte sequence
	if(wchar <= 0x7FF) {
	   if(out) {
		   *out++ = (wchar >> 6) | 0xC0;
		   *out++ = (wchar & 0x3F) | 0x80;
		}
		return 2;
	}

   // 3-byte sequence
   if(out) {
	   *out++ = (wchar >> 12) | 0xE0;
	   *out++ = ((wchar & 0xFC0) >> 6) | 0x80;
	   *out = (wchar & 0x3F) | 0x80;
	}
	return 3;
}

//
// [Unicode v5 ch.3 p.104]
//
// Code Points          First Byte  Second Byte Third Byte  Fourth Byte
// U+0000   .. U+007F   00..7F
// U+0080   .. U+07FF   C2..DF      80..BF
// U+0800   .. U+0FFF   E0          A0..BF      80..BF
// U+1000   .. U+CFFF   E1..EC      80..BF      80..BF
// U+D000   .. U+D7FF   ED          80..9F      80..BF
// U+E000   .. U+FFFF   EE..EF      80..BF      80..BF
// U+10000  .. U+3FFFF  F0          90..BF      80..BF      80..BF
// U+40000  .. U+FFFFF  F1..F3      80..BF      80..BF      80..BF
// U+100000 .. U+10FFFF F4          80..8F      80..BF      80..BF
//
// Returns the number of bytes in a UTF-8 character or zero if the sequence
// of bytes is not a valid UTF-8 character.
//
inline u_int utf8bcnt(const u_char *cp)
{
   #define CHKRNG(ch, lo, hi) ((ch) >= lo && (ch) <= hi)

   if(!cp)
      return 0;

   // 1-byte character (including zero terminator)
   if(*cp <= 0x7F)
      return 1;
   
   // 2-byte sequences
   if(CHKRNG(*cp, 0xC2, 0xDF))
      return CHKRNG(*(cp+1), 0x80, 0xBF) ? 2 : 0;
   
   // 3-byte sequences
   if(*cp == 0xE0)
      return CHKRNG(*(cp+1), 0xA0, 0xBF) && CHKRNG(*(cp+2), 0x80, 0xBF) ? 3 : 0;
      
   if(CHKRNG(*cp, 0xE1, 0xEC))
      return CHKRNG(*(cp+1), 0x80, 0xBF) && CHKRNG(*(cp+2), 0x80, 0xBF) ? 3 : 0;
      
   if(*cp == 0xED)
      return CHKRNG(*(cp+1), 0x80, 0x9F) && CHKRNG(*(cp+2), 0x80, 0xBF) ? 3 : 0;
      
   if(CHKRNG(*cp, 0xEE, 0xEF))
      return CHKRNG(*(cp+1), 0x80, 0xBF) && CHKRNG(*(cp+2), 0x80, 0xBF) ? 3 : 0;

   // 4-byte sequences
   if(*cp == 0xF0)
      return CHKRNG(*(cp+1), 0x90, 0xBF) && CHKRNG(*(cp+2), 0x80, 0xBF) &&  CHKRNG(*(cp+3), 0x80, 0xBF) ? 4 : 0;

   if(CHKRNG(*cp, 0xF1, 0xF3))
      return CHKRNG(*(cp+1), 0x80, 0xBF) && CHKRNG(*(cp+2), 0x80, 0xBF) &&  CHKRNG(*(cp+3), 0x80, 0xBF) ? 4 : 0;
      
   if(*cp == 0xF4)
      return CHKRNG(*(cp+1), 0x80, 0x8F) && CHKRNG(*(cp+2), 0x80, 0xBF) &&  CHKRNG(*(cp+3), 0x80, 0xBF) ? 4 : 0;
   
   return 0;
   
   #undef CHKRNG
}

//
//
//
u_int ucs2utf8(const wchar_t *str, u_int slen, char *out, u_int bsize);

//
//
//
inline double AVG(double curavg, double value, uint64_t newcnt) {return curavg+(value-curavg)/(double)newcnt;}
inline double AVG(double curavg, uint64_t value, uint64_t newcnt) {return AVG(curavg, (double) value, newcnt);}

inline double AVG2(double a1, uint64_t n1, double a2, uint64_t n2) {return a1+(a2-a1)*((double)n2/((double)n1+(double)n2));}

inline double PCENT(double val, double max) {return max ? (val/max)*100.0 : 0.0;}
inline double PCENT(uint64_t val, uint64_t max) {return PCENT((double) val, (double) max);}

uint64_t usec2msec(uint64_t usec);

bool is_secure_url(u_int urltype, bool use_https);
u_int url_path_len(const char *url, u_int *urllen);
bool is_http_redirect(u_int respcode);
bool is_http_error(u_int respcode);

const char *strstr_ex(const char *str1, const char *str2, u_int l1, u_int l2 = 0, const bmh_delta_table *delta = NULL);

size_t strncpy_ex(char *dest, size_t destsize, const char *src, size_t srclen);

int strncmp_ex(const char *str1, u_int slen1, const char *str2, u_int slen2);

string_t& url_decode(const string_t& str, string_t& out);

char *html_encode(const char *str, char *buffer, u_int bsize, bool resize, bool multiline = false, u_int *olen = NULL);
char *xml_encode(const char *str, char *buffer, u_int bsize, bool resize, bool multiline = false, u_int *olen = NULL);

u_char from_hex(u_char);                           /* convert hex to dec  */
const u_char *from_hex(const u_char *cp1, u_char *cp2);

const char *cstr2str(const char *cp, string_t& str);

u_int ul2str(uint64_t value, char *str);
uint64_t str2ul(const char *str, const char **eptr = NULL, u_int len = ~0);
int64_t str2l(const char *str, const char **eptr = NULL, u_int len = ~0);

string_t cur_time(bool local_time);
void cur_time_ex(bool local_time, string_t& date, string_t& time, string_t *tzname);

uint64_t ctry_idx(const char *);
string_t idx_ctry(uint64_t idx);

const char *get_domain(const char *, u_int);       /* return domain name  */
char *get_url_domain(const char *url, char *buffer, u_int bsize);

bool is_abs_path(const char *path);
string_t get_cur_path(void);
string_t make_path(const char *base, const char *path);

bool is_ip4_address(const char *str);

uint64_t elapsed(uint64_t stime, uint64_t etime);

bool isinstrex(const char *str, const char *cp, u_int slen, u_int cplen, bool substr, const bmh_delta_table *deltas);

template <typename char_t>
const char_t *strptr(const char_t *str, const char_t *defstr = NULL);

// avoid any locale-specific checks and conversions
inline bool isalphaex(int ch) {return ch >= 'A' && ch <= 'Z' || ch >= 'a' && ch <= 'z';}
inline bool isdigitex(int ch) {return ch >= '0' && ch <= '9';}
inline bool isxdigitex(int ch) {return ch >= '0' && ch <= '9' || ch >= 'A' && ch <= 'F' || ch >= 'a' && ch <= 'f';}
inline bool isspaceex(int ch) {return ch == ' ' || ch == '\t';}
inline bool iswspaceex(int ch) {return isspaceex(ch) || ch == '\r' || ch == '\n';}

#endif // __UTIL_H
