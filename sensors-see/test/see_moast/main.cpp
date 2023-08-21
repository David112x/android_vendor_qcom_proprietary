/* ===================================================================
**
** Copyright (c) 2018-2020 Qualcomm Technologies, Inc.
** All Rights Reserved.
** Confidential and Proprietary - Qualcomm Technologies, Inc.
**
** FILE: main.cpp
** DESC: Entry class for the program to start
** ===================================================================*/

#include <cstdlib>        // EXIT_SUCCESS/EXIT_FAILURE
#include <exception>      // std::exception
#include <iostream>       // std::cout
#include <sstream>      // std::ostringstream

#include "Parser.h"
#include "Utility.h"
#include "Moast.h"

using namespace sensors_moast;

void usage()
{
   std::cout << "usage: adb shell see_moast sensor_list.txt config.txt\n"
             << "note: The full paths for sensor_list and config files\n";
}

int main(int argc, char *argv[])
{
    Utility utility; // handle

    std::cout << "see_moast v2.2" << std::endl;

    if(argc < 2) {
        std::cout << "Missing arguments\n";
        usage();
        return(EXIT_FAILURE);
    }

    std::string argv1 = argv[1];
    if(argv1 == "-h" || argv1 == "-help") {
        usage();
        return(EXIT_SUCCESS);
    }
    if(argv1 == "-v" || argv1 == "-version") {
        return(EXIT_SUCCESS);
    }
    try {
        Parser::parse_input_files(argv);
        Moast* moast_instance = Moast::get_moast_instance();
        if ( moast_instance != nullptr) {
           moast_instance->get_config_test_params();
           moast_instance->open_qmi_clients();
        }

        // timestamp completion
        std::ostringstream oss;
        std::ostringstream action;
        std::string done = "see_moast run complete.\n";
        action << done << "PASS " << done;
        utility.create_anchor_msg( oss, action.str(), "");
        utility.write_line(oss);
    }
    catch(std::exception const& e) {
        std::cout << "FAIL Standard exception: " << e.what() << std::endl;
        return(EXIT_FAILURE);
    }
    catch(const char* ex) {
        std::cout << "FAIL Exception message: " << ex << std::endl;
        return(EXIT_FAILURE);
    }
    catch(...) { // Catch all exception handler
        std::cout << "FAIL Caught exception of undetermined type" << std::endl;
        return(EXIT_FAILURE);
    }
   return 0;
}
///////////////////////////////////////////////////////////////////////////////
