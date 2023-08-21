/*
 * Copyright (c) 2017-2019 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 */

#pragma once

#ifndef USE_SENSOR_HAL_VER_2_0
#include <pthread.h>
#include <string>
#include "sensors_log.h"

const int FD_COUNT = 2;

class sns_wakelock {
public:
    /**
     * @brief creates wakelock instace to aquire/release wakelock
       with the name provided by string
     */
    sns_wakelock(std::string S);
    ~sns_wakelock();

    /**
     * @brief get holds the wakelock by calling write on
      *          wakelock fd
     * @note subsequent gets increaments the count as
     *           wakelock is already aquired.
     */
   void get();

   /**
    * @brief release the wakelock by calling write on
     *          release wakelock fd if count is "0"
                 if brelease is false then just decreaments the count
    * @note it will not release the wakelock if more elements
    *          in queue and releases only when all gets removed.
    */
   void put(bool brelease);

   private:
    int32_t _set_cnt;
    pthread_mutex_t _lock;
    /*each wakeup source in kernel needs a name to be associated*/
    std::string _name;
    /*FDs to hold below
        "/sys/power/wake_lock",
       "/sys/power/wake_unlock",
     */
    int _fds[FD_COUNT];
    bool _heldwakelock;
};

#else //USE_SENSOR_HAL_VER_2_0
#include <stdio.h>
#include <mutex>
#include <pthread.h>
#include <atomic>
using namespace std;

const int FD_COUNT = 2;

/*Singleton class which will interact with sns_wakelock
 * to acuire / release the wakelock based on the number of
 * wakeup events processed from FMQ */
class sns_wakelock {
public:
  /**
   * @brief Static function to get instance of sns_wakelock
   */
  static sns_wakelock* get_instance();

  /**
   * @brief Number of wakeup samples pushed to framework
   */
  void get_n_locks(unsigned int _in_count);

  /**
   * @brief Number of wakeup samples processed by framework
   */
  void put_n_locks(unsigned int _in_count);

  /**
   * @brief Framework processed all wakeup samples.
   *  This is used to release the wakelock in hal
   *  without any wakeup count conditions
   */
  void put_all();

  /**
   * @brief returns the status of wakelock
   */
  inline bool get_status() {
    std::lock_guard<std::mutex> lock(_mutex);
    return _is_held;
  }

  /**
   * @brief wait for wakelock to be held
   *  in wakelock FMQ thread in h/w module
   */
  void wait_for_held();
private:
  sns_wakelock();
  ~sns_wakelock();

  /* @brief acquire the wakelock
   * */
  void acquire();

  /* @brief release the wakelock based on processed count
   * */
  void release();

  /* @brief Instance to singleton class
   * */
  static sns_wakelock* _self;

  /* @brief Total number of unprocessed wakeup samples by framework
   * */
  unsigned int _accu_count;
  std::mutex _mutex;
  std::condition_variable _cv;
  int _fds[FD_COUNT];
  bool _is_held;
};
#endif // USE_SENSOR_HAL_VER_2_0
