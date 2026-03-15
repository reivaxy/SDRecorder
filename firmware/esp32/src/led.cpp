#include "led.h"
#include "recorderPreferences.h"

BlinkMode::BlinkMode(unsigned long onDuration, unsigned long offDuration, int maxBlinks)
    : onDuration(onDuration), offDuration(offDuration), maxBlinks(maxBlinks) {}

Led::Led(int p) : pin(p), stackSize(0), preferences(nullptr) {
    pinMode(pin, OUTPUT);
    digitalWrite(pin, LOW);
}

// push a new mode onto the stack (maintain max depth)
static void pushMode(Led::ModeEntry stack[], int &stackSize, const BlinkMode &mode, int pin) {
    Led::ModeEntry entry;
    entry.mode = mode;
    entry.lastToggleTime = millis();
    entry.isOn = true;
    entry.modeOff = false;
    entry.currentInterval = mode.getOnDuration();
    entry.blinkCount = 0;

    if (stackSize >= Led::MODE_STACK_MAX) {
        // drop the oldest entry, shift left
        for (int i = 0; i < Led::MODE_STACK_MAX - 1; ++i) stack[i] = stack[i + 1];
        stack[Led::MODE_STACK_MAX - 1] = entry;
        // stackSize stays at max
    } else {
        stack[stackSize++] = entry;
    }

    // activate top entry immediately
    digitalWrite(pin, HIGH);
}

void Led::setMode(const BlinkMode mode) {
    // Check if LED is disabled in preferences
    if (preferences && preferences->getSettingBool(PREF_DISABLE_LED)) {
        log_i("LED mode requested but LED is disabled in preferences: onDuration=%lu, offDuration=%lu, maxBlinks=%d", mode.getOnDuration(), mode.getOffDuration(), mode.getMaxBlinks());
        return;
    }

    log_i("Setting LED mode: onDuration=%lu, offDuration=%lu, maxBlinks=%d", mode.getOnDuration(), mode.getOffDuration(), mode.getMaxBlinks());
    pushMode(stack, stackSize, mode, pin);
}

void Led::off() {
    log_i("Setting LED off");
    // clear stack and force off
    stackSize = 0;
    digitalWrite(pin, LOW);
}

void Led::run() {
    if (stackSize == 0) {
        // nothing to do, ensure LED off
        digitalWrite(pin, LOW);
        return;
    }

    ModeEntry &entry = stack[stackSize - 1];

    if (entry.modeOff) {
        digitalWrite(pin, LOW);
        return;
    }

    unsigned long now = millis();
    if (now - entry.lastToggleTime >= entry.currentInterval) {
        entry.isOn = !entry.isOn;
        digitalWrite(pin, entry.isOn ? HIGH : LOW);
        entry.lastToggleTime = now;

        if (!entry.isOn) {
            entry.blinkCount++;
            int maxBlinks = entry.mode.getMaxBlinks();
            if (maxBlinks > 0 && entry.blinkCount >= maxBlinks) {
                // pop this mode and restore previous if any
                stackSize--;
                if (stackSize == 0) {
                    // no previous mode: ensure LED off
                    digitalWrite(pin, LOW);
                    return;
                } else {
                    // resume previous top entry (do not reset its blinkCount)
                    ModeEntry &prev = stack[stackSize - 1];
                    // ensure the resumed entry starts consistently
                    prev.lastToggleTime = now;
                    // write the current state of resumed entry
                    digitalWrite(pin, prev.isOn ? HIGH : LOW);
                    return;
                }
            }
        }

        entry.currentInterval = entry.isOn ? entry.mode.getOnDuration() : entry.mode.getOffDuration();
    }
}