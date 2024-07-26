#include <Arduino.h>
#include <SPI.h>
// --- External Libraries:
#include <pocketBME280.h>
// ----------------------- 
#include "Config.h"
#include "Sensor.h"
#include "Pins.h"
#include "BTN.h"
#include "LDR.h"
#include "DiskLCD.h"
#include "Needle.h"
#include "Dial.h"
#include "Display.h"

//   W e a t h e r R e n d e r R o u n d
//
// Displays the pressure, pressure trend, humidity and temperature, as dials and needles, on a round LCD (GC9a01 1.28in)
//
// The dial was created with Python code running as an Extension in Inkscape v1.3.2 and exported as a PNG with +1 anti-alias.
// A second Python script run-length-encoded the images as a PROGMEM array of bytes in the sketch. 
// See the resources sub-dir.
// The needles are drawn over the decompressed dial image using a fixed-point (uint16_t) anti-aliased line algorithm (with optional thickness).
// The display is not pixel-perfect! All the graphics are done from scratch, pocketBME280 is the only library.
//
// Measurements of pressure, humidity and temperature come from a BME280 using the pocketBME280 Library.
// Readings are taken every 5 minutes, the pressure trend needle (shorter, "copper/brass") shows the pressure 3 hours ago.
// Tapping the button while running resets the pressure trend to the current pressure.
//
//
// Config.h defines some configuration:
// The DIAL_METRIC define enables hPa and degrees C.  If undefined, an inHg and Fahrenheit dial is used.
// The LCD_ORIENTATION define in DiskLCD.cpp controls the display orientation.
// The default day/night (hi/lo) display brightness is set according to the LDR & PWM defines.
// If CONFIG_EDITABLE is defined, pressing-and-holding the button while running enters config mode to set the LDR & PWM values.
//   In the config screen:
//   Tapping the button increments the current field. 
//   Pressing-and-holding advances to the next field, or saves on the last field.
//   LDR values are 0, 50, 100,...950
//   PWM values are 0, 1, 2,... 255
//   The current LDR reading is shown in addition to the LDR/PWM fields.
// If CONFIG_EDITABLE is not defined, the compile-time LDR/PWM values are used. Saves a little over 2k or program space.
// If the LDR is less than the LO value, the LCD's brightness is set to the LO PWM.
// If the LDR is greater than the HI value, the LCD's brightness is set to the HI PWM.
// Otherwise it is unchanged.
// 
//
// Holding the button while turning on resets the LDR & PWM values. (if CONFIG_EDITABLE is defined)
// Holding the button to the end of the splash sets the trend needle to 7hPa below the initial reading, for photos.
//
// Mark Wilson, July 2024


const uint32_t UPDATE_IMTERVAL_MINS = 5UL; // take a reading every 5 minutes
const uint32_t UPDATE_IMTERVAL_MS = UPDATE_IMTERVAL_MINS*1000UL*60UL;
const uint32_t TREND_IMTERVAL_MINS = 3UL*60UL; // the pressure trend is from 3 hours ago
const int TREND_BUFFER_SIZE = TREND_IMTERVAL_MINS*60UL*1000UL/UPDATE_IMTERVAL_MS;
uint32_t updateTimerMS;
int trendReadingIdx = 0;
int trendReadings[TREND_BUFFER_SIZE]; // the history of readings
bool FirstReading = true;
bool NoReading = false; // unable to read sensor
bool Photogenic = false;

void setup() 
{
#ifdef DEBUG  
  Serial.begin(38400);
  // Serial output seems unreliable on the Leo Tiny
  delay(5000);
  Serial.println("; WeatherRenderRound");
#endif
  btn.Init(PIN_BTN);
#ifdef CONFIG_EDITABLE
  bool btnPressed = btn.IsDown();
#endif  
  LDR::Init();
  Sensor::Init();
  DiskLCD::Init();
  DiskLCD::FillColour(DiskLCD::BeginFill(0, 0,  DiskLCD::Diameter,  DiskLCD::Diameter), 0x0000);
  DiskLCD::On(true);
#ifdef CONFIG_EDITABLE
  if (btnPressed)
  {
    Display::DrawStr(DiskLCD::Diameter/2,  DiskLCD::Diameter/2, "RESET", 0xFFFF, 0x0000, 3);
    Config::Reset(); // then load...
    delay(1000);
    Display::DrawStr(DiskLCD::Diameter/2,  DiskLCD::Diameter/2, "RESET", 0x0000, 0x0000, 3);
  }
  Config::Load();
#endif  
#ifndef DEBUG
  Needle::Splash();
  Photogenic = btn.IsDown();  // suitable for photo, no need to wait 3 hours
#endif  
  Dial::Draw();
  Display::DrawSig(false);
  Needle::Init();
  updateTimerMS = millis();
}

void loop() 
{
  LDR::Check();
  bool held;
  if (btn.CheckButtonPressed(held, 3000UL))
  {
#ifdef CONFIG_EDITABLE
    if (held)
      Config::Edit();
#endif      
    FirstReading = true;  // reset the trend to <now>
  }
  
  unsigned long int nowMS = millis();
  if ((nowMS - updateTimerMS) > UPDATE_IMTERVAL_MS || FirstReading)
  {
    // Take a reading
    updateTimerMS = nowMS;
    int pressurehPa, temperatureC, humidityPercent;
    if (Sensor::Read(pressurehPa, temperatureC, humidityPercent))
    {
      if (FirstReading)
        for (int i = 0; i < TREND_BUFFER_SIZE; i++)
          trendReadings[i] = pressurehPa;  // no trend yet, use the first reading
      int trendPressure = trendReadings[trendReadingIdx];
      trendReadings[trendReadingIdx++] = pressurehPa;
      if (trendReadingIdx >= TREND_BUFFER_SIZE)
        trendReadingIdx = 0;  // rap around
      if (Photogenic)
        trendPressure = pressurehPa - 7;
#if defined(DEMO) && defined(DEBUG)
      trendPressure = 1008;
#endif        
      Display::UpdateNeedles(pressurehPa, trendPressure, temperatureC, humidityPercent);
      if (NoReading || FirstReading)
        Display::DrawSig(false);  // was an error, clear it
      FirstReading = NoReading = false;
    }
    else if (!NoReading)
    {
      Display::DrawSig(true);  // indicate read error, reddish "MEW MMXXIV"
      NoReading = true;
    }
  }
}
