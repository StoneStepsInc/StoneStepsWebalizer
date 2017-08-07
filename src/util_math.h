/*
   webalizer - a web server log analysis program

   Copyright (c) 2004-2017, Stone Steps Inc. (www.stonesteps.ca)
   Copyright (C) 1997-2001  Bradford L. Barrett (brad@mrunix.net)

   See COPYING and Copyright files for additional licensing and copyright information 
   
   util_math.h
*/

#ifndef UTIL_MATH_H
#define UTIL_MATH_H

#include <cstdint>

//
//
//
inline double AVG(double curavg, double value, uint64_t newcnt) {return curavg+(value-curavg)/(double)newcnt;}
inline double AVG(double curavg, uint64_t value, uint64_t newcnt) {return AVG(curavg, (double) value, newcnt);}

inline double AVG2(double a1, uint64_t n1, double a2, uint64_t n2) {return a1+(a2-a1)*((double)n2/((double)n1+(double)n2));}

inline double PCENT(double val, double max) {return max ? (val/max)*100.0 : 0.0;}
inline double PCENT(uint64_t val, uint64_t max) {return PCENT((double) val, (double) max);}

#endif // UTIL_MATH_H
