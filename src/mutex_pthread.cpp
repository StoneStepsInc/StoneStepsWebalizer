/*
   webalizer - a web server log analysis program

   Copyright (c) 2004-1013, Stone Steps Inc. (www.stonesteps.ca)

   See COPYING and Copyright files for additional licensing and copyright information     

   mutex_pthread.cpp
*/
#include "mutex.h"
#include <memory.h>

#include <pthread.h>

struct mutex_handle_t {
   pthread_mutex_t *mutex_handle;
   mutex_handle_t((pthread_mutex_t *mutex_handle) : mutex(mutex_handle) {}
};

mutex_t mutex_create(void)
{
	pthread_mutexattr_t attr;
	pthread_mutex_t *mutex;

	if(pthread_mutexattr_init(&attr))
      return NULL;

	if(!pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE)) {
      mutex = new pthread_mutex_t;
      memset(mutex, 0, sizeof(pthread_mutex_t));

	   if(pthread_mutex_init(mutex, &attr)) {
         delete mutex;
         mutex = NULL;
      }
   }

   pthread_mutexattr_destroy(&attr);

   return new pthread_handle_t(mutex);
}

void mutex_destroy(mutex_t mutex)
{
	if(mutex) {
		pthread_mutex_destroy(&mutex->mutex_handle);
      delete mutex->mutex_handle;
		delete mutex;
	}
}

void mutex_lock(mutex_t mutex)
{
   if(mutex)
	   pthread_mutex_lock(mutex->mutex_handle);
}

void mutex_unlock(mutex_t mutex)
{
   if(mutex)
	   pthread_mutex_unlock(mutex->mutex_handle);
}
