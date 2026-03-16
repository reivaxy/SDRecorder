#include "rtc.h"

RTC::RTC() : initialized(false) {
}

void RTC::setTime(time_t unixTime) {
    // Set system time using timeval
    struct timeval tv;
    tv.tv_sec = unixTime;
    tv.tv_usec = 0;
    
    // Set the system clock
    settimeofday(&tv, nullptr);
    
    initialized = true;
    
    // Log the time that was set
    struct tm* timeinfo = localtime(&unixTime);
    log_i("RTC time set to: %04d-%02d-%02d %02d:%02d:%02d",
        timeinfo->tm_year + 1900,
        timeinfo->tm_mon + 1,
        timeinfo->tm_mday,
        timeinfo->tm_hour,
        timeinfo->tm_min,
        timeinfo->tm_sec);
}

time_t RTC::getTime() {
    return time(nullptr);
}

struct tm* RTC::getLocalTime() {
    time_t now = getTime();
    return localtime(&now);
}

String RTC::formatTime(time_t timestamp) {
    if (timestamp == 0) {
        timestamp = getTime();
    }
    
    struct tm* timeinfo = localtime(&timestamp);
    char buffer[30];
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", timeinfo);
    return String(buffer);
}
