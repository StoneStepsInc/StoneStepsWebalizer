/*
   webalizer - a web server log analysis program

   Copyright (c) 2004-2019, Stone Steps Inc. (www.stonesteps.ca)
   Copyright (C) 1997-2001  Bradford L. Barrett (brad@mrunix.net)

   See COPYING and Copyright files for additional licensing and copyright information 
   
   util_ipaddr.h
*/

#ifndef UTIL_IPADDR_H
#define UTIL_IPADDR_H

bool is_ipv4_address(const char *str);
bool is_ipv6_address(const char *str);
bool is_ip_address(const char *str);

#endif // UTIL_IPADDR_H
