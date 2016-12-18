/*
    webalizer - a web server log analysis program

    Copyright (c) 2004-2016, Stone Steps Inc. (www.stonesteps.ca)

    See COPYING and Copyright files for additional licensing and copyright information

    thread.h
*/
#ifndef THREAD_H
#define THREAD_H

#include "types.h"

struct thread_handle_t;
typedef thread_handle_t *thread_t;

#if defined(_WIN32)
typedef unsigned (__stdcall *start_routine_t)(void*); 
#else
typedef void *(*start_routine_t)(void*);
#endif

thread_t thread_create(start_routine_t start_routine, void *arg);
void thread_destroy(thread_t thread);
void msleep(unsigned long timeout);
uint64_t msecs(void);
unsigned long thread_id(void);

#endif /* THREAD_H */
