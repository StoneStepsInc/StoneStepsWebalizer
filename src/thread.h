/*
    webalizer - a web server log analysis program

    Copyright (c) 2004-2022, Stone Steps Inc. (www.stonesteps.ca)

    See COPYING and Copyright files for additional licensing and copyright information

    thread.h
*/
#ifndef THREAD_H
#define THREAD_H

#include "types.h"
#include <chrono>
#include <thread>

///
/// @brief  Suspends the current thread for `timeout` milliseconds.
///
inline void msleep(unsigned long timeout)
{
   std::this_thread::sleep_for(std::chrono::milliseconds(timeout));
}

///
/// @brief  Returns a number of milliseconds since epoch.
///
inline uint64_t msecs(void)
{
   std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
   std::chrono::system_clock::time_point zero;

   return (uint64_t) std::chrono::duration_cast<std::chrono::milliseconds>(now - zero).count();
}

///
/// @brief  Returns a numeric current thread identifier.
///
unsigned long thread_id(void);

#endif /* THREAD_H */
