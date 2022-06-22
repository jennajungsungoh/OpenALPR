
#include <iostream>
#include "plog/Log.h"
#include "plog/Initializers/RollingFileInitializer.h"
#include "logger.h"


//gboolean log_enable(const char* _log_domain)

int Logging_Index(int arr[][3])
{
    std::cout << "hello\n";
    plog::init(plog::debug, "Hello.txt"); // Step2: initialize the logger

    // Step3: write log messages using a special macro
    // There are several log macros, use the macro you liked the most

    PLOGD << "Hello log!"; // short macro
    PLOG_DEBUG << "Hello log!"; // long macro
    PLOG(plog::debug) << "Hello log!"; // function-style macro



    return 0;
}