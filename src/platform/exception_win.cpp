/*
   webalizer - a web server log analysis program

   Copyright (c) 2004-2021, Stone Steps Inc. (www.stonesteps.ca)

   See COPYING and Copyright files for additional licensing and copyright information 
   
   exception_win.cpp
*/
#include "../pch.h"

#include "../exception.h"

#include <eh.h>
#include <windows.h>

static string_t get_ex_desc(unsigned int excode, _EXCEPTION_POINTERS *exinfo)
{
   string_t extext;
      
   // check if we have everything we need
   if(!exinfo || !exinfo->ExceptionRecord) {
      extext = "Missing Windows exception information";
      return extext;
   }
         
   // walk the exception chain, starting from the first exception record
   EXCEPTION_RECORD *exrec = exinfo->ExceptionRecord;
   while(exrec) {
      // add a separator between chained exceptions
      if(!extext.isempty())
         extext += "; ";
      else
         extext = "Windows exception: ";
            
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
      
//
// Win32 detailed exception reporting
//
void __cdecl _win32_se_handler(unsigned int excode, _EXCEPTION_POINTERS *exinfo)
{
   throw os_ex_t(0, get_ex_desc(excode, exinfo));
}

void set_os_ex_translator(void)
{
   // set a callback to translate Win32 structured exceptions to C++ exceptins
   _set_se_translator(_win32_se_handler);    // $$$ check if has to be reset to the default callback !!!
}
