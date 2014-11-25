/*
    webalizer - a web server log analysis program

    Copyright (c) 2004-2014, Stone Steps Inc. (www.stonesteps.ca)

    See COPYING and Copyright files for additional licensing and copyright information

    event.h
*/

#ifndef __EVENT_H
#define __EVENT_H

#if defined(_WIN32)
#include <windows.h>
#else
#include <pthread.h>
#endif

#include "types.h"

#if defined(_WIN32)
typedef HANDLE event_t;
#else
typedef struct event_imp_t {
   pthread_cond_t    condition;
   pthread_mutex_t   mutex;
   bool              manual;
   bool              signalled;
} *event_t;
#endif

enum event_result_t {
   EVENT_OK       = 0,
   EVENT_ERROR    = 1,
   EVENT_TIMEOUT  = 2
};

event_t event_create(bool manual, bool signalled);
void event_destroy(event_t event);
event_result_t event_wait(event_t event, uint32_t timeout);
bool event_set(event_t event);
bool event_reset(event_t event);

#endif /* __EVENT_H */
