#ifndef LED_H
#define LED_H

#include <Arduino.h>

class RecorderPreferences;  // Forward declaration

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
    void setPreferences(RecorderPreferences* prefs) { preferences = prefs; }
    void setMode(const BlinkMode mode);
    void off();
    void run();

    // Mode stack: supports pushing temporary modes that restore previous modes
    struct ModeEntry {
        ModeEntry() : mode(0,0,0), lastToggleTime(0), isOn(false), modeOff(false), currentInterval(0), blinkCount(0) {}
        BlinkMode mode;
        unsigned long lastToggleTime;
        bool isOn;
        bool modeOff;
        unsigned long currentInterval;
        int blinkCount;
    };

    static const int MODE_STACK_MAX = 4;

private:
    int pin;
    ModeEntry stack[MODE_STACK_MAX];
    int stackSize; // number of valid entries in stack (0..MODE_STACK_MAX)
    RecorderPreferences* preferences;
};

#endif