/*
   webalizer - a web server log analysis program

   Copyright (c) 2004-1013, Stone Steps Inc. (www.stonesteps.ca)

   See COPYING and Copyright files for additional licensing and copyright information     

   exception.h
*/
#ifndef __EXCEPTION_H
#define __EXCEPTION_H

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
		u_int		errcode;
		string_t	errdesc;

	public:
		exception_t(u_int code, const char *desc) {errcode = code; errdesc = desc;}

      exception_t(const exception_t& exception) {errcode = exception.errcode; errdesc = exception.errdesc;};

		u_int code(void) const {return errcode;}

		const string_t& desc(void) const {return errdesc;}
};

#ifdef _WIN32
// -----------------------------------------------------------------------
// ex_win32_se_t
//
// Win32 structured exception wrapper class that captures exception details
// and creates a formatted exception description string.
// -----------------------------------------------------------------------
class ex_win32_se_t : public exception_t {
   private:
      unsigned int   excode;        // exception code of the first exception
   
   private:   
      static string_t get_ex_desc(unsigned int excode, _EXCEPTION_POINTERS *exinfo)
      {
         string_t extext;
         u_int reccnt = 0;
      
         // check if we have everything we need
         if(!exinfo || !exinfo->ExceptionRecord) {
            extext = "Missing Windows exception information";
            return extext;
         }
         
         extext = "Windows exception: ";
         
         // walk the exception chain, starting from the first exception record
         EXCEPTION_RECORD *exrec = exinfo->ExceptionRecord;
         while(exrec) {
            // add a separator between chained exceptions
            if(!extext.isempty())
               extext += "; ";
            
            // grab the base exceptino information first
            extext += string_t::_format("code: %x flags: %x address: %p", exrec->ExceptionCode, exrec->ExceptionFlags, exrec->ExceptionAddress);
            
            // check if this exception record has any parameters
            if(exrec->NumberParameters) {
               // add them to the exception description
               extext += " (";
               for(unsigned int i = 0; i < exrec->NumberParameters; i++) {
                  if(i != 0)
                     extext += ", ";
                  extext += string_t::_format("%p", exrec->ExceptionInformation[i]);
               }
               extext += ")";
            }
            
            // move on to the next exception record
            exrec = exrec->ExceptionRecord;
         }
         
         return extext;
      }
      
   public:
      ex_win32_se_t(unsigned int excode, _EXCEPTION_POINTERS *exinfo) : 
            exception_t(0, get_ex_desc(excode, exinfo)), excode(excode)
      {
      }
};

#endif   // _WIN32

#endif // __EXCEPTION_H
