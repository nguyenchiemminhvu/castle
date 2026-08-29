#include "castle/logging/logging.h"

#include <iostream>
#include <sstream>

using namespace castle::logging;

int main()
{
    LOG_SET_LEVEL(log_level::debug);

    int satellites = 8;
    bool has_fix = true;
    LOG_INFO("Satellites in view: ", satellites, " (fix=", has_fix, ")");
    
    LOG_DEBUG("This is a debug message containing a number: ", 42);
    LOG_WARNING("UART buffer is filling up! Current load: ", 85, "%");
    LOG_ERROR("Hardware fault detected!");

    std::cout << "\n--- Testing local logger instance ---\n";
    
    // Instantiate a standalone logger targeting std::cerr instead of std::cout
    basic_logger my_local_logger(std::cerr);
    
    // Configure a localized filter independent of the global singleton
    my_local_logger.set_min_level(log_level::warning); 
    
    // This will be filtered out because log_level::info < log_level::warning
    my_local_logger.log_info("This info log will NOT be printed.");
    
    // This meets the criteria and prints straight to std::cerr
    my_local_logger.log_warning("Local logger alert: Temperature threshold exceeded!");

    return 0;
}
