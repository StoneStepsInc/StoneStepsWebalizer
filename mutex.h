/*
    webalizer - a web server log analysis program

    Copyright (c) 2004-2014, Stone Steps Inc. (www.stonesteps.ca)

    See COPYING and Copyright files for additional licensing and copyright information

    mutex.h
*/

#ifndef __MUTEX_H
#define __MUTEX_H

#if defined(_WIN32)
#include <windows.h>
#else
#include <pthread.h>
#endif

#if defined(_WIN32)
typedef CRITICAL_SECTION *mutex_t;
#else
typedef pthread_mutex_t *mutex_t;
#endif
		  
mutex_t mutex_create(void);
void mutex_destroy(mutex_t mutex);
void mutex_lock(mutex_t mutex);
void mutex_unlock(mutex_t mutex);

#endif /* __MUTEX_H */
