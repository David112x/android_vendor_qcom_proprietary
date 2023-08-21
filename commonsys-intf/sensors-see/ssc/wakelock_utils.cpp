/*
 * Copyright (c) 2017-2019 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 */

#ifndef USE_SENSOR_HAL_VER_2_0

#include <errno.h>
#include "wakelock_utils.h"
#ifdef SNS_LE_QCS605
#include <string.h>
#endif

const char * const PATHS[] = {
 /*to hold wakelock*/
    "/sys/power/wake_lock",
 /*to release wakelock*/
    "/sys/power/wake_unlock",
};

/**
 * @brief sns_wakelock constructor to hold wakelock
 *          with the name specified as argument.
 * @note
 */
sns_wakelock::sns_wakelock(std::string S) {
    _name = S;
    _set_cnt = 0;
    _lock = PTHREAD_MUTEX_INITIALIZER;
    int i = 0;
    for (i=0; i<FD_COUNT; i++) {
        _fds[i] = -1;
        int fd = open(PATHS[i], O_RDWR | O_CLOEXEC);
        if (fd < 0) {
            sns_loge("fatal error opening \"%s\": %s\n", PATHS[i],
                strerror(errno));
        }
        _fds[i] = fd;
    }
    _heldwakelock = false;
 }

/**
 * @brief sns_wakelock destructor
 */
sns_wakelock::~sns_wakelock() {
    int i;

    if(true == _heldwakelock) {
      if (_fds[1] >= 0) {
        if (write( _fds[1],(char *)_name.c_str(), _name.length()) > 0) {
            sns_logv("sucess release %s wakelock", _name.c_str());
            _heldwakelock = false;
            _set_cnt = 0;
        } else {
            sns_loge("failed in write:%s wake unlock", _name.c_str());
        }
      }
    }

    for (i=0; i<FD_COUNT; i++) {
        if (_fds[i] >= 0)
            close(_fds[i]);
    }
 }


/**
 * @brief get holds the wakelock by calling write on
  *          wakelock fd
 * @note subsequent gets increaments the count as
 *           wakelock is already aquired.
 */
void sns_wakelock::get() {
    pthread_mutex_lock(&_lock);

    if (_fds[0] < 0) {
        /*it will flood if sns_loge till selinux changes merge so keeping logd*/
        sns_logd("open wakelock fd(%d) failed for %s",
        (int)_fds[0], _name.c_str());
        pthread_mutex_unlock(&_lock);
        return;
    }
    if (INT32_MAX == _set_cnt) {
        sns_loge("set count(:%ul) reached max for %s",
            _set_cnt, _name.c_str());
        pthread_mutex_unlock(&_lock);
        return;
    }
    if (_set_cnt++) {
        sns_logv("set count(:%ul)is > 1 for %s",
                _set_cnt, _name.c_str());
        pthread_mutex_unlock(&_lock);
        return;
    }
    if (_fds[0] >= 0) {
        if (write( _fds[0],(char *)_name.c_str(), _name.length()) > 0) {
            sns_logv("sucess wakelock get:%s", _name.c_str());
            _heldwakelock = true;
        } else {
            sns_loge("write fail wakelock get:%s", _name.c_str());
        }
    }

    pthread_mutex_unlock(&_lock);
    return;
}


/**
 * @brief release the wakelock by calling write on
  *          release wakelock fd if count is "0"
              if brelease is false then just decreaments the count
 * @note it will not release the wakelock if more elements
 *          in queue and releases only when all gets removed.
 */
void sns_wakelock::put(bool brelease) {
    pthread_mutex_lock(&_lock);

    if (_fds[1] < 0) {
        /*it will flood if sns_loge till selinux changes merge so keeping logd*/
        sns_logv("open release lock fd(%d) failed for %s",
        (int)_fds[1], _name.c_str());
        pthread_mutex_unlock(&_lock);
        return;
    }
    if (INT32_MIN == _set_cnt) {
        sns_logd("set count(:%ul) reached min for %s",
            _set_cnt, _name.c_str());
        /*there is a chance client calls with false for brelease many times
                 so count can become minimum */
        _set_cnt++;
        pthread_mutex_unlock(&_lock);
        return;
    }
    if ((false == _heldwakelock) ||
        (--_set_cnt > 0) ||
        ( false == brelease )) {
        sns_logv(" _heldwakelock:%d count:%ul for %s",
            (int)_heldwakelock, _set_cnt, _name.c_str());
        pthread_mutex_unlock(&_lock);
        return;
    }
    if (_fds[1] >= 0) {
        if (write( _fds[1],(char *)_name.c_str(), _name.length()) > 0) {
            sns_logv("sucess release %s wakelock", _name.c_str());
            _heldwakelock = false;
            _set_cnt = 0;
        } else {
            sns_loge("failed in write:%s wake unlock", _name.c_str());
        }
    }

    pthread_mutex_unlock(&_lock);
    return;
}

#else
#include "wakelock_utils.h"

#include "sensors_log.h"

const char* SNS_SENSORS_WACKLOCK_NAME = "SensorsHAL_WAKEUP";
#define SNS_POWER_WAKE_lOCK_ACQUIRE_INDEX 0
#define SNS_POWER_WAKE_lOCK_RELEASE_INDEX 1
const char * const PATHS[] = {
    /*to hold wakelock*/
    "/sys/power/wake_lock",
    /*to release wakelock*/
    "/sys/power/wake_unlock",
};

sns_wakelock* sns_wakelock::_self= nullptr;

sns_wakelock* sns_wakelock::get_instance()
{
  if(nullptr != _self) {
    return _self;
  }
  else {
    _self = new sns_wakelock();
    return _self;
  }
}

sns_wakelock::sns_wakelock()
{
  _is_held = false;
  _accu_count = 0;
  for (int i=0; i<FD_COUNT; i++) {
    _fds[i] = -1;
    int fd = open(PATHS[i], O_RDWR | O_CLOEXEC);
    if (fd < 0) {
      sns_loge("fatal error opening \"%s\": %s\n", PATHS[i],
          strerror(errno));
    }
    _fds[i] = fd;
  }
}

sns_wakelock::~sns_wakelock()
{
  std::lock_guard<std::mutex> lock(_mutex);
  if(true == _is_held) {
    _accu_count = 0;
    release();
  }

  for (int i=0; i<FD_COUNT; i++) {
    if (_fds[i] >= 0)
      close(_fds[i]);
  }
}
void sns_wakelock::wait_for_held()
{
  std::unique_lock<std::mutex> lock(_mutex);
  if(false == _is_held) {
    _cv.wait(lock);
  }
}
void sns_wakelock::get_n_locks(unsigned int _in_count)
{
  std::lock_guard<std::mutex> lock(_mutex);
  if (_in_count > std::numeric_limits<unsigned int>::max() - _accu_count) {
      sns_loge("_in_count %d will overflow max of _accu_count %u", _in_count, _accu_count);
      return;
  }
  if(_accu_count == 0 && false == _is_held)
    acquire();
  _accu_count = _accu_count + _in_count;
}

void sns_wakelock::put_n_locks(unsigned int _in_count)
{
  std::lock_guard<std::mutex> lock(_mutex);
  if (_in_count > _accu_count) {
      sns_loge("_in_count %d is larger than _accu_count %u", _in_count, _accu_count);
      return;
  }
  _accu_count = _accu_count - _in_count;
  if(_accu_count == 0 && true ==_is_held)
    release();
}
void sns_wakelock::put_all() {
  std::lock_guard<std::mutex> lock(_mutex);
  if(true == _is_held) {
    _accu_count = 0;
    release();
  }
}

void sns_wakelock::acquire()
{
  if (_fds[SNS_POWER_WAKE_lOCK_ACQUIRE_INDEX] >= 0) {
    if (write( _fds[SNS_POWER_WAKE_lOCK_ACQUIRE_INDEX],SNS_SENSORS_WACKLOCK_NAME, strlen(SNS_SENSORS_WACKLOCK_NAME)+1) > 0) {
      sns_logv("sucess wakelock acquire:%s", SNS_SENSORS_WACKLOCK_NAME);
      _is_held = true;
      _cv.notify_one();
    } else {
      sns_loge("write fail wakelock acquire:%s", SNS_SENSORS_WACKLOCK_NAME);
    }
  } else {
    sns_loge("Not able to open fd for wakeup:%s", SNS_SENSORS_WACKLOCK_NAME);
  }
}

void sns_wakelock::release()
{
  if (_fds[SNS_POWER_WAKE_lOCK_RELEASE_INDEX] >= 0) {
    if (write( _fds[SNS_POWER_WAKE_lOCK_RELEASE_INDEX],SNS_SENSORS_WACKLOCK_NAME, strlen(SNS_SENSORS_WACKLOCK_NAME)+1) > 0) {
      sns_logv("sucess release %s wakelock", SNS_SENSORS_WACKLOCK_NAME);
      _is_held = false;
    } else {
      sns_loge("failed in write:%s wake unlock", SNS_SENSORS_WACKLOCK_NAME);
    }
  } else {
    sns_loge("Not able to open fd for wakeup:%s", SNS_SENSORS_WACKLOCK_NAME);
  }
}
#endif // USE_SENSOR_HAL_VER_2_0
