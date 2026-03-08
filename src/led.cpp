#include "led.h"

BlinkMode::BlinkMode(unsigned long onDuration, unsigned long offDuration, int maxBlinks)
    : onDuration(onDuration), offDuration(offDuration), maxBlinks(maxBlinks) {}

Led::Led(int p) : pin(p), currentMode(BlinkMode(0, 0, 0)), isOn(false), currentInterval(0), blinkCount(0) {
    pinMode(pin, OUTPUT);
    digitalWrite(pin, LOW);
}

void Led::setMode(const BlinkMode mode) {
    log_i("Setting LED mode: onDuration=%lu, offDuration=%lu, maxBlinks=%d", mode.getOnDuration(), mode.getOffDuration(), mode.getMaxBlinks());
    currentMode = mode;
    isOn = true;
    modeOff = false;
    blinkCount = 0;
    digitalWrite(pin, HIGH);
    lastToggleTime = millis();
    currentInterval = currentMode.getOnDuration();
  }
  
  void Led::off() {
    log_i("Setting LED off");
    modeOff = true;
}

void Led::run() {
    if (modeOff) {
        digitalWrite(pin, LOW);
        return;
    }

    if (millis() - lastToggleTime >= currentInterval) {
        isOn = !isOn;
        digitalWrite(pin, isOn ? HIGH : LOW);
        lastToggleTime = millis();

        if (!isOn) {
            blinkCount++;
            int maxBlinks = currentMode.getMaxBlinks();
            if (maxBlinks > 0 && blinkCount >= maxBlinks) {
                off();
                return;
            }
        }

        currentInterval = isOn ? currentMode.getOnDuration() : currentMode.getOffDuration();
    }
}