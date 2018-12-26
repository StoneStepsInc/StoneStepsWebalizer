/*
   webalizer - a web server log analysis program

   Copyright (c) 2004-2019, Stone Steps Inc. (www.stonesteps.ca)
   Copyright (C) 1997-2001  Bradford L. Barrett (brad@mrunix.net)

   See COPYING and Copyright files for additional licensing and copyright information 
   
   util_math.h
*/

#ifndef UTIL_MATH_H
#define UTIL_MATH_H

#include <cstdint>

///
/// @brief  Computes an average from the current average, new `double` value and 
///         the new number of items.
///
inline double AVG(double curavg, double value, uint64_t newcnt) {return curavg+(value-curavg)/(double)newcnt;}

///
/// @brief  Computes an average from the current average, new `uint64_t` value and 
///         the new number of items.
///
inline double AVG(double curavg, uint64_t value, uint64_t newcnt) {return AVG(curavg, (double) value, newcnt);}

///
/// @brief  Computes an average from two average values and corresponding two number
///         of items values.
///
inline double AVG2(double a1, uint64_t n1, double a2, uint64_t n2) {return a1+(a2-a1)*((double)n2/((double)n1+(double)n2));}

///
/// @brief  Computes a percentage for a `double` value
///
inline double PCENT(double val, double max) {return max ? (val/max)*100.0 : 0.0;}

///
/// @brief  Computes a percentage for an `uint64_t` value
///
inline double PCENT(uint64_t val, uint64_t max) {return PCENT((double) val, (double) max);}

#endif // UTIL_MATH_H
