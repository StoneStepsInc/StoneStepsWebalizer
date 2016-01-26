/*
    webalizer - a web server log analysis program

    Copyright (c) 2004-2016, Stone Steps Inc. (www.stonesteps.ca)

    See COPYING and Copyright files for additional licensing and copyright information

    event_win.cpp
*/
#include "../pch.h"

#include "../event.h"

#include <windows.h>

struct event_handle_t {
   HANDLE event_handle;
   event_handle_t(HANDLE event_handle) : event_handle(event_handle) {}
};

event_t event_create(bool manual, bool signalled)
{
   HANDLE event_handle;

   if((event_handle = CreateEvent(NULL, manual, signalled, NULL)) == NULL)
      return NULL;

   return new event_handle_t(event_handle);
}

void event_destroy(event_t event)
{
   if(event) {
      CloseHandle(event->event_handle);
      delete event;
   }
}

event_result_t event_wait(event_t event, uint32_t timeout)
{
   if(!event)
      return EVENT_ERROR;

   switch(WaitForSingleObject(event->event_handle, timeout)) {
      case WAIT_OBJECT_0:
         return EVENT_OK;

      case WAIT_TIMEOUT:
         return EVENT_TIMEOUT;
   }

   return EVENT_ERROR;
}

bool event_set(event_t event)
{
   return (event && SetEvent(event->event_handle)) ? true : false;
}

bool event_reset(event_t event)
{
   return (event && ResetEvent(event->event_handle)) ? true : false;
}
