/*
   webalizer - a web server log analysis program

   Copyright (c) 2004-2021, Stone Steps Inc. (www.stonesteps.ca)
   Copyright (C) 1997-2001  Bradford L. Barrett (brad@mrunix.net)

   See COPYING and Copyright files for additional licensing and copyright information 
   
   history.cpp
*/
#include "pch.h"

#include "history.h"
#include "lang.h"
#include "util_path.h"
#include "tstring.h"

#include <cstdio>

//
//
//

hist_month_t::hist_month_t(void)
{
   month = year = 0;
   fday = lday = 0;
   hits = files = hosts = pages = visits = 0;
   xfer = 0;
}

hist_month_t::hist_month_t(u_int nyear, u_int nmonth, uint64_t nhits, uint64_t nfiles, uint64_t npages, uint64_t nvisits, uint64_t nhosts, uint64_t nxfer, u_int nfday, u_int nlday)
{
   year = nyear;
   month = nmonth;
   fday = nfday;
   lday = nlday;
   hits = nhits;
   files = nfiles;
   hosts = nhosts;
   pages = npages;
   visits = nvisits;
   xfer = nxfer;
}

//
//
//
history_t::history_t(const config_t& _config) : config(_config) 
{
   max_length = 12;
}

history_t::~history_t(void)
{
}

//
// Returns the number of months in the history
//
u_int history_t::length(void) const
{
   const hist_month_t *hp1, *hp2;
   
   if(months.size() == 0)
      return 0;

   hp1 = &months[0];
   hp2 = &months[months.size()-1];

   if(hp1->year == hp2->year)
      return hp2->month - hp1->month + 1;

   return (hp2->year - hp1->year - 1) * 12 + hp2->month + (12 - hp1->month) + 1;
}

//
// Returns the history display length, in months
//
u_int history_t::disp_length(void) const
{
   return length() > 12 ? length() : 12;
}

//
// Returns the month index. The index of the first month is 
// zero, the index of the last month is (disp_length()-1).
//
int history_t::month_index(u_int year, u_int month) const
{
   const hist_month_t *hptr;

   if(months.size() == 0)
      return 0;

   hptr = &months[months.size()-1];

   // if the same year, just return the difference in months
   if(hptr->year == year)
      return disp_length() - (hptr->month - month) - 1;

   //
   // otherwise, count the number of whole years and add 
   // the remainder of the first year and the first months 
   // of the last
   //
   return disp_length() - ((hptr->year - year - 1) * 12 + hptr->month + (12 - month)) - 1;
}

//
// Return the first month in history, given the last month 
// and the history display length
//
u_int history_t::first_month(void) const
{
   const hist_month_t *hptr;

   if(months.size() == 0)
      return 1;

   hptr = &months[months.size()-1];

   //
   //   1  4        1           1           1   5
   //   ...=========+-----------+-----------=====
   //      <-- 9 -->
   //      <---------------- 38 ---------------->
   //
   //    ((12 - (38 - 5) % 12) % 12) + 1 = 4
   //    ((12 - (24 - 12) % 12) % 12) + 1 = 1
   //
   // Subtract the last few months, so we can figure out the number of months in the 
   // partial year in front of the history range (the first % 12). We need the second 
   // modulo division to account for the history length being a multiple of 12.
   //
   return ((12 - (disp_length() - hptr->month) % 12) % 12) + 1;
}

const hist_month_t *history_t::find_month(u_int year, u_int month) const
{
   const hist_month_t *hptr;
   const_iterator iter = months.begin();

   while(iter != months.end()) {
      hptr = &*iter++;
      if(hptr->month == month && hptr->year == year)
         return hptr;
   }

   return nullptr;
}

bool history_t::update(const hist_month_t *month)
{
   int i;

   if(month == nullptr)
      return false;

   // if the history is empty, just add a new month/year pair
   if(months.size() == 0) {
      months.push_back(*month);
      return true;
   }

   // find the greatest month/year pair that is before the new one
   for(i = (int) months.size() - 1; i >= 0; i--) {
      if(month->year > months[i].year)
         break;

      if(month->year == months[i].year) {
         if(month->month > months[i].month)
            break;

         // overwrite the same month
         if(month->month == months[i].month) {
            months[i] = *month;
            return true;
         }
      }
   }
   
   // and insert the new month after the month/year we found
   months.insert(months.begin() + (i + 1), *month);

   //
   // History length includes empty months in between, so delete the one from
   // the start until history the length is within bounds.
   //
   while(length() > max_length)
      months.erase(months.begin());

   return true;
}

bool history_t::update(u_int year, u_int month, uint64_t hits, uint64_t files, uint64_t pages, uint64_t visits, uint64_t hosts, uint64_t xfer, u_int fday, u_int lday)
{
   hist_month_t hist(year, month, hits, files, pages, visits, hosts, xfer, fday, lday);
   return update(&hist);
}

void history_t::initialize(void)
{
   set_max_length(config.max_hist_length);
}

void history_t::cleanup(void)
{
}

///
/// @brief  Initializes monthly history data.
///
/// Reads and parses the history file. 
///
/// This method reports all errors to the stderr stream and returns `false` if the 
/// history file is malformed. If the history file is missing, the method prints a 
/// warning to the standard output and returns `true` to indicate success.
///
bool history_t::get_history(void)
{
   int numfields;
   size_t i;
   FILE *hist_fp;
   hist_month_t hnode;
   string_t fpath;

   fpath = make_path(config.out_dir, config.hist_fname);

   //
   // A history file may be missing for a variety of reasons and this should not
   // be considered as an error. Report the missing history file to the standard 
   // output, and return success.
   //
   if((hist_fp=fopen(fpath,"r")) == nullptr) {
      if (config.verbose>1) 
         printf("%s\n", config.lang.msg_no_hist);
      return true;
   }

   string_t::char_buffer_t buffer(BUFSIZE);

   if (config.verbose>1) 
      printf("%s %s\n", config.lang.msg_get_hist, fpath.c_str());

   while ((fgets(buffer,BUFSIZE,hist_fp)) != nullptr)
   {
      /* month# year# requests files sites xfer firstday lastday visits */
      numfields = sscanf(buffer,"%u %u %" SCNu64 " %" SCNu64 " %" SCNu64 " %" SCNu64 " %d %d %" SCNu64 " %" SCNu64 "",
                    &hnode.month,
                    &hnode.year,
                    &hnode.hits,
                    &hnode.files,
                    &hnode.hosts,
                    &hnode.xfer,
                    &hnode.fday,
                    &hnode.lday,
                    &hnode.pages,
                    &hnode.visits);

      if (numfields==8)     /* kludge for reading 1.20.xx history files */
      {
         hnode.pages = 0;
         hnode.visits = 0;
      }
      
      // check if the month is in the 1-12 range
      if(hnode.month == 0 || hnode.month > 12) {
         fprintf(stderr,"%s (mth=%d)\n",config.lang.msg_bad_hist, hnode.month);
         fclose(hist_fp);
         return false;
      }
      
      //
      // Find the right spot in the months array. The input list is most likely
      // sorted (not guaranteed, though), so start from the end. There should be 
      // no duplicates and we don't need to do assignments (i.e. inserts only).
      //
      for(i = months.size(); i > 0; i--) {
         const hist_month_t& hm = months[i-1];

         // break if the incoming date is greater than the current one
         if(hnode.year == hm.year && hnode.month > hm.month)
            break;

         if(hnode.year > hm.year)
            break;
      }
      
      // insert the new history month after the one we found
      months.insert(months.begin()+i, hnode);
   }
   
   fclose(hist_fp);

   // length includes empty months in between, so delete one and check how many are left
   while(length() > max_length)
      months.erase(months.begin());
      
   return true;
}

/*********************************************/
/* PUT_HISTORY - write out history file      */
/*********************************************/

bool history_t::put_history(void)
{
   FILE *hist_fp;
   const hist_month_t *hptr;
   history_t::const_iterator iter = begin();
   string_t fpath;

   fpath = make_path(config.out_dir, config.hist_fname);

   if((hist_fp = fopen(fpath,"w")) == nullptr) {
      fprintf(stderr,"%s %s\n", config.lang.msg_hist_err, fpath.c_str());
      return false;
   }

   if (config.verbose>1) 
      printf("%s\n",config.lang.msg_put_hist);

   while(iter != end()) {
      hptr = &*iter++;
      if(hptr->hits != 0) {
         fprintf(hist_fp,"%d %d %" PRIu64 " %" PRIu64 " %" PRIu64 " %" PRIu64 " %d %d %" PRIu64 " %" PRIu64 "\n",
                         hptr->month,
                         hptr->year,
                         hptr->hits,
                         hptr->files,
                         hptr->hosts,
                         hptr->xfer,
                         hptr->fday,
                         hptr->lday,
                         hptr->pages,
                         hptr->visits);
      }
   }
   fclose(hist_fp);

   return true;
}
