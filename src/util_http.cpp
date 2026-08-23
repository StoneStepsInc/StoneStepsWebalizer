/*
   webalizer - a web server log analysis program

   Copyright (c) 2004-2022, Stone Steps Inc. (www.stonesteps.ca)
   Copyright (C) 1997-2001  Bradford L. Barrett (brad@mrunix.net)

   See COPYING and Copyright files for additional licensing and copyright information 
   
   util_http.cpp
*/
#include "pch.h"

#include "util_http.h"

bool is_http_redirect(std::underlying_type<rc_t>::type respcode)
{
   switch (respcode) {
      case RC_MOVEDPERM:         // 301
      case RC_MOVEDTEMP:         // 302
      case RC_SEEOTHER:          // 303
      case RC_MOVEDTEMPORARILY:  // 307
         return true;
   }

   return false;
}

bool is_http_error(std::underlying_type<rc_t>::type respcode)
{
   std::underlying_type<rc_t>::type code = respcode / 100;
   return code == 4 || code == 5;
}
