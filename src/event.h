/*
    webalizer - a web server log analysis program

    Copyright (c) 2004-2017, Stone Steps Inc. (www.stonesteps.ca)

    See COPYING and Copyright files for additional licensing and copyright information

    event.h
*/

#ifndef EVENT_H
#define EVENT_H

#include "types.h"

struct event_handle_t;
typedef event_handle_t *event_t;

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

#endif /* EVENT_H */
