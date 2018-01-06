/*
   webalizer - a web server log analysis program

   Copyright (c) 2004-2018, Stone Steps Inc. (www.stonesteps.ca)
   Copyright (C) 1997-2001  Bradford L. Barrett (brad@mrunix.net)

   See COPYING and Copyright files for additional licensing and copyright information 
   
   util_time.h
*/
#ifndef UTIL_TIME_H
#define UTIL_TIME_H

uint64_t usec2msec(uint64_t usec);

string_t cur_time(bool local_time);

uint64_t elapsed(uint64_t stime, uint64_t etime);

#endif // UTIL_TIME_H
