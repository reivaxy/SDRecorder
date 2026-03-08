#ifndef LED_H
#define LED_H

#include <Arduino.h>

class BlinkMode {
public:
    BlinkMode(unsigned long onDuration, unsigned long offDuration, int maxBlinks = 0);
    unsigned long getOnDuration() const { return onDuration; }
    unsigned long getOffDuration() const { return offDuration; }
    int getMaxBlinks() const { return maxBlinks; }

private:
    unsigned long onDuration;
    unsigned long offDuration;
    int maxBlinks;  // 0 for infinite, >0 for limited blinks
};

class Led {
public:
    Led(int pin);
    void setMode(const BlinkMode mode);
    void off();
    void run();

private:
    int pin;
    BlinkMode currentMode;
    unsigned long lastToggleTime;
    bool isOn; // blinking state
    bool modeOff;
    unsigned long currentInterval;
    int blinkCount;
};

#endif