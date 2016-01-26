/*
    webalizer - a web server log analysis program

    Copyright (c) 2004-2016, Stone Steps Inc. (www.stonesteps.ca)

    See COPYING and Copyright files for additional licensing and copyright information

    mutex.h
*/

#ifndef __MUTEX_H
#define __MUTEX_H

struct mutex_handle_t;
typedef mutex_handle_t *mutex_t;
		  
mutex_t mutex_create(void);
void mutex_destroy(mutex_t mutex);
void mutex_lock(mutex_t mutex);
void mutex_unlock(mutex_t mutex);

#endif /* __MUTEX_H */
