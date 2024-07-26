#pragma once

// hPa+Celcius vs inHg+Fahrenheit
#define DIAL_METRIC 

// These are the defaults for the members below.
#define DARK_LDR  100
#define DARK_PWM   16
#define LIGHT_LDR 500
#define LIGHT_PWM 255
// NOTE: 
// LDR values are 0..950 in steps of 50
// PWM values are 0=LCD off ... 255=LCD max brightness (powers of 2, or 255)

// Define to show config screen. Otherwise LDR/PWM values are from the defines above.
#define CONFIG_EDITABLE

// If defined, serial on, no splash etc
//#define DEBUG
#define DBG(_x) { Serial.print(#_x);Serial.print(":");Serial.println(_x); }

// If defined (and DEBUG), hard-wired values for needles
//#define DEMO

namespace Config
{
  void Load();
  void Reset();
  void Edit();
// if LDR reading <= _DarkLDR, set brightness to _DarkPWM
// if LDR reading >= _LightLDR, set brightness to _LightPWM  
// otherwise unchanged
// 0        DARK               LIGHT      1023
// | dark   |     no change    |       light |
  extern int _DarkLDR, _DarkPWM, _LightLDR, _LightPWM;
};
