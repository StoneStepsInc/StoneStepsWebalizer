/*
   webalizer - a web server log analysis program

   Copyright (c) 2004-2021, Stone Steps Inc. (www.stonesteps.ca)

   See COPYING and Copyright files for additional licensing and copyright information 
   
   pch.h
*/
#ifndef PCHTEST_H
#define PCHTEST_H

//
// Google Test includes windows.h, which defines min/max macros that
// stomp on all other functionality using min/max, such as STL numeric
// limits. Disable these macros, so they don't leak into this code.
//
#define NOMINMAX

#include <gtest/gtest.h>

#endif // PCHTEST_H
