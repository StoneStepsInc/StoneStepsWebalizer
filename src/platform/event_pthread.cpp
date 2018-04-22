/*
   webalizer - a web server log analysis program

   Copyright (c) 2004-1013, Stone Steps Inc. (www.stonesteps.ca)

   See COPYING and Copyright files for additional licensing and copyright information     

   event_pthread.cpp
*/
#include <string.h>
#include <sys/time.h>
#include <errno.h>
#include "../event.h"

#include <pthread.h>

struct event_handle_t {
   pthread_cond_t    condition;
   pthread_mutex_t   mutex;
   bool              manual;
   bool              signalled;
};

static timespec *get_abs_time(uint32_t timeout, timespec *abstime) 
{
   struct timeval now;
   uint32_t sec, nsec;

   if(abstime == NULL)
      return NULL;

   gettimeofday(&now, NULL);

   // add timeout (nsec may overflow)
   nsec = (now.tv_usec * 1000) + (timeout % 1000) * 1000000ul;
   sec = now.tv_sec + timeout / 1000;

   abstime->tv_sec = sec + nsec / 1000000000ul;
   abstime->tv_nsec = nsec % 1000000000ul;

   return abstime;
}

event_t event_create(bool manual, bool signalled)
{
   event_t event = new event_handle_t;

   event->manual = manual;
   event->signalled = signalled;

   memset(&event->condition, 0, sizeof(pthread_cond_t));
   memset(&event->mutex, 0, sizeof(pthread_mutex_t));

   if(!pthread_cond_init(&event->condition, NULL)) {
      if(!pthread_mutex_init(&event->mutex, NULL))
         return event;
      pthread_cond_destroy(&event->condition);
   }

   delete event;
   return NULL;
}

void event_destroy(event_t event)
{
   if(event) {
      pthread_cond_destroy(&event->condition);
      pthread_mutex_destroy(&event->mutex);
      delete event;
   }
}

event_result_t event_wait(event_t event, uint32_t timeout)
{
	timespec abstime;
	int retval = 0;
   bool signalled = false;

	get_abs_time(timeout, &abstime);

	if(pthread_mutex_lock(&event->mutex))
		return EVENT_ERROR;

   /* wait until event is signalled or timeout expires */
   if(!event->signalled) {
	   do {
		   retval = pthread_cond_timedwait(&event->condition, &event->mutex, &abstime);
      } while(!event->signalled && retval == EINTR);
   }

   signalled = event->signalled;

   /* reset auto-reset events */
	if(retval == 0 && signalled && !event->manual)
		event->signalled = false;

	if(pthread_mutex_unlock(&event->mutex))
		return EVENT_ERROR;

   if(retval == ETIMEDOUT) 
      return EVENT_TIMEOUT;

   return (retval == 0 && signalled) ? EVENT_OK : EVENT_ERROR;
}

bool event_set(event_t event)
{
   bool retval = true;

	if(pthread_mutex_lock(&event->mutex))
		return false;

	event->signalled = true;
	
	if(event->manual)
		retval = pthread_cond_broadcast(&event->condition) ? false : true;
	else
		retval = pthread_cond_signal(&event->condition) ? false : true;

	if(pthread_mutex_unlock(&event->mutex))
		return false;

	return retval;
}

bool event_reset(event_t event)
{
	if(pthread_mutex_lock(&event->mutex))
		return false;

	event->signalled = false;
	
	if(pthread_mutex_unlock(&event->mutex))
		return false;

	return true;
}
