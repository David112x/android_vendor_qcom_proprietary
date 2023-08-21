/* ===================================================================
** Copyright (c) 2017-2020 Qualcomm Technologies, Inc.
** All Rights Reserved.
** Confidential and Proprietary - Qualcomm Technologies, Inc.
**
** FILE: see_salt_sleep.cpp
** DESC: sleep_and_awake() called by main thread to sleep for
**       milliseconds then if necessary wakeup the apps processor
**       from suspend mode and resume.
** ================================================================ */
#include <iostream>
#include <stdexcept>
#include <sys/timerfd.h>
#include <unistd.h>

/**
 * @brief sleep_and_awake() called by main thread to sleep for milliseconds then
 *        if necessary, wakeup the apps processor from suspend mode
 * @param milliseconds
 */
void sleep_and_awake( uint32_t milliseconds)
{
   struct timespec now;
   struct itimerspec new_value;
   int fd;
   uint64_t expired;
   ssize_t s;

   uint32_t secs = milliseconds / 1000; // sleep seconds
   long nsecs = ( milliseconds - (secs * 1000)) * 1000000; // sleep nanoseconds

   if (clock_gettime(CLOCK_BOOTTIME_ALARM, &now) == -1)
      throw std::runtime_error("timerfd clock_gettime");

   new_value.it_interval.tv_sec = 0;
   new_value.it_interval.tv_nsec = 0;
   new_value.it_value.tv_sec = now.tv_sec + secs;
   new_value.it_value.tv_nsec = now.tv_nsec + nsecs;
   if ( new_value.it_value.tv_nsec > 1000000000 ) {
      new_value.it_value.tv_nsec -= 1000000000;
      new_value.it_value.tv_sec++;
   }

   fd = timerfd_create(CLOCK_BOOTTIME_ALARM, 0);
   if (fd == -1)
       throw std::runtime_error("timerfd_create");

   if (timerfd_settime(fd, TFD_TIMER_ABSTIME, &new_value, NULL) == -1)
       throw std::runtime_error("timerfd_settime");

   s = read(fd, &expired, sizeof(uint64_t));
   if (s != sizeof(uint64_t))
      throw std::runtime_error("timerfd read");
}

