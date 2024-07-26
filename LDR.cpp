#include <Arduino.h>
#include "Pins.h"
#include "Config.h"
#include "LDR.h"
#include "DiskLCD.h"

// Simple brightness control. 
// Hysteresis, and the intent is just to dim the display at night

namespace LDR
{
byte currentPWM = 0;
void Init()
{
  pinMode(PIN_LDR, INPUT);
  currentPWM = Config::_LightPWM;
  DiskLCD::Brightness(currentPWM);
}

void Reset()
{
  // go to full brightness
  currentPWM = 255;
  DiskLCD::Brightness(currentPWM);
}

void Check()
{
  // check the ambient light and adjust the LCD brightness
  int ldr = analogRead(PIN_LDR);
  byte newPWM = currentPWM;
  if (ldr <= Config::_DarkLDR)
    newPWM = Config::_DarkPWM;
  else if (ldr >= Config::_LightLDR)
    newPWM = Config::_LightPWM;
    
  if (newPWM != currentPWM)
  {
    DiskLCD::Brightness(newPWM);
    currentPWM = newPWM;
  }
}

int Read()
{
  // return a raw LDR
  return analogRead(PIN_LDR);
}

}
