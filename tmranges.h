/*
    webalizer - a web server log analysis program

    Copyright (c) 2004-2013, Stone Steps Inc. (www.stonesteps.ca)

    See COPYING and Copyright files for additional licensing and copyright information

    tmranges.h
*/

#ifndef __TMRANGE_H
#define __TMRANGE_H

#include <time.h>

#include "types.h"
#include "tstamp.h"
#include "vector.h"

class tm_ranges_t {
   public:
      typedef u_int iterator;
      
   private:
      struct tm_range_t {
         tstamp_t    start;      // inclusive
         tstamp_t    end;        // exclusive
         
         public:
            tm_range_t(const tstamp_t& start, const tstamp_t& end) : start(start), end(end) {}
      };
   
   private:
      vector_t<tm_range_t> tm_ranges;
      
   public:
      tm_ranges_t(void);

      iterator begin(void) const {return 0;}
      
      bool add_range(const tstamp_t& start, const tstamp_t& end);
      
      bool is_in_range(const tstamp_t& tstamp, iterator& cur_range) const;
};

#endif // __TMRANGE_H
