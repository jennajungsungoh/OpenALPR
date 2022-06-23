
#include <iostream>
#include <string>
#include <vector>
#include "plog/Log.h"
#include "plog/Initializers/RollingFileInitializer.h"
#include "logger.h"


//gboolean log_enable(const char* _log_domain)
bool logging_start()
{
    plog::init(plog::debug, "logger.txt"); // Step2: initialize the logger
    return 1;
}

int Logging_Index(int arr[][3])
{
    std::cout << "hello\n";

    PLOGD << "Hello log!";


    return 0;
}