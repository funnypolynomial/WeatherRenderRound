#include <Arduino.h>
#include <EEPROM.h>
#include "Config.h"
#include "BTN.h"
#include "LDR.h"
#include "Dial.h"
#include "DiskLCD.h"
#include "Display.h"

namespace Config
{
  const int LDR_MIN = 0;
  const int LDR_MAX = 950;
  const int LDR_DIVISOR = 50;

  int _DarkLDR = DARK_LDR, _DarkPWM = DARK_PWM, _LightLDR = LIGHT_LDR, _LightPWM = LIGHT_PWM;
  
  void Load()
  {
    // init LDR/PWM values to defaults then load them from EEPROM if the 'W' signature is there
    _DarkLDR = DARK_LDR;
    _DarkPWM = DARK_PWM;
    _LightLDR = LIGHT_LDR;
    _LightPWM = LIGHT_PWM;
    int idx = 0;
    if (EEPROM.read(idx++) == 'W')
    {
      _DarkLDR = EEPROM.read(idx++)*LDR_DIVISOR;
      _DarkPWM = EEPROM.read(idx++);
      _LightLDR = EEPROM.read(idx++)*LDR_DIVISOR;
      _LightPWM = EEPROM.read(idx++);
    }
  }
  
  void Reset()
  {
    // restore the settings to defaults.  Needs to be followed by a Read() to take effect
    EEPROM.write(0, '\x00');  // effectively erase the EEPROM settings
  }

  void DrawLine(int& line, const char* prompt, int value, bool blinkOn, int field)
  {
    // draws prompt:value at the given line, increments line. If blinkOn, black on white, else white on white
    // if value is -1, shown as " OFF"
    int Size = 3;
    int Row = DiskLCD::Diameter/3 + (line+1)*Size*(Display::FONT_SIZE + 1);
    int Col = DiskLCD::Diameter/2 - strlen(prompt)*Size*(Display::FONT_SIZE + 1);
    Display::DrawChar(Col - 1, Row, line == field?'>':' ', 0x0000, 0xFFFF, Size); // cursor for feedback on long press
    Display::DrawStr(-Col, -Row, prompt, 0x0000, 0xFFFF, Size);
    word fg = (blinkOn || line != field)?0x0000:0xFFFF;
    Col = DiskLCD::Diameter/2;
    if (value == -1)
      Display::DrawStr(-Col, -Row, " OFF", fg, 0xFFFF, Size);
    else
    {
      char buff[8];
      Display::DrawStr(-Col, -Row, Display::FormatNNNN(value, buff), fg, 0xFFFF, Size);
    }
    line++;
  }
  
  enum Fields {eDarkLDR, eDarkPWM, eLightLDR, eLightPWM,  eCurrentLDR}; // last is not an editable field
  void Edit()
  {
    // edit the fields
    DiskLCD::FillColour(DiskLCD::BeginFill(0, 0,  DiskLCD::Diameter,  DiskLCD::Diameter), 0xFFFF);
    LDR::Reset();
    bool save = true, blinkOn = true, update = true, longPress = false;
    int field = eDarkLDR;
    int DarkLDR = _DarkLDR, DarkPWM = _DarkPWM, LightLDR = _LightLDR, LightPWM = _LightPWM;
    unsigned long blinkTimeMS = millis();
    unsigned long idleTimeMS = blinkTimeMS;
    
    while (true)
    {
      if (update)
      {
        // draw fields, use "LO/HI" vs "DARK/LIGHT" to fit
        int line = 0;
        DrawLine(line, "LO LDR",  DarkLDR,              blinkOn, field);
        DrawLine(line, "LO PWM",  DarkPWM?DarkPWM:-1,   blinkOn, field);
        DrawLine(line, "HI LDR",  LightLDR,             blinkOn, field);
        DrawLine(line, "HI PWM",  LightPWM?LightPWM:-1, blinkOn, field);
        DrawLine(line, "LDR",     LDR::Read(),          true,    field);
        byte brightness = 255;
        if (field == eDarkPWM)
          brightness = DarkPWM;
        else if (field == eLightPWM)
          brightness = LightPWM;
        if (brightness == 0)
          brightness = 255;
        DiskLCD::Brightness(brightness);
        update = false;
      }
      
      if (btn.CheckButtonPressed(longPress, 2000UL))
      {
        if (longPress) // long press -- next field
        {
          idleTimeMS = blinkTimeMS = millis();
          update = blinkOn = true;
          field++;
          if (field == eCurrentLDR)
          {
            if (DarkLDR > LightLDR)
              field = eDarkLDR; // reject it
            else
            {
              save = true;
              break;
            }
          }
        }
        else // next value
        {
          switch (field)
          {
            case eDarkLDR:
              DarkLDR = (DarkLDR > LDR_MAX)?LDR_MIN:DarkLDR + LDR_DIVISOR;
              break;
            case eDarkPWM:
              if (DarkPWM == 128)
                DarkPWM = 255;
              else if (DarkPWM == 0)
                DarkPWM = 1;
              else
                DarkPWM = (DarkPWM == 255)?0:DarkPWM << 1;
              break;
            case eLightLDR:
              LightLDR = (LightLDR >= LDR_MAX)?LDR_MIN:(LightLDR + LDR_DIVISOR);
              break;
            case eLightPWM:
              if (LightPWM == 128)
                LightPWM = 255;
              else if (LightPWM == 0)
                LightPWM = 1;
              else
                LightPWM = (LightPWM == 255)?0:LightPWM << 1;
              break;
            default:
              break;
          }
          idleTimeMS = blinkTimeMS = millis();
          blinkOn = update = true;
        }
      }
      else
      {
        unsigned long nowMS = millis();
        if ((nowMS - blinkTimeMS) > 500L) // half second blink
        {
          blinkTimeMS = millis();
          blinkOn = !blinkOn;
          update = true;
        }
        if ((nowMS - idleTimeMS) > 30000L)  // idle 30s, bail out
        {
          save = false;
          break;
        }
      }
    }
  
    if (save)
    {
      _DarkLDR = DarkLDR;
      _DarkPWM = DarkPWM;
      _LightLDR = LightLDR;
      _LightPWM = LightPWM;
      int idx = 0;
      EEPROM.write(idx++, 'W');
      EEPROM.write(idx++, _DarkLDR / LDR_DIVISOR);
      EEPROM.write(idx++, _DarkPWM);
      EEPROM.write(idx++, _LightLDR / LDR_DIVISOR);
      EEPROM.write(idx++, _LightPWM);
    }
    LDR::Reset();
    Dial::Draw();
  }
}
