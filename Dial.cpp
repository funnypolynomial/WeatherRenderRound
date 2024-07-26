#include <Arduino.h>
#include "Config.h"
#include "Dial.h"
#include "DiskLCD.h"

namespace Dial
{
// see #includes at bottom of file
extern const uint8_t pDialData[]; 
extern const uint16_t pDialOffsetData[];


// Dial image data
// each row is <number of bytes of pixel data> <pixel offset to first on disk> <dat1> <data2>...
// data is 
//  0b0nnnnnnn for n background (white) pix
// or
//  0b1aaa1bbb or 0b1aaa0000, encode 2 or 1 pix, a, b, 000=full black, 111=lightest
// trailing backgound omitted

#define BACKGROUND 0xFFFF
void Draw()
{
  /// draw the entire dial
  const byte* pData = pDialData;
  for (int row = 0; row < DiskLCD::Diameter; row++)
  {
    byte bytes = pgm_read_byte(pData++);
    byte skip  = pgm_read_byte(pData++);
    byte pix = 0;
    DiskLCD::BeginFill(skip, row, DiskLCD::Diameter - 2*skip,  1);
    while (bytes)
    {
      byte data = pgm_read_byte(pData++);
      if (data & 0x80)
      {
        byte c = 32*((data & 0b01110000) >> 4);
        DiskLCD::FillColour(1,  RGB(c, c, c));
        pix++;
        if (data & 0x08)
        {
          byte c = 32*(data & 0b00000111);
          DiskLCD::FillColour(1,  RGB(c, c, c));
          pix++;
        }
      }
      else
      {
        DiskLCD::FillColour(data, BACKGROUND);
        pix += data;
      }
      bytes--;
    }
    DiskLCD::FillColour((DiskLCD::Diameter - 2*skip) - pix, BACKGROUND); // fill to end
  }
}

void StartRead()
{
  // there is no read cache to be init'd
}

word Read(int x, int y)
{
  // lookup colour at x, y
  const byte* pData = pDialData;
  // skip to the row
  int row = y;
  pData += pgm_read_word(pDialOffsetData + row);
  byte bytes = pgm_read_byte(pData++);
  byte col = pgm_read_byte(pData++);;
  
  // read the pixel data
  while (bytes)
  {
    byte data = pgm_read_byte(pData++);
    if (data & 0x80)
    {
      if (col == x)
      {
        byte c = 32*((data & 0b01110000) >> 4);
        return RGB(c, c, c);
      }
      col++;
      if (data & 0x08)
      {
        if (col == x)
        {
          byte c = 32*(data & 0b00000111);
          return RGB(c, c, c);
        }
        col++;
      }
    }
    else
    {
      if (x < col + data)
        return BACKGROUND;
      else
        col += data;
    }
    bytes--;
  }
  return BACKGROUND;
}


#ifdef DIAL_METRIC
// The hecto-Pascals & Celsius dial data
#include "DialData_hPa.h"
#else
// The inches of Mercury & Fahrenheit dial data
#include "DialData_inHg.h"
#endif
};
