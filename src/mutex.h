/*
    webalizer - a web server log analysis program

    Copyright (c) 2004-2017, Stone Steps Inc. (www.stonesteps.ca)

    See COPYING and Copyright files for additional licensing and copyright information

    mutex.h
*/

#ifndef MUTEX_H
#define MUTEX_H

struct mutex_handle_t;
typedef mutex_handle_t *mutex_t;

mutex_t mutex_create(void);
void mutex_destroy(mutex_t mutex);
void mutex_lock(mutex_t mutex);
void mutex_unlock(mutex_t mutex);

#endif /* MUTEX_H */
