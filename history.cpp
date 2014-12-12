/*
   webalizer - a web server log analysis program

   Copyright (c) 2004-2014, Stone Steps Inc. (www.stonesteps.ca)
   Copyright (C) 1997-2001  Bradford L. Barrett (brad@mrunix.net)

   See COPYING and Copyright files for additional licensing and copyright information 
   
   history.cpp
*/
#include "pch.h"

#include "history.h"
#include "lang.h"

#include <stdio.h>

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
   u_int displen;
   return ((displen = length()) > 12) ? displen : 12;
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

   return ((12 - (disp_length() - hptr->month) % 12) % 12) + 1;
}

const hist_month_t *history_t::find_month(u_int year, u_int month) const
{
   const hist_month_t *hptr;
   const_iterator iter = months.begin();

   while(iter.more()) {
      hptr = &iter.next();
      if(hptr->month == month && hptr->year == year)
         return hptr;
   }

   return NULL;
}

bool history_t::update(const hist_month_t *month)
{
   size_t i;

   if(month == NULL)
      return false;

   // just add, if the history is empty
   if(months.size() == 0) {
      months.push(*month);
      return true;
   }

   // find the right spot in the months array
   for(i = months.size()-1; i >= 0; i--) {
      if(month->year == months[i].year) {
         if(month->month > months[i].month)
            break;

         // overwrite the same month
         if(month->month == months[i].month) {
            months[i] = *month;
            return true;
         }
      }

      if(month->year > months[i].year)
         break;
   }
   
   // and insert the new month
   months.insert(++i, *month);

   // trim the history if it's too long
   while(length() > max_length)
      months.remove(0);

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

bool history_t::get_history(void)
{
   int numfields;
   size_t i;
   FILE *hist_fp;
   hist_month_t hnode;
   char *buffer;
   string_t fpath;

   fpath = make_path(config.out_dir, config.hist_fname);

   if((hist_fp=fopen(fpath,"r")) == NULL) {
      if (verbose>1) 
         printf("%s\n", config.lang.msg_no_hist);
      return false;
   }

   buffer = new char[BUFSIZE];

   if (verbose>1) 
      printf("%s %s\n", config.lang.msg_get_hist, fpath.c_str());

   while ((fgets(buffer,BUFSIZE,hist_fp)) != NULL)
   {
      /* month# year# requests files sites xfer firstday lastday visits */
      numfields = sscanf(buffer,"%d %d %"SCNu64" %"SCNu64" %"SCNu64" %"SCNu64" %d %d %"SCNu64" %"SCNu64"",
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
         if (verbose)
            fprintf(stderr,"%s (mth=%d)\n",config.lang.msg_bad_hist, hnode.month);
         continue;
      }
      
      //
      // Find the right spot in the months array. The input list is most likely
      // sorted (not guaranteed, though), so start from the end. There should be 
      // no duplicates and we don't need to do assignments (i.e. inserts only).
      //
      for(i = months.size()-1; i >= 0; i--) {
         // if it's the same year and the new month is greater, we found the spot
         if(hnode.year == months[i].year && hnode.month > months[i].month)
            break;

         //
         // If the new year is greater at this point, then its the first one and 
         // month doesn't matter (or we would get it above)
         //
         if(hnode.year > months[i].year)
            break;
      }
      
      // insert the new month after the one we found of last
      months.insert(++i, hnode);
   }
   
   fclose(hist_fp);
   delete [] buffer;

   // remove extra months from history
   while(length() > max_length)
      months.remove(0);
      
   return true;
}

/*********************************************/
/* PUT_HISTORY - write out history file      */
/*********************************************/

void history_t::put_history(void)
{
   FILE *hist_fp;
   const hist_month_t *hptr;
   history_t::const_iterator iter = begin();
   string_t fpath;

   fpath = make_path(config.out_dir, config.hist_fname);

   if((hist_fp = fopen(fpath,"w")) == NULL) {
      if (verbose)
         fprintf(stderr,"%s %s\n", config.lang.msg_hist_err, fpath.c_str());
      return;
   }

   if (verbose>1) 
      printf("%s\n",config.lang.msg_put_hist);

   while(iter.more()) {
      hptr = &iter.next();
      if(hptr->hits != 0) {
         fprintf(hist_fp,"%d %d %"PRIu64" %"PRIu64" %"PRIu64" %"PRIu64" %d %d %"PRIu64" %"PRIu64"\n",
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
}
