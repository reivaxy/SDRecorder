#ifndef RTC_H
#define RTC_H

#include <Arduino.h>
#include <time.h>
#include <sys/time.h>

class RTC {
public:
    RTC();
    
    // Set time from Unix timestamp (seconds since 1970-01-01)
    void setTime(time_t unixTime);
    
    // Get current Unix timestamp
    time_t getTime();
    
    // Get current time as tm struct for date/time operations
    struct tm* getLocalTime();
    
    // Format time as string (e.g., "2025-03-16 14:30:45")
    String formatTime(time_t timestamp = 0);
    
    // Check if RTC has been initialized with valid time
    bool isInitialized() { return initialized; }
    
private:
    bool initialized;
};

#endif
