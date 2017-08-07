/*
   webalizer - a web server log analysis program

   Copyright (c) 2004-2017, Stone Steps Inc. (www.stonesteps.ca)

   See COPYING and Copyright files for additional licensing and copyright information 
   
   output.cpp
*/
#include "pch.h"

#ifndef _WIN32
#include <unistd.h>                           /* normal stuff             */
#endif

#include "lang.h"
#include "hashtab.h"
#include "linklist.h"
#include "graphs.h"
#include "output.h"
#include "util_path.h"
#include "history.h"
#include "exception.h"

#include <ctime>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cctype>

output_t::output_t(const config_t& config, const state_t& state) : state(state), config(config)
{
   makeimgs = false;
   graphinfo = NULL;
}

output_t::~output_t(void)
{
   if(makeimgs)
      delete graphinfo;
}

/*********************************************/
/* OPEN_OUT_FILE - Open file for output      */
/*********************************************/

FILE *output_t::open_out_file(const char *filename) const
{
   FILE *out_fp;

   /* open the file... */
   if ( (out_fp=fopen(make_path(config.out_dir, filename),"w")) == NULL)
   {
      if (config.verbose)
         fprintf(stderr,"%s %s!\n",config.lang.msg_no_open,filename);
      return NULL;
   }
   return out_fp;
}

output_t::graphinfo_t *output_t::alloc_graphinfo(void)
{
   if(!graphinfo) {
      graphinfo = new graphinfo_t;
      makeimgs = true;
   }
   return graphinfo;
}

int output_t::qs_cc_cmpv(const void *cp1, const void *cp2)
{
   uint64_t  t1, t2;

   // compare visits first
   t1=(*(ccnode_t**)cp1)->visits;
   t2=(*(ccnode_t**)cp2)->visits;

   if(t1 != t2) 
      return t2 < t1 ? -1 : 1;

   // then hits
   t1=(*(ccnode_t**)cp1)->count;
   t2=(*(ccnode_t**)cp2)->count;

   if(t1 != t2) 
      return t2 < t1 ? -1 : 1;

   /* if hits are the same, we sort by country code instead */
   return strcmp((*(ccnode_t**)cp1)->ccode, (*(ccnode_t**)cp2)->ccode);
}
