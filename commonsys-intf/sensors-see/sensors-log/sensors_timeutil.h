/*
 * Copyright (c) 2017-2019 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 */
#pragma once
#include <cstdint>
#include <cinttypes>

/* This is platform specific code, so make sure that we are compiling
   for correct architecture */
#if defined(__aarch64__)
#define TARGET_ARM64
#elif defined(__arm__)
#define TARGET_ARM
#else
#error "target cpu architecture not supported"
#endif

#include "utils/SystemClock.h"

#include "sensors_log.h"
#ifdef SNS_LE_QCS605
#include <atomic>
#endif

/**
 * @brief Sensors time utilities
 *
 * Provides utility functions for
 *  - reading QTimer ticks/frequency and timestamps
 *  - converting QTimer timestamps to android system clock
 *    timestamps and vice versa.
 */
class sensors_timeutil
{
public:

    /**
     * @brief get singleton instance of the timeutil class
     * @return sensors_timeutil&
     */
    static sensors_timeutil& get_instance()
    {
        static sensors_timeutil inst;
        return inst;
    }

    /**
     * @brief reads the current QTimer count value
     * @return uint64_t QTimer tick-count
     */
    uint64_t qtimer_get_ticks()
    {
    #if defined(TARGET_ARM64)
        unsigned long long val = 0;
        asm volatile("mrs %0, cntvct_el0" : "=r" (val));
        return val;
    #else
        uint64_t val;
        unsigned long lsb = 0, msb = 0;
        asm volatile("mrrc p15, 1, %[lsb], %[msb], c14"
                     : [lsb] "=r" (lsb), [msb] "=r" (msb));
        val = ((uint64_t)msb << 32) | lsb;
        return val;
    #endif
    }

    /**
     * @brief get QTimer frequency in Hz
     * @return uint64_t
     */
    uint64_t qtimer_get_freq()
    {
    #if defined(TARGET_ARM64)
        uint64_t val = 0;
        asm volatile("mrs %0, cntfrq_el0" : "=r" (val));
        return val;
    #else
        uint32_t val = 0;
        asm volatile("mrc p15, 0, %[val], c14, c0, 0" : [val] "=r" (val));
        return val;
    #endif
    }

    /**
     * @brief get time in nanoseconds since boot-up
     * @return uint64_t nanoseconds
     */
    uint64_t qtimer_get_time_ns()
    {
        return qtimer_ticks_to_ns(qtimer_get_ticks());
    }

    /**
     * @brief convert the qtimer tick value of nanoseconds
     * @param ticks
     * @return uint64_t qtimer time in nanoseconds
     */
    uint64_t qtimer_ticks_to_ns(uint64_t ticks)
    {
        return uint64_t(double(ticks) * (double(NSEC_PER_SEC) / double(_qtimer_freq)));
    }

    /**
     * @brief convert qtimer timestamp (in ns) to android system
     *        time (in ns)
     * @param qtimer_ts
     * @return int64_t
     */
    int64_t qtimer_ns_to_elapsedRealtimeNano(uint64_t qtimer_ts)
    {
        int64_t realtime_ns = int64_t(qtimer_ts) + _offset_ns;
        if (update_qtimer_to_realtime_offset(realtime_ns)) {
            realtime_ns = int64_t(qtimer_ts) + _offset_ns;
        }
        return realtime_ns;
    }

    /**
     * @brief convert android system time (in ns) to qtimer
     *        timestamp (in ns)
     * @param android_ts
     * @return uint64_t
     */
    uint64_t elapsedRealtimeNano_to_qtimer_ns(int64_t android_ts)
    {
        update_qtimer_to_realtime_offset(android_ts);
        return (android_ts - _offset_ns);
    }

    /**
     * @brief convert qtimer timestamp (in ticks) to android
     *        timestamp (in ns)
     * @param qtimer_ticks
     * @return int64_t
     */
    int64_t qtimer_ticks_to_elapsedRealtimeNano(uint64_t qtimer_ticks)
    {
        return qtimer_ns_to_elapsedRealtimeNano(qtimer_ticks_to_ns(qtimer_ticks));
    }
    /**
     * @brief get offset between sensor time system and Android time system (in ns)
     * @return int64_t
     */
    int64_t getElapsedRealtimeNanoOffset()
    {
        return _offset_ns;
    }

    /**
     * @brief kick in the logic to recalculate offset for drift
     * @param force_update if true, always update the offset (even if the
     *       previous update was more recent than the last update)
     * @return true if offset changed, false otherwise

     */

    bool recalculate_offset(bool force_update)
    {
       return update_qtimer_to_realtime_offset(android::elapsedRealtimeNano(), force_update);
    }

    uint64_t get_offset_update_schedule_ns() const
    {
       return OFFSET_UPDATE_SCHEDULE_NS;
    }

private:
    /**
     * @brief update offset for qtimer timestamp (in ticks) to android
     *        timestamp (in ns)
     * @param android_ts realtime timestamp (in ns)
     * @param force if true, recalculate offset even if last update was more
     *        recent than the usual update schedule
     * @return bool true if offset is changed, flase if offset is not changed
     */
    bool update_qtimer_to_realtime_offset(int64_t android_ts, bool force=false)
    {
        bool changed = false;
        int64_t last_android_ts = _last_realtime_ns;

        if (android_ts > 0 &&
            (force ||
            android_ts < last_android_ts ||
            android_ts - last_android_ts >= OFFSET_UPDATE_SCHEDULE_NS)) {
            uint64_t ns = 0, ns_end = 0;
            int iter = 0;
            int64_t old_offset_ns = _offset_ns;
            do {
                ns = qtimer_get_time_ns();
                int64_t curr_android_ts = android::elapsedRealtimeNano();
                if (ns <= std::numeric_limits<int64_t>::max() && curr_android_ts >= 0) {
                    _offset_ns = curr_android_ts - int64_t(ns);
                } else {
                    sns_loge("invalid time: ts = %" PRIu64 " curr_android_ts = %" PRId64,
                        ns, curr_android_ts);
                }
                ns_end = qtimer_get_time_ns();
                iter++;
            } while ((ns_end - ns > QTIMER_GAP_THRESHOLD_NS) &&
                        (iter < QTIMER_GAP_MAX_ITERATION));
            _last_realtime_ns = android_ts;
            sns_logd("updating qtimer-realtime offset, offset_diff = %" PRId64 \
                     " time_diff = %" PRId64, _offset_ns - old_offset_ns,
                     android_ts - last_android_ts);
            changed = true;
        }
        return changed;
    }

    sensors_timeutil() :
        _qtimer_freq(qtimer_get_freq()),
        _last_realtime_ns(android::elapsedRealtimeNano()),
        _offset_ns(_last_realtime_ns - int64_t(qtimer_get_time_ns()))
    {
    }
    ~sensors_timeutil() = default;
    /* delete copy constructors/assignment for singleton operation */
    sensors_timeutil(const sensors_timeutil&) = delete;
    sensors_timeutil& operator=(const sensors_timeutil&) = delete;

    /* QTimer frequency in Hz */
    const uint64_t _qtimer_freq;

    /* constant offset between the android elapsedRealTimeNano clock
       and QTimer clock in nanoseconds */
    // PEND: Move this variable back into the private section (see PEND below)
//    const int64_t _offset_ns;

    const uint64_t NSEC_PER_SEC = 1000000000ull;

    /* the last realtime timestamp converted or passed */
#ifndef SNS_LE_QCS605
    std::atomic<int64_t> _last_realtime_ns;
#else
    std::atomic<std::int64_t> _last_realtime_ns;
#endif
    /* how often offset needs to be updated */
    const int64_t OFFSET_UPDATE_SCHEDULE_NS = 60000000000ll;
    /* THRESHOLD to get minimum offset */
    const uint64_t QTIMER_GAP_THRESHOLD_NS = 10000;
    /* Max Interation to get minimum offset */
    const int QTIMER_GAP_MAX_ITERATION = 20;

public:
    // For some unknown reason, the Android and QTimer timestamps used to
    // calculate the _offset_ns only make sense if _offset_ns is a public
    // variable. I suspect that our method for reading the QTimer value may
    // have some unaccounted caveats.
    // PEND: Make this variable private again once we fix the root-cause for
    // the invalid QTimer values read during bootup
#ifndef SNS_LE_QCS605
    std::atomic<int64_t> _offset_ns;
#else
    std::atomic<std::int64_t> _offset_ns;
#endif
};
