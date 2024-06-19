#include "logger.h"
#include "config.h"
#include <fstream>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <iostream>

void logEvent(const std::vector<std::string>& events) {
    std::ofstream logFile(LOG_FILE, std::ios_base::app);
    if (!logFile.is_open()) {
        std::cerr << "Failed to open log file: " << LOG_FILE << std::endl;
        return;
    }

    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);
    std::tm buf;
    localtime_r(&in_time_t, &buf);

    logFile << std::put_time(&buf, "%Y-%m-%d %H:%M:%S") << " ";

    for (const auto& event : events) {
        logFile << event << " ";
    }

    logFile << std::endl;

    logFile.close();
}