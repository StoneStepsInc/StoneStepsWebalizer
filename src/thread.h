/*
    webalizer - a web server log analysis program

    Copyright (c) 2004-2018, Stone Steps Inc. (www.stonesteps.ca)

    See COPYING and Copyright files for additional licensing and copyright information

    thread.h
*/
#ifndef THREAD_H
#define THREAD_H

#include "types.h"

void msleep(unsigned long timeout);
uint64_t msecs(void);
unsigned long thread_id(void);

#endif /* THREAD_H */
