/*
   webalizer - a web server log analysis program

   Copyright (c) 2004-1013, Stone Steps Inc. (www.stonesteps.ca)

   See COPYING and Copyright files for additional licensing and copyright information     

   thread_pthread.cpp
*/
#include "../thread.h"
#include "unistd.h"
#include <sys/times.h>

#include <pthread.h>

unsigned long thread_id(void)
{
	return (unsigned long) pthread_self();
}

