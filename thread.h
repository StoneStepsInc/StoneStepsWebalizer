/*
    webalizer - a web server log analysis program

    Copyright (c) 2004-2014, Stone Steps Inc. (www.stonesteps.ca)

    See COPYING and Copyright files for additional licensing and copyright information

    thread.h
*/
#ifndef __THREAD_H
#define __THREAD_H


#if defined(_WIN32)
#include <windows.h>
#else
#include <pthread.h>
#endif

#if defined(_WIN32)
typedef HANDLE thread_t;
typedef unsigned (__stdcall *start_routine_t)(void*); 
#else
typedef pthread_t thread_t;
typedef void *(*start_routine_t)(void*);
#endif
		  
thread_t thread_create(start_routine_t start_routine, void *arg);
void thread_destroy(thread_t thread);
void msleep(unsigned long timeout);
unsigned long msecs(void);
unsigned long thread_id(void);

#endif /* __THREAD_H */
