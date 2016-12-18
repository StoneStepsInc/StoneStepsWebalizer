/*
   webalizer - a web server log analysis program

   Copyright (c) 2004-1013, Stone Steps Inc. (www.stonesteps.ca)

   See COPYING and Copyright files for additional licensing and copyright information     

   exception.h
*/
#ifndef EXCEPTION_H
#define EXCEPTION_H

#include "types.h"
#include "tstring.h"

// -----------------------------------------------------------------------
// exception_t
//
// Generic exception class that is good for most simple cases where only
// a code and some text is required.
// -----------------------------------------------------------------------
class exception_t {
   private:
      u_int      errcode;
      string_t   errdesc;

   public:
      exception_t(u_int code, const char *desc) {errcode = code; errdesc = desc;}

      exception_t(const exception_t& exception) {errcode = exception.errcode; errdesc = exception.errdesc;};

      u_int code(void) const {return errcode;}

      const string_t& desc(void) const {return errdesc;}
};

class os_ex_t : public exception_t {
   public:
      os_ex_t(u_int code, const char *desc) : exception_t(code, desc) {}
};

//
//
//
void set_os_ex_translator(void);

#endif // EXCEPTION_H
