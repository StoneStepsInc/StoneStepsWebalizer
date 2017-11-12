/*
   webalizer - a web server log analysis program

   Copyright (c) 2004-2017, Stone Steps Inc. (www.stonesteps.ca)
   Copyright (C) 1997-2001  Bradford L. Barrett (brad@mrunix.net)

   See COPYING and Copyright files for additional licensing and copyright information 

   version.h
*/
#ifndef VERSION_H
#define VERSION_H

//
// Current application version components
//
#define VERSION_MAJOR           4
#define VERSION_MINOR           3
#define EDITION_LEVEL           0
#define BUILD_NUMBER            16

//
// Current numeric application version
//
#define VERSION         ((unsigned int) ((VERSION_MAJOR << 24) | (VERSION_MINOR << 16) | (EDITION_LEVEL << 8) | (BUILD_NUMBER)))

//
// Application version component extractors
//
#define VERSION_MAJOR_EX(version)     (((unsigned int) version >> 24) & 0xFFu)
#define VERSION_MINOR_EX(version)     (((unsigned int) version >> 16) & 0xFFu)
#define EDITION_LEVEL_EX(version)     (((unsigned int) version >>  8) & 0xFFu)
#define BUILD_NUMBER_EX(version)      (((unsigned int) version      ) & 0xFFu)

//
// Minimum application version in the state database we can load
//
#define MIN_APP_DB_VERSION 0x0400000Cu

#endif // VERSION_H
