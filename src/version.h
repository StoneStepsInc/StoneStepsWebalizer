/*
   webalizer - a web server log analysis program

   Copyright (c) 2004-2021, Stone Steps Inc. (www.stonesteps.ca)
   Copyright (C) 1997-2001  Bradford L. Barrett (brad@mrunix.net)

   See COPYING and Copyright files for additional licensing and copyright information 

   version.h
*/
#ifndef VERSION_H
#define VERSION_H

//
// Current application version components
//
#define VERSION_MAJOR           6
#define VERSION_MINOR           2
#define VERSION_PATCH           0

//
// BUILD_NUMBER is assigned in webalizer_release.props from the value of
// a user macro $(AZP_BUILD_NUMBER), which is defined in the same property
// sheet. This macro is intended for release builds only and is expected
// to be assigned an auto-incremented value in a build pipeline. For manual
// builds, it is expected to be zero for all builds.
//
// Note that the full version, including build number, is recorded in the
// state database and a manual build with a build number zero won't be able
// to open a database made by a pipeline build with a non-zero build number.
// AZP_BUILD_NUMBER may be changed in this case to open such database for
// debugging, but this change must not be checked in.
//
#ifndef BUILD_NUMBER
#define BUILD_NUMBER            0
#endif

//
// Current version is maintained as a 32-bit number and cannot have
// any component greater than an 8-bit maximum value would allow. This
// shouldn't be a problem in the current state of the project, because
// CI builds are not very frequent, but if build frequency increases,
// version logic should change to use a 64-bit integer and the database
// logic should be updated to deal with 32-bit versions.
//
#if BUILD_NUMBER > 255
#error Build number cannot be greater than 255
#endif

//
// Current numeric application version
//
#define VERSION         ((unsigned int) ((VERSION_MAJOR << 24) | (VERSION_MINOR << 16) | (VERSION_PATCH << 8) | (BUILD_NUMBER)))

//
// Application version component extractors
//
#define VERSION_MAJOR_EX(version)     (((unsigned int) version >> 24) & 0xFFu)
#define VERSION_MINOR_EX(version)     (((unsigned int) version >> 16) & 0xFFu)
#define VERSION_PATCH_EX(version)     (((unsigned int) version >>  8) & 0xFFu)
#define BUILD_NUMBER_EX(version)      (((unsigned int) version      ) & 0xFFu)

//
// Minimum application version in the state database we can load
//
#define MIN_APP_DB_VERSION 0x0400000Cu

#endif // VERSION_H
