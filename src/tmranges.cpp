/*
    webalizer - a web server log analysis program

    Copyright (c) 2004-2018, Stone Steps Inc. (www.stonesteps.ca)

    See COPYING and Copyright files for additional licensing and copyright information

    tmranges.cpp
*/
#include "pch.h"

#include "tmranges.h"

tm_ranges_t::tm_ranges_t(void)
{
}

bool tm_ranges_t::add_range(const tstamp_t& start, const tstamp_t& end)
{
   size_t i;
   tm_range_t range(start, end);
   
   // start must be less than end
   if(start >= end)
      return false;
   
   // move backward through the ranges and find the spot after the range with the end time less than start
   for(i = tm_ranges.size(); i > 0; i--) {
      if(start > tm_ranges[i-1].end)
         break;
   }

   // make sure that the end of the new range doesn't cross into the start of the next one
   if(i != tm_ranges.size()) {
      if(end >= tm_ranges[i+1].start)
         return false;
   }
   
   // and insert the range
   tm_ranges.insert(tm_ranges.begin()+i, range);
   return true;
}

bool tm_ranges_t::is_in_range(const tstamp_t& tstamp, iterator& cur_range) const
{
   if(cur_range >= tm_ranges.size())
      return false;

   // return false if before the current range
   if(tstamp < tm_ranges[cur_range].start)
      return false;
      
   while(cur_range < tm_ranges.size()) {
      // check if within the current range
      if(tstamp >= tm_ranges[cur_range].start && tstamp < tm_ranges[cur_range].end)
         return true;

      cur_range++;
                  
      // check if it's the last range
      if(cur_range == tm_ranges.size())
         break;
      
      // if before the new curent range, break out   
      if(tstamp < tm_ranges[cur_range].start)
         break;
   }
   
   return false;
}
