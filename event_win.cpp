/*
    webalizer - a web server log analysis program

    Copyright (c) 2004-2014, Stone Steps Inc. (www.stonesteps.ca)

    See COPYING and Copyright files for additional licensing and copyright information

    event_win.cpp
*/
#include "pch.h"

#include "event.h"


event_t event_create(bool manual, bool signalled)
{
   return CreateEvent(NULL, manual, signalled, NULL);
}

void event_destroy(event_t event)
{
   if(event)
      CloseHandle(event);
}

uint32_t event_wait(event_t event, uint32_t timeout)
{
   if(!event)
      return EVENT_ERROR;

   switch(WaitForSingleObject(event, timeout)) {
      case WAIT_OBJECT_0:
         return EVENT_OK;

      case WAIT_TIMEOUT:
         return EVENT_TIMEOUT;
   }

   return EVENT_ERROR;
}

bool event_set(event_t event)
{
   return (event && SetEvent(event)) ? true : false;
}

bool event_reset(event_t event)
{
   return (event && ResetEvent(event)) ? true : false;
}
