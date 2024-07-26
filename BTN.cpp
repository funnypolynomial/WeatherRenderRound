#include <Arduino.h>
#include "BTN.h"
#include "Config.h"

// buttons

BTN btn;


#define HOLD_TIME_MS 50
#define CLOSED_STATE LOW  // pin pulled LOW when pressed
#define OPEN_STATE   HIGH

void BTN::Init(int Pin)
{
  m_iPin = Pin;
  m_iPrevReading = OPEN_STATE;
  m_iPrevState = CLOSED_STATE;
  m_iTransitionTimeMS = millis();
  if (m_iPin)
    pinMode(m_iPin, INPUT_PULLUP);
}

bool BTN::CheckButtonPress()
{
  // debounced button, true if button pressed
  if (!m_iPin) return false;
  int ThisReading = digitalRead(m_iPin);
  if (ThisReading != m_iPrevReading)
  {
    // state change, reset the timer
    m_iPrevReading = ThisReading;
    m_iTransitionTimeMS = millis();
  }
  else if (ThisReading != m_iPrevState &&
           (millis() - m_iTransitionTimeMS) >= HOLD_TIME_MS)
  {
    // a state other than the last one and held for long enough
    m_iPrevState = ThisReading;
    return (ThisReading == CLOSED_STATE);
  } 
  return false;
}

bool BTN::IsDown()
{
  if (!m_iPin) return false;
  // non-debounced, instantaneous reading
  return digitalRead(m_iPin) == CLOSED_STATE;
}

bool BTN::CheckButtonPressed(bool& longPress, unsigned long holdTimeMS)
{
  // true if btn pressed, longPress is true if held down (holdTimeMS)
  longPress = false;
  if (btn.CheckButtonPress())
  {
    unsigned long startMS = millis();
    while ((longPress = btn.IsDown()) && (millis() - startMS) < holdTimeMS)
      ;
    return true;
  }
  return false;
}
