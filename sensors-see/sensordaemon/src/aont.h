/*
 * Copyright (c) 2018 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 */
#pragma once
#include "worker.h"
#include "ssc_utils.h"
#include <vector>
#include <iostream>
#include <memory>

using namespace std;

/**
 * @brief Utility class for sensordaemon
 *        dataytpe
 *
 */
class aont
{
 public:
    /**
     * constructor
     *
     * @aont test constructor
     */
    aont();
    /**
     * desstructor
     *
     * @aont test desstructor
     */
    ~aont();

    /**
     * enable or disable aont
     *
     * @param bool true for enable and false for disable
     * @return success or fail
     */
    int enable();

private:
    std::unique_ptr<ssc_connection> _ssc_conn;
    std::unique_ptr<suid_lookup> _lookup;
    void suid_cbk(const std::string& datatype,
                       const std::vector<sensor_uid>& suids);
    /**
     * enable aont test
     *
     * @param void
     * @prints the aont event received
     */
    static void handle_aont_event(const uint8_t *data, size_t size);
    void send_request(sensor_uid suid);
};
