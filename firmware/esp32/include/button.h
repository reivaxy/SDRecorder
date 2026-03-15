#ifndef BUTTON_H
#define BUTTON_H

#include <Arduino.h>

enum class ButtonState {
  IDLE,
  SHORT_PRESS,
  LONG_PRESS,
  VERY_LONG_PRESS
};

class Button {
private:
  uint8_t pin;
  unsigned long longPressDuration;
  unsigned long veryLongPressDuration;
  unsigned long pressStartTime;
  bool isCurrentlyPressed;
  ButtonState currentState;
  unsigned long lastDebounceTime;
  int lastReadValue;
  static const unsigned long DEBOUNCE_TIME = 50; // ms

public:
  Button(uint8_t pin, unsigned long longPressDuration = 2000, unsigned long veryLongPressDuration = 6000);
  void run();
  ButtonState readState();
};

#endif
