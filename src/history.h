/*
    webalizer - a web server log analysis program

    Copyright (c) 2004-2019, Stone Steps Inc. (www.stonesteps.ca)

    See COPYING and Copyright files for additional licensing and copyright information

    history.h
*/

#ifndef HISTORY_H
#define HISTORY_H

#include "types.h"
#include "config.h"

#include <vector>

///
/// @brief  A class that keeps historical data for one month
///
struct hist_month_t {
   u_int   year;           // year (4 digits)
   u_int   month;          // month number (one-based)

   int     fday;           // first day
   int     lday;           // last day

   uint64_t  hits; 
   uint64_t  files;
   uint64_t  hosts;
   uint64_t  xfer; 
   uint64_t  pages;
   uint64_t  visits;

   public:
      hist_month_t(void);

      hist_month_t(u_int year, u_int month, uint64_t hits, uint64_t files, uint64_t pages, uint64_t visits, uint64_t hosts, uint64_t xfer, u_int fday, u_int lday);
};

///
/// @class  history_t
///
/// @brief  A class that maintains monthly history data
///
class history_t {
   public:
      typedef std::vector<hist_month_t>::const_iterator const_iterator;
      typedef std::vector<hist_month_t>::const_reverse_iterator const_reverse_iterator;

   private:
      const config_t&         config;
      std::vector<hist_month_t> months;
      u_int                   max_length;

   public:
      history_t(const config_t& _config);

      ~history_t(void);

      void initialize(void);

      void cleanup(void);

      const_iterator begin(void) const {return months.begin();}

      const_iterator end(void) const {return months.end();}

      const_reverse_iterator rbegin(void) const {return months.rbegin();}

      const_reverse_iterator rend(void) const {return months.rend();}

      void set_max_length(u_int len) {max_length = (len > 12) ? len : 12;}

      u_int get_max_length(void) const {return max_length;}

      u_int length(void) const;

      u_int disp_length(void) const;

      void clear(void) {months.clear(); max_length = 0;}

      size_t size(void) const {return months.size();}

      const hist_month_t *first(void) const {return months.size() ? &months[0] : NULL;}

      const hist_month_t *last(void) const {return months.size() ? &months[months.size()-1] : NULL;}

      int month_index(u_int year, u_int month) const;

      u_int first_month(void) const;
                       
      const hist_month_t *find_month(u_int year, u_int month) const;

      bool update(const hist_month_t *month);

      bool update(u_int year, u_int month, uint64_t hits, uint64_t files, uint64_t pages, uint64_t visits, uint64_t hosts, uint64_t xfer, u_int fday, u_int lday);

      bool get_history(void);

      bool put_history(void);
};

#endif // HISTORY_H

