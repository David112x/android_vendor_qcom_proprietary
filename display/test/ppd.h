/*
 * Copyright (c) 2017, 2019 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 */

#ifndef _POSTPROC_TESTER_H
#define _POSTPROC_TESTER_H

#include <iostream>

using namespace std;

#ifdef POSTPROC_DEBUG
#define log_debug(x) printf("postproc_debug: %s\n", x)
#else
#define log_debug(x)
#endif

#define log_error(x) printf("postproc_error: %s\n", x)
#define log_info(x)  printf("postproc_info: %s\n", x)

class postprocTester {
public:
    bool sendDppsCommand(char* cmd);

    postprocTester() {}
    ~postprocTester() {}
};

#endif /* _POSTPROC_TESTER_H */
