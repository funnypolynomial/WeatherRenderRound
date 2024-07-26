#include <Arduino.h>
#include <SPI.h>
#include "Pins.h"
#include "DiskLCD.h"
#include "Config.h"

#ifdef SERIALIZE
#define SERIALISE_COMMENT(_c) if (_serialise) { Serial.print("; ");Serial.println(_c);}
#define SERIALISE_INIT(_w,_h,_s) if (_serialise) { Serial.print(_w);Serial.print(',');Serial.print(_h);Serial.print(',');Serial.println(_s);}
#define SERIALISE_BEGINFILL(_x,_y,_w,_h) if (_serialise) { Serial.print(_x);Serial.print(',');Serial.print(_y);Serial.print(',');Serial.print(_w);Serial.print(',');Serial.println(_h);}
#define SERIALISE_FILLCOLOUR(_len,_colour) if (_serialise) { Serial.print(_len);Serial.print(',');Serial.println(_colour);}
#define SERIALISE_FILLBYTE(_len,_colour) if (_serialise) { Serial.print(_len);Serial.print(',');Serial.println(_colour?0xFFFF:0x0000);}
#else
#define SERIALISE_COMMENT(_c)
#define SERIALISE_INIT(_w,_h,_s)
#define SERIALISE_BEGINFILL(_x,_y,_w,_h)
#define SERIALISE_FILLCOLOUR(_len,_colour)
#define SERIALISE_FILLBYTE(_len,_colour)
#endif

namespace DiskLCD
{
bool _serialise = true;
// Use CS/DC/RES pin registers (otherwise pin numbers)
#define LCD_USE_REGISTERS

// Connector Position.  With pixel[0, 0] at top-left, used in pInitialisation below
#define ORIENTATION_LEFT   0xC8
#define ORIENTATION_RIGHT  0x28
#define ORIENTATION_TOP    0x18
#define ORIENTATION_BOTTOM 0x48
#define LCD_ORIENTATION    ORIENTATION_BOTTOM

// Colours, used in pInitialisation below
#define COLOR_12_BIT  0x03
#define COLOR_16_BIT  0x05
#define COLOR_18_BIT  0x06

// Command codes:
#define COL_ADDR_SET        0x2A
#define ROW_ADDR_SET        0x2B
#define MEM_WR              0x2C

#ifdef LCD_USE_REGISTERS
// Direct register access, for speed
// CS Pin A0
#define CS_HI PORTF |=   0b10000000
#define CS_LO PORTF &=  ~0b10000000

// DC Pin 9
#define DC_HI PORTB |=   0b01000000
#define DC_LO PORTB &=  ~0b01000000

// RES Pin 11
#define RES_HI PORTB |=  0b10000000
#define RES_LO PORTB &= ~0b10000000
#else ////////////////
// Use pin assignments
// CS Pin
#define CS_HI digitalWrite(PIN_CS, HIGH)
#define CS_LO digitalWrite(PIN_CS, LOW)

// DC Pin
#define DC_HI digitalWrite(PIN_DC, HIGH)
#define DC_LO digitalWrite(PIN_DC, LOW)

// RES Pin
#define RES_HI digitalWrite(PIN_RES, HIGH)
#define RES_LO digitalWrite(PIN_RES, LOW)
#endif

#define COMMAND(_cmd, _len) _cmd, _len
#define DELAY(_delay) 0x00, _delay
static const byte pInitialisation[] PROGMEM =
{
  COMMAND(0xEF, 0x00),   
  COMMAND(0xEB, 0x01),   0x14, 
  COMMAND(0xFE, 0x00),   
  COMMAND(0xEF, 0x00),   
  COMMAND(0xEB, 0x01),   0x14, 
  COMMAND(0x84, 0x01),   0x40, 
  COMMAND(0x85, 0x01),   0xFF, 
  COMMAND(0x86, 0x01),   0xFF, 
  COMMAND(0x87, 0x01),   0xFF, 
  COMMAND(0x88, 0x01),   0x0A, 
  COMMAND(0x89, 0x01),   0x21, 
  COMMAND(0x8A, 0x01),   0x00, 
  COMMAND(0x8B, 0x01),   0x80, 
  COMMAND(0x8C, 0x01),   0x01, 
  COMMAND(0x8D, 0x01),   0x01, 
  COMMAND(0x8E, 0x01),   0xFF, 
  COMMAND(0x8F, 0x01),   0xFF, 
  COMMAND(0xB6, 0x02),   0x00, 0x00, 
  
  COMMAND(0x3A, 0x01),   COLOR_16_BIT, 
  COMMAND(0x36, 0x01),   LCD_ORIENTATION,
  
  COMMAND(0x90, 0x04),   0x08, 0x08, 0x08, 0x08, 
  COMMAND(0xBD, 0x01),   0x06, 
  COMMAND(0xBC, 0x01),   0x00, 
  COMMAND(0xFF, 0x03),   0x60, 0x01, 0x04, 
  COMMAND(0xC3, 0x01),   0x13, 
  COMMAND(0xC4, 0x01),   0x13, 
  COMMAND(0xC9, 0x01),   0x22, 
  COMMAND(0xBE, 0x01),   0x11, 
  COMMAND(0xE1, 0x02),   0x10, 0x0E, 
  COMMAND(0xDF, 0x03),   0x21, 0x0c, 0x02, 
  COMMAND(0xF0, 0x06),   0x45, 0x09, 0x08, 0x08, 0x26, 0x2A, 
  COMMAND(0xF1, 0x06),   0x43, 0x70, 0x72, 0x36, 0x37, 0x6F, 
  COMMAND(0xF2, 0x06),   0x45, 0x09, 0x08, 0x08, 0x26, 0x2A, 
  COMMAND(0xF3, 0x06),   0x43, 0x70, 0x72, 0x36, 0x37, 0x6F, 
  COMMAND(0xED, 0x02),   0x1B, 0x0B, 
  COMMAND(0xAE, 0x01),   0x77, 
  COMMAND(0xCD, 0x01),   0x63, 
  COMMAND(0x70, 0x09),   0x07, 0x07, 0x04, 0x0E, 0x0F, 0x09, 0x07, 0x08, 0x03, 
  COMMAND(0xE8, 0x01),   0x34, 
  COMMAND(0x62, 0x0C),   0x18, 0x0D, 0x71, 0xED, 0x70, 0x70, 0x18, 0x0F, 0x71, 0xEF, 0x70, 0x70, 
  COMMAND(0x63, 0x0C),   0x18, 0x11, 0x71, 0xF1, 0x70, 0x70, 0x18, 0x13, 0x71, 0xF3, 0x70, 0x70, 
  COMMAND(0x64, 0x07),   0x28, 0x29, 0xF1, 0x01, 0xF1, 0x00, 0x07, 
  COMMAND(0x66, 0x0A),   0x3C, 0x00, 0xCD, 0x67, 0x45, 0x45, 0x10, 0x00, 0x00, 0x00, 
  COMMAND(0x67, 0x0A),   0x00, 0x3C, 0x00, 0x00, 0x00, 0x01, 0x54, 0x10, 0x32, 0x98, 
  COMMAND(0x74, 0x07),   0x10, 0x85, 0x80, 0x00, 0x00, 0x4E, 0x00, 
  COMMAND(0x98, 0x02),   0x3e, 0x07, 
  COMMAND(0x35, 0x00),   
  COMMAND(0x21, 0x00),   
  COMMAND(0x11, 0x00),   
  DELAY(120),
  DELAY(0),
};

void Init() 
{
  pinMode(PIN_RES, OUTPUT);
  pinMode(PIN_DC, OUTPUT);
  pinMode(PIN_CS, OUTPUT);  
  pinMode(PIN_BLK, OUTPUT);  
  digitalWrite(PIN_BLK, HIGH);
  SPI.begin();

  CS_HI;
  delay(5);
  RES_LO;
  delay(10);
  RES_HI;
  delay(120);

  CS_LO;
  const byte* pInit = pInitialisation;
  while (true)
  {
    byte cmd = pgm_read_byte(pInit++);
    byte len = pgm_read_byte(pInit++);
    if (cmd)
    {
      DC_LO;
      SPI.transfer(cmd);
      DC_HI;
      while (len--)
        SPI.transfer(pgm_read_byte(pInit++));
    }
    else if (len)
    {
      delay(len);
    }
    else
      break;
  }
  SERIALISE_COMMENT("*** START");
  SERIALISE_INIT(Diameter, Diameter, 1);  
}

void On(bool on)
{
  DC_LO;
  SPI.transfer(on?0x29:0x28);
}

void Brightness(byte pwm)
{
  analogWrite(PIN_BLK, pwm);
}

unsigned long BeginFill(int x, int y, int w, int h)
{
  SERIALISE_BEGINFILL(x, y, w, h);  
  int x2 = x + w - 1;
  int y2 = y + h - 1;
  
  // column range
  DC_LO;
  SPI.transfer(COL_ADDR_SET);  
  DC_HI;
  SPI.transfer16(x);
  SPI.transfer16(x2);
  
  // row range
  DC_LO;
  SPI.transfer(ROW_ADDR_SET);  
  DC_HI;
  SPI.transfer16(y);
  SPI.transfer16(y2);

  // start writing data
  DC_LO;
  SPI.transfer(MEM_WR);    
  unsigned long count = w;
  count *= h;
  return count;  
}

void FillColour(unsigned long size, word colour)
{
  SERIALISE_FILLCOLOUR(size, colour);  
  DC_HI;
  while (size--)
    SPI.transfer16(colour);
}

}
