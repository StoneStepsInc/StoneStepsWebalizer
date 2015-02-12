/*
   webalizer - a web server log analysis program

   Copyright (c) 2004-1013, Stone Steps Inc. (www.stonesteps.ca)

   See COPYING and Copyright files for additional licensing and copyright information     

   thread_pthread.cpp
*/
#include "thread.h"
#include "unistd.h"
#include <sys/times.h>

thread_t thread_create(start_routine_t start_routine, void *arg)
{
	pthread_attr_t attr;
	pthread_t thread = 0;

	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM);

	if(pthread_create(&thread, &attr, start_routine, arg)) 
		thread = 0;
	 
	pthread_attr_destroy(&attr);

	return thread;
}

void thread_destroy(thread_t thread)
{
}

void msleep(unsigned long timeout)
{
	timespec req, rem;

	req.tv_sec = timeout / 1000;
	req.tv_nsec = (timeout % 1000) * 1000000ul;

	while(nanosleep(&req, &rem) != 0)
		req = rem;
}

uint64_t msecs(void)
{
   static unsigned long clocks_per_sec = sysconf(_SC_CLK_TCK);
   struct tms mytms;    /* bogus tms structure         */

   return ((uint64_t) times(&mytms) * 1000ull) / clocks_per_sec;
}

unsigned long thread_id(void)
{
	return (unsigned long) pthread_self();
}

