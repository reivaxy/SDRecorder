#include "button.h"

Button::Button(uint8_t pin, unsigned long longPressDuration, unsigned long veryLongPressDuration)
  : pin(pin), longPressDuration(longPressDuration),
    veryLongPressDuration(veryLongPressDuration),
    pressStartTime(0), isCurrentlyPressed(false),
    currentState(ButtonState::IDLE), lastDebounceTime(0),
    lastReadValue(HIGH) {
  pinMode(pin, INPUT_PULLUP);
}

void Button::run() {
  int reading = digitalRead(pin);

  // Debounce: detect if reading changed
  if (reading != lastReadValue) {
    lastDebounceTime = millis();
  }

  // If reading has been stable for DEBOUNCE_TIME, process state changes
  if ((millis() - lastDebounceTime) > DEBOUNCE_TIME) {
    if (reading == LOW && !isCurrentlyPressed) {
      // Button press detected
      isCurrentlyPressed = true;
      pressStartTime = millis();
    }
    else if (reading == HIGH && isCurrentlyPressed) {
      // Button release detected
      isCurrentlyPressed = false;
      unsigned long pressDuration = millis() - pressStartTime;

      if (pressDuration >= veryLongPressDuration) {
        currentState = ButtonState::VERY_LONG_PRESS;
      } else if (pressDuration >= longPressDuration) {
        currentState = ButtonState::LONG_PRESS;
      } else {
        currentState = ButtonState::SHORT_PRESS;
      }
    }
  }

  lastReadValue = reading;
}

ButtonState Button::readState() {
  ButtonState stateToReturn = currentState;
  currentState = ButtonState::IDLE;
  if (stateToReturn != ButtonState::IDLE) {
    log_i("Button state: %d", static_cast<int>(stateToReturn));
  }
  return stateToReturn;
}
