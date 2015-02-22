/*
   webalizer - a web server log analysis program

   Copyright (c) 2004-2015, Stone Steps Inc. (www.stonesteps.ca)
   Copyright (C) 1997-2001  Bradford L. Barrett (brad@mrunix.net)

   See COPYING and Copyright files for additional licensing and copyright information 

   version.h
*/
#ifndef __VERSION_H
#define __VERSION_H

//
// Current application version components
//
#define VERSION_MAJOR           4
#define VERSION_MINOR           0
#define EDITION_LEVEL           0
#define BUILD_NUMBER            1

//
// Current numeric application version
//
#define VERSION			((unsigned int) ((VERSION_MAJOR << 24) | (VERSION_MINOR << 16) | (EDITION_LEVEL << 8) | (BUILD_NUMBER)))

//
// Application version component extractors
//
#define VERSION_MAJOR_EX(version)     (((unsigned int) version >> 24) & 0xFFu)
#define VERSION_MINOR_EX(version)     (((unsigned int) version >> 16) & 0xFFu)
#define EDITION_LEVEL_EX(version)     (((unsigned int) version >>  8) & 0xFFu)
#define BUILD_NUMBER_EX(version)      (((unsigned int) version      ) & 0xFFu)

//
// Previous application versions for conditional processing
//
#define VERSION_3_3_1_5	   0x03030105u
#define VERSION_3_5_1_1    0x03050101u
#define VERSION_3_8_0_4    0x03080004u

//
// Minimum application version in the state database we can load
//
#define MIN_APP_DB_VERSION 0x04000001u

#endif // __VERSION_H

