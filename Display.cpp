#include <Arduino.h>
#include "Config.h"
#include "BTN.h"
#include "LDR.h"
#include "Dial.h"
#include "Sensor.h"
#include "DiskLCD.h"
#include "Display.h"
#include "Needle.h"

namespace Display
{
  // Numbers from Barometer.py, 100th's of a degree:
  const int baseAngleCentiDeg = -13500;
  const int baseSpanCentiDeg = 2 * 13500;
  // Constants in methods below match images

#ifdef DIAL_METRIC
  int PressureToAngle(int hPa)
  {
    // Angle for hecto-Pascals
    const int markDeltaDeciDeg = 450;  // 1hPa = 4.5 deg
    const int extraMarks = 5; // pressure goes a little past the +/-135 degree mark
    const int basePressure = 980;
    hPa = constrain(hPa, basePressure - extraMarks, basePressure + baseSpanCentiDeg / markDeltaDeciDeg + extraMarks);
    return Sensor::Round(baseAngleCentiDeg + markDeltaDeciDeg * (hPa - basePressure), 100);
  }

  int TemperatureToAngle(int TemperatureC)
  {
    // Angle for Celcius
    const int baseTemperatureC = -20;
    const int maxTemperatureC = 50;
    const int markDeltaDeciDeg = baseSpanCentiDeg / (maxTemperatureC - baseTemperatureC);
    TemperatureC = constrain(TemperatureC, baseTemperatureC, maxTemperatureC);
    return Sensor::Round(baseAngleCentiDeg + markDeltaDeciDeg * (TemperatureC - baseTemperatureC), 100);
  }
#else
  int PressureToAngle(int hPa)
  {
    // Angle for inches of Mercury
    // 1 hPa = 0.029529980164712 inHg
    const int markDeltaDeciDeg = 900;  // 1 deci-inHg = 9 deg
    const int basePressureDeciInchHg = 285;
    int deciInchHg = Sensor::Round(hPa*2952L, 10000);
    deciInchHg = constrain(deciInchHg, basePressureDeciInchHg, basePressureDeciInchHg + baseSpanCentiDeg / markDeltaDeciDeg);
    return Sensor::Round(baseAngleCentiDeg + markDeltaDeciDeg * (deciInchHg - basePressureDeciInchHg), 100);
  }

  int TemperatureToAngle(int TemperatureC)
  {
    // Angle for Fahrenheit
    const int baseTemperatureF = 0;
    const int maxTemperatureF = 120;
    const int markDeltaDeciDeg = baseSpanCentiDeg / (maxTemperatureF - baseTemperatureF);
    int TemperatureF = 32 + Sensor::Round(9*TemperatureC, 5);
    TemperatureF = constrain(TemperatureF, baseTemperatureF, maxTemperatureF);
    return Sensor::Round(baseAngleCentiDeg + markDeltaDeciDeg * (TemperatureF - baseTemperatureF), 100);
  }
#endif

  int HumidityToAngle(int Percent)
  {
    // Angle for percentage
    const int basePercent = 0;
    const int maxPercent = 100;
    const int markDeltaDeciDeg = baseSpanCentiDeg / (maxPercent - basePercent);
    Percent = constrain(Percent, basePercent, maxPercent);
    return Sensor::Round(baseAngleCentiDeg + markDeltaDeciDeg * (Percent - basePercent), 100);
  }

  int PressureHPACurrent = NULL_READING, PressureTrendHPACurrent = NULL_READING, TemperatureCCurrent = NULL_READING, HumidityPCCurrent = NULL_READING;
  void UpdateNeedles(int PressureHPA, int PressureTrendHPA, int TemperatureC, int HumidityPC)
  {
    if (PressureHPACurrent != PressureHPA && PressureHPACurrent != NULL_READING)
      Needle::ErasePressure();
    if (PressureTrendHPACurrent != PressureTrendHPA && PressureTrendHPACurrent != NULL_READING)
      Needle::ErasePressureTrend();
    if (TemperatureCCurrent != TemperatureC && TemperatureCCurrent != NULL_READING)
      Needle::EraseTemperature();
    if (HumidityPCCurrent != HumidityPC && HumidityPCCurrent != NULL_READING)
      Needle::EraseHumidity();
      
    PressureHPACurrent = PressureHPA;
    PressureTrendHPACurrent = PressureTrendHPA;
    TemperatureCCurrent = TemperatureC;
    HumidityPCCurrent = HumidityPC;

    Needle::DrawTemperature(TemperatureToAngle(TemperatureCCurrent));
    Needle::DrawHumidity(HumidityToAngle(HumidityPCCurrent));
    Needle::DrawPressure(PressureToAngle(PressureHPACurrent));
    Needle::DrawPressureTrend(PressureToAngle(PressureTrendHPACurrent));
  }

  extern const uint32_t font5x5[];  // font table is at the bottom of the file
  void DrawChar(int x, int y, char ch, word fg, word bg, int size)
  {
    // draw ch at x,y, using fg/bg colours, if found in font5x5
    const uint32_t* pData = font5x5;
    uint32_t data = 0;
    char dataChar;
    do
      if ((data = pgm_read_dword(pData++)))
      {
        if (ch == (dataChar = (char)(data >> FONT_SIZE*FONT_SIZE)))
          break;  // match
        else if (ch < dataChar)
          data = 0; // not in table
      }
    while (data);
    int ctr = FONT_SIZE*FONT_SIZE;
    if (size == 1)
      DiskLCD::BeginFill(x, y, FONT_SIZE, FONT_SIZE);
    while (ctr--)
    {
      if (size == 1)
        DiskLCD::FillColour(1, (data & 1)?fg:bg);
      else
        DiskLCD::FillColour(DiskLCD::BeginFill(x + (FONT_SIZE - ctr % FONT_SIZE)*size, y - (ctr / FONT_SIZE)*size, size, size), (data & 1)?fg:bg);
      data >>= 1;
    }
  }
  
  void DrawStr(int x, int y, const char* str, word fg, word bg, int size)
  {
    // Draw str, centred at x,y, using fg/bg colours with cells of the given size
    // if x or y are negative, no centring, abs values
    int offs = (FONT_SIZE + 1)*size;
    if (x < 0)
      x = -x;
    else
      x -= strlen(str)*offs/2 + offs;
    if (y < 0)
      y = -y;
    else
      y += FONT_SIZE*size/2;
    while (*str)
      DrawChar(x += offs, y, *(str++), fg, bg, size);
  }

  void DrawSig(bool err)
  {
    // Show "MEW MMXXIV" redish if there's an error, otherwise pale grey
    word colour = err?RGB(160, 0, 0):RGB(160, 160, 160);
    DrawStr(DiskLCD::Diameter/2, DiskLCD::Diameter - 17, "MEW",    colour, RGB(255, 255, 255), 1);
    DrawStr(DiskLCD::Diameter/2, DiskLCD::Diameter - 11, "MMXXIV", colour, RGB(255, 255, 255), 1);
  }

char* FormatNNNN(int value, char* pStr, int MSD, char pad)
{
  // Format value into pStr, advancing it.  
  // MSD controls number of digits (100 = 3 digits).  Initial 0's padded with pad
  // Adds terminating NUL, returns buffer. Less code than using itoa()
  char* pBuff = pStr;
  while (MSD)
  {
    if (value < MSD && pad && !(value == 0 && MSD == 1))
      *pStr++ = pad;
    else
    {
      *pStr++ = '0' + (value / MSD);
      pad = 0;
    }
    value %= MSD;
    MSD /= 10;
  }
  *pStr = 0;
  return pBuff;
}  

// Tiny Font
// All ASCII chars are present, all but 0-9, A-Z and '>' are commented-out
// Font is 5x5, lsb is top-right pixel 
// 0b00000000000000000000000000000000
//   cccccccddddddddddddddddddddddddd
//   char           definition
//  (7 bits)        (25 bits)
// or a block entry, group of chars, a..b
//   0000000000000000bbbbbbbbaaaaaaaa
#define BLOCKY_CHARS // Block-style chars, vs (some) rounded corners
#define X 1 // always on
#ifdef BLOCKY_CHARS
#define Y 1 // on if blocky chars
#define Z 0 // on if non-blocky chars
#else
#define Y 0
#define Z 1
#endif
#define _ 0
// font row
#define ROW(a,b,c,d,e) (uint32_t)((a << 0) | (b << 1) | (c << 2) | (d << 3) | (e << 4))
// font char
#define CHAR(ch, a,b,c,d,e) (uint32_t)((a << 0) | (b << 5) | (c << 10) | (d << 15) | (e << 20) | (((uint32_t)ch) << 25))
// font block (of consequtive chars)
#define BLOCK(a, b) (uint32_t)((uint32_t)(a) | ((uint32_t)b) << 8)
static const uint32_t font5x5[] PROGMEM =
{
  // Alphabetical, but chars not present will be drawn blank
  // chars can be omitted or defined in blocks
//  CHAR(' ',
//     ROW(_,_,_,_,_),
//     ROW(_,_,_,_,_),
//     ROW(_,_,_,_,_),
//     ROW(_,_,_,_,_),
//     ROW(_,_,_,_,_)),

//  BLOCK('!', '/'),
//  CHAR('!',
//      ROW(_,_,X,_,_),
//      ROW(_,_,X,_,_),
//      ROW(_,_,X,_,_),
//      ROW(_,_,_,_,_),
//      ROW(_,_,X,_,_)),
//  CHAR('"',
//      ROW(_,X,_,X,_),
//      ROW(_,X,_,X,_),
//      ROW(_,_,_,_,_),
//      ROW(_,_,_,_,_),
//      ROW(_,_,_,_,_)),
//  CHAR('#',
//      ROW(_,X,_,X,_),
//      ROW(X,X,X,X,X),
//      ROW(_,X,_,X,_),
//      ROW(X,X,X,X,X),
//      ROW(_,X,_,X,_)),
//  CHAR('$',
//      ROW(Y,X,X,X,X),
//      ROW(X,_,X,_,_),
//      ROW(Y,X,X,X,Y),
//      ROW(_,_,X,_,X),
//      ROW(X,X,X,X,Y)),
//  CHAR('%',
//      ROW(X,X,_,_,X),
//      ROW(X,_,_,X,_),
//      ROW(_,_,X,_,_),
//      ROW(_,X,_,_,X),
//      ROW(X,_,_,X,X)),
//  CHAR('&',
//      ROW(_,X,_,_,_),
//      ROW(X,_,X,_,_),
//      ROW(_,X,X,_,X),
//      ROW(X,_,_,X,_),
//      ROW(_,X,X,_,X)),
//  CHAR('\'',
//      ROW(_,_,X,_,_),
//      ROW(_,_,X,_,_),
//      ROW(_,_,_,_,_),
//      ROW(_,_,_,_,_),
//      ROW(_,_,_,_,_)),
//  CHAR('(',
//      ROW(_,_,X,X,_),
//      ROW(_,X,_,_,_),
//      ROW(_,X,_,_,_),
//      ROW(_,X,_,_,_),
//      ROW(_,_,X,X,_)),
//  CHAR(')',
//      ROW(_,X,X,_,_),
//      ROW(_,_,_,X,_),
//      ROW(_,_,_,X,_),
//      ROW(_,_,_,X,_),
//      ROW(_,X,X,_,_)),
//  CHAR('*',
//      ROW(X,_,X,_,X),
//      ROW(_,X,X,X,_),
//      ROW(X,X,X,X,X),
//      ROW(_,X,X,X,_),
//      ROW(X,_,X,_,X)),
//  CHAR('+',
//      ROW(_,_,_,_,_),
//      ROW(_,_,X,_,_),
//      ROW(_,X,X,X,_),
//      ROW(_,_,X,_,_),
//      ROW(_,_,_,_,_)),
//  CHAR(',',
//      ROW(_,_,_,_,_),
//      ROW(_,_,_,_,_),
//      ROW(_,_,_,_,_),
//      ROW(_,_,X,_,_),
//      ROW(_,X,_,_,_)),
//  CHAR('-',
//      ROW(_,_,_,_,_),
//      ROW(_,_,_,_,_),
//      ROW(_,X,X,X,_),
//      ROW(_,_,_,_,_),
//      ROW(_,_,_,_,_)),
//  CHAR('.',
//      ROW(_,_,_,_,_),
//      ROW(_,_,_,_,_),
//      ROW(_,_,_,_,_),
//      ROW(_,_,X,_,_),
//      ROW(_,_,X,_,_)),
//  CHAR('/',
//      ROW(_,_,_,_,X),
//      ROW(_,_,_,X,_),
//      ROW(_,_,X,_,_),
//      ROW(_,X,_,_,_),
//      ROW(X,_,_,_,_)),

  BLOCK('0', '9'),
  CHAR('0',
     ROW(X,X,X,X,X),
     ROW(X,_,_,_,X),
     ROW(X,_,_,_,X),
     ROW(X,_,_,_,X),
     ROW(X,X,X,X,X)),
  CHAR('1',
     ROW(X,X,X,_,_),
     ROW(_,_,X,_,_),
     ROW(_,_,X,_,_),
     ROW(_,_,X,_,_),
     ROW(X,X,X,X,X)),
  CHAR('2',
     ROW(X,X,X,X,X),
     ROW(_,_,_,_,X),
     ROW(X,X,X,X,X),
     ROW(X,_,_,_,_),
     ROW(X,X,X,X,X)),
  CHAR('3',
     ROW(X,X,X,X,X),
     ROW(_,_,_,_,X),
     ROW(X,X,X,X,X),
     ROW(_,_,_,_,X),
     ROW(X,X,X,X,X)),
  CHAR('4',
     ROW(X,_,_,_,X),
     ROW(X,_,_,_,X),
     ROW(X,X,X,X,X),
     ROW(_,_,_,_,X),
     ROW(_,_,_,_,X)),
  CHAR('5',
     ROW(X,X,X,X,X),
     ROW(X,_,_,_,_),
     ROW(X,X,X,X,X),
     ROW(_,_,_,_,X),
     ROW(X,X,X,X,X)),
  CHAR('6',
     ROW(X,X,X,X,X),
     ROW(X,_,_,_,_),
     ROW(X,X,X,X,X),
     ROW(X,_,_,_,X),
     ROW(X,X,X,X,X)),
  CHAR('7',
     ROW(X,X,X,X,X),
     ROW(_,_,_,_,X),
     ROW(_,_,_,_,X),
     ROW(_,_,_,_,X),
     ROW(_,_,_,_,X)),
  CHAR('8',
     ROW(X,X,X,X,X),
     ROW(X,_,_,_,X),
     ROW(X,X,X,X,X),
     ROW(X,_,_,_,X),
     ROW(X,X,X,X,X)),
  CHAR('9',
     ROW(X,X,X,X,X),
     ROW(X,_,_,_,X),
     ROW(X,X,X,X,X),
     ROW(_,_,_,_,X),
     ROW(X,X,X,X,X)),

//  BLOCK(':', '@'),
//  CHAR(':',
//      ROW(_,_,_,_,_),
//      ROW(_,_,X,_,_),
//      ROW(_,_,_,_,_),
//      ROW(_,_,X,_,_),
//      ROW(_,_,_,_,_)),
//  CHAR(';',
//      ROW(_,_,_,_,_),
//      ROW(_,_,X,_,_),
//      ROW(_,_,_,_,_),
//      ROW(_,_,X,_,_),
//      ROW(_,X,_,_,_)),
//  CHAR('<',
//      ROW(_,_,_,X,_),
//      ROW(_,_,X,_,_),
//      ROW(_,X,_,_,_),
//      ROW(_,_,X,_,_),
//      ROW(_,_,_,X,_)),
//  CHAR('=',
//      ROW(_,_,_,_,_),
//      ROW(_,X,X,X,_),
//      ROW(_,_,_,_,_),
//      ROW(_,X,X,X,_),
//      ROW(_,_,_,_,_)),
  CHAR('>',   // <<<<<<<<<<<<<<<<< NOTE: used as cursor in configuration screen
      ROW(_,X,_,_,_),
      ROW(_,_,X,_,_),
      ROW(_,_,_,X,_),
      ROW(_,_,X,_,_),
      ROW(_,X,_,_,_)),
//  CHAR('?',
//      ROW(Y,X,X,X,Y),
//      ROW(X,_,_,_,X),
//      ROW(_,_,X,X,Y),
//      ROW(_,_,_,_,_),
//      ROW(_,_,X,_,_)),
//  CHAR('@',
//      ROW(Y,X,X,X,Y),
//      ROW(X,_,X,_,X),
//      ROW(X,_,X,X,X),
//      ROW(X,_,_,_,_),
//      ROW(Y,X,X,X,Y)),

  BLOCK('A', 'Z'),
  CHAR('A',
     ROW(Y,X,X,X,Y),
     ROW(X,_,_,_,X),
     ROW(X,X,X,X,X),
     ROW(X,_,_,_,X),
     ROW(X,_,_,_,X)),
  CHAR('B',
     ROW(X,X,X,X,Y),
     ROW(X,_,_,_,X),
     ROW(X,X,X,X,Y),
     ROW(X,_,_,_,X),
     ROW(X,X,X,X,Y)),
  CHAR('C',
     ROW(Y,X,X,X,Y),
     ROW(X,_,_,_,Z),
     ROW(X,_,_,_,_),
     ROW(X,_,_,_,Z),
     ROW(Y,X,X,X,Y)),
  CHAR('D',
     ROW(X,X,X,X,_),
     ROW(X,_,_,_,X),
     ROW(X,_,_,_,X),
     ROW(X,_,_,_,X),
     ROW(X,X,X,X,_)),
  CHAR('E',
     ROW(X,X,X,X,X),
     ROW(X,_,_,_,_),
     ROW(X,X,X,X,Y),
     ROW(X,_,_,_,_),
     ROW(X,X,X,X,X)),
  CHAR('F',
     ROW(X,X,X,X,X),
     ROW(X,_,_,_,_),
     ROW(X,X,X,X,Y),
     ROW(X,_,_,_,_),
     ROW(X,_,_,_,_)),
  CHAR('G',
     ROW(_,X,X,X,X),
     ROW(X,_,_,_,_),
     ROW(X,_,_,X,X),
     ROW(X,_,_,_,X),
     ROW(_,X,X,X,_)),
  CHAR('H',
     ROW(X,_,_,_,X),
     ROW(X,_,_,_,X),
     ROW(X,X,X,X,X),
     ROW(X,_,_,_,X),
     ROW(X,_,_,_,X)),
  CHAR('I',
     ROW(X,X,X,X,X),
     ROW(_,_,X,_,_),
     ROW(_,_,X,_,_),
     ROW(_,_,X,_,_),
     ROW(X,X,X,X,X)),
  CHAR('J',
     ROW(X,X,X,X,X),
     ROW(_,_,X,_,_),
     ROW(_,_,X,_,_),
     ROW(_,_,X,_,_),
     ROW(X,X,_,_,_)),
  CHAR('K',
     ROW(X,_,_,_,X),
     ROW(X,_,_,X,_),
     ROW(X,X,X,_,_),
     ROW(X,_,_,X,_),
     ROW(X,_,_,_,X)),
  CHAR('L',
     ROW(X,_,_,_,_),
     ROW(X,_,_,_,_),
     ROW(X,_,_,_,_),
     ROW(X,_,_,_,_),
     ROW(X,X,X,X,X)),
  CHAR('M',
     ROW(Y,X,Y,X,Y),
     ROW(X,_,X,_,X),
     ROW(X,_,X,_,X),
     ROW(X,_,X,_,X),
     ROW(X,_,Y,_,X)),
  CHAR('N',
     ROW(X,_,_,_,X),
     ROW(X,X,_,_,X),
     ROW(X,_,X,_,X),
     ROW(X,_,_,X,X),
     ROW(X,_,_,_,X)),
  CHAR('O',
     ROW(Y,X,X,X,Y),
     ROW(X,_,_,_,X),
     ROW(X,_,_,_,X),
     ROW(X,_,_,_,X),
     ROW(Y,X,X,X,Y)),
  CHAR('P',
     ROW(X,X,X,X,Y),
     ROW(X,_,_,_,X),
     ROW(X,X,X,X,Y),
     ROW(X,_,_,_,_),
     ROW(X,_,_,_,_)),
  CHAR('Q',
     ROW(Y,X,X,X,Y),
     ROW(X,_,_,_,X),
     ROW(X,_,X,_,X),
     ROW(X,_,_,X,X),
     ROW(Y,X,X,X,X)),
  CHAR('R',
     ROW(X,X,X,X,Y),
     ROW(X,_,_,_,X),
     ROW(X,X,X,X,Y),
     ROW(X,_,_,X,_),
     ROW(X,_,_,_,X)),
  CHAR('S',
     ROW(Y,X,X,X,X),
     ROW(X,_,_,_,_),
     ROW(Y,X,X,X,Y),
     ROW(_,_,_,_,X),
     ROW(X,X,X,X,Y)),
  CHAR('T',
     ROW(X,X,X,X,X),
     ROW(_,_,X,_,_),
     ROW(_,_,X,_,_),
     ROW(_,_,X,_,_),
     ROW(_,_,X,_,_)),
  CHAR('U',
     ROW(X,_,_,_,X),
     ROW(X,_,_,_,X),
     ROW(X,_,_,_,X),
     ROW(X,_,_,_,X),
     ROW(Y,X,X,X,Y)),
  CHAR('V',
     ROW(X,_,_,_,X),
     ROW(X,_,_,_,X),
     ROW(X,_,_,_,X),
     ROW(_,X,_,X,_),
     ROW(_,_,X,_,_)),
  CHAR('W',
     ROW(X,_,Y,_,X),
     ROW(X,_,X,_,X),
     ROW(X,_,X,_,X),
     ROW(X,_,X,_,X),
     ROW(Y,X,Y,X,Y)),
  CHAR('X',
     ROW(X,_,_,_,X),
     ROW(_,X,_,X,_),
     ROW(_,_,X,_,_),
     ROW(_,X,_,X,_),
     ROW(X,_,_,_,X)),
  CHAR('Y',
     ROW(X,_,_,_,X),
     ROW(_,X,_,X,_),
     ROW(_,_,X,_,_),
     ROW(_,_,X,_,_),
     ROW(_,_,X,_,_)),
  CHAR('Z',
     ROW(X,X,X,X,X),
     ROW(_,_,_,X,_),
     ROW(_,_,X,_,_),
     ROW(_,X,_,_,_),
     ROW(X,X,X,X,X)),

//  BLOCK('[', '`'),
//  CHAR('[',
//      ROW(_,X,X,X,_),
//      ROW(_,X,_,_,_),
//      ROW(_,X,_,_,_),
//      ROW(_,X,_,_,_),
//      ROW(_,X,X,X,_)),
//  CHAR('\\',
//      ROW(X,_,_,_,_),
//      ROW(_,X,_,_,_),
//      ROW(_,_,X,_,_),
//      ROW(_,_,_,X,_),
//      ROW(_,_,_,_,X)),
//  CHAR(']',
//      ROW(_,X,X,X,_),
//      ROW(_,_,_,X,_),
//      ROW(_,_,_,X,_),
//      ROW(_,_,_,X,_),
//      ROW(_,X,X,X,_)),
//  CHAR('^',
//      ROW(_,_,X,_,_),
//      ROW(_,X,_,X,_),
//      ROW(_,_,_,_,_),
//      ROW(_,_,_,_,_),
//      ROW(_,_,_,_,_)),
//  CHAR('_',
//      ROW(_,_,_,_,_),
//      ROW(_,_,_,_,_),
//      ROW(_,_,_,_,_),
//      ROW(_,_,_,_,_),
//      ROW(X,X,X,X,X)),
//  CHAR('`',
//      ROW(_,_,X,_,_),
//      ROW(_,_,_,X,_),
//      ROW(_,_,_,_,_),
//      ROW(_,_,_,_,_),
//      ROW(_,_,_,_,_)),

//  BLOCK('a', 'z'),
//  CHAR('a',
//     ROW(_,_,_,_,_),
//     ROW(_,_,_,_,_),
//     ROW(Y,X,_,X,_),
//     ROW(X,_,X,_,_),
//     ROW(Y,X,_,X,_)),
//  CHAR('b',
//     ROW(X,_,_,_,_),
//     ROW(X,_,_,_,_),
//     ROW(X,X,X,Y,_),
//     ROW(X,_,_,X,_),
//     ROW(X,X,X,Y,_)),
//  CHAR('c',
//     ROW(_,_,_,_,_),
//     ROW(_,_,_,_,_),
//     ROW(Y,X,X,X,_),
//     ROW(X,_,_,_,_),
//     ROW(Y,X,X,X,_)),
//  CHAR('d',
//     ROW(_,_,_,X,_),
//     ROW(_,_,_,X,_),
//     ROW(Y,X,X,X,_),
//     ROW(X,_,_,X,_),
//     ROW(Y,X,X,X,_)),
//  CHAR('e',
//     ROW(_,_,_,_,_),
//     ROW(Y,X,X,Y,_),
//     ROW(X,X,X,X,_),
//     ROW(X,_,_,_,_),
//     ROW(Y,X,X,X,_)),
//  CHAR('f',
//     ROW(_,Y,X,_,_),
//     ROW(_,X,_,_,_),
//     ROW(X,X,X,_,_),
//     ROW(_,X,_,_,_),
//     ROW(_,X,_,_,_)),
//  CHAR('g',
//     ROW(_,_,_,_,_),
//     ROW(Y,X,X,Y,_),
//     ROW(X,X,X,X,_),
//     ROW(_,_,_,X,_),
//     ROW(X,X,X,Y,_)),
//  CHAR('h',
//     ROW(X,_,_,_,_),
//     ROW(X,_,_,_,_),
//     ROW(X,X,X,Y,_),
//     ROW(X,_,_,X,_),
//     ROW(X,_,_,X,_)),
//  CHAR('i',
//     ROW(_,X,_,_,_),
//     ROW(_,_,_,_,_),
//     ROW(_,X,_,_,_),
//     ROW(_,X,_,_,_),
//     ROW(_,X,_,_,_)),
//  CHAR('j',
//     ROW(_,X,_,_,_),
//     ROW(_,_,_,_,_),
//     ROW(_,X,_,_,_),
//     ROW(_,X,_,_,_),
//     ROW(X,_,_,_,_)),
//  CHAR('k',
//     ROW(X,_,_,_,_),
//     ROW(X,_,_,_,_),
//     ROW(X,_,_,X,_),
//     ROW(X,X,X,_,_),
//     ROW(X,_,_,X,_)),
//  CHAR('l',
//     ROW(_,X,_,_,_),
//     ROW(_,X,_,_,_),
//     ROW(_,X,_,_,_),
//     ROW(_,X,_,_,_),
//     ROW(_,X,_,_,_)),
//  CHAR('m',
//     ROW(_,_,_,_,_),
//     ROW(_,_,_,_,_),
//     ROW(Y,X,Y,X,Y),
//     ROW(X,_,X,_,X),
//     ROW(X,_,X,_,X)),
//  CHAR('n',
//     ROW(_,_,_,_,_),
//     ROW(_,_,_,_,_),
//     ROW(X,X,X,Y,_),
//     ROW(X,_,_,X,_),
//     ROW(X,_,_,X,_)),
//  CHAR('o',
//     ROW(_,_,_,_,_),
//     ROW(_,_,_,_,_),
//     ROW(Y,X,X,Y,_),
//     ROW(X,_,_,X,_),
//     ROW(Y,X,X,Y,_)),
//  CHAR('p',
//     ROW(_,_,_,_,_),
//     ROW(X,X,X,Y,_),
//     ROW(X,X,X,X,_),
//     ROW(X,_,_,_,_),
//     ROW(X,_,_,_,_)),
//  CHAR('q',
//     ROW(_,_,_,_,_),
//     ROW(Y,X,X,X,_),
//     ROW(X,X,X,X,_),
//     ROW(_,_,_,X,_),
//     ROW(_,_,_,X,X)),
//  CHAR('r',
//     ROW(_,_,_,_,_),
//     ROW(_,_,_,_,_),
//     ROW(X,X,X,X,_),
//     ROW(X,_,_,_,_),
//     ROW(X,_,_,_,_)),
//  CHAR('s',
//     ROW(_,_,_,_,_),
//     ROW(_,_,_,_,_),
//     ROW(_,X,X,X,_),
//     ROW(_,X,X,_,_),
//     ROW(X,X,X,_,_)),
//  CHAR('t',
//     ROW(_,X,_,_,_),
//     ROW(_,X,_,_,_),
//     ROW(X,X,X,_,_),
//     ROW(_,X,_,_,_),
//     ROW(_,Y,X,_,_)),
//  CHAR('u',
//     ROW(_,_,_,_,_),
//     ROW(_,_,_,_,_),
//     ROW(X,_,_,X,_),
//     ROW(X,_,_,X,_),
//     ROW(Y,X,X,X,_)),
//  CHAR('v',
//     ROW(_,_,_,_,_),
//     ROW(_,_,_,_,_),
//     ROW(X,_,_,X,_),
//     ROW(X,_,_,X,_),
//     ROW(_,X,X,_,_)),
//  CHAR('w',
//     ROW(_,_,_,_,_),
//     ROW(_,_,_,_,_),
//     ROW(X,_,X,_,X),
//     ROW(X,_,X,_,X),
//     ROW(Y,X,Y,X,Y)),
//  CHAR('x',
//     ROW(_,_,_,_,_),
//     ROW(_,_,_,_,_),
//     ROW(X,_,_,X,_),
//     ROW(_,X,X,_,_),
//     ROW(X,_,_,X,_)),
//  CHAR('y',
//     ROW(_,_,_,_,_),
//     ROW(X,_,_,X,_),
//     ROW(Y,X,X,X,_),
//     ROW(_,_,_,X,_),
//     ROW(X,X,X,Y,_)),
//  CHAR('z',
//     ROW(_,_,_,_,_),
//     ROW(_,_,_,_,_),
//     ROW(X,X,X,_,_),
//     ROW(_,X,X,_,_),
//     ROW(_,X,X,X,_)),

//  BLOCK('{', '~'),
//  CHAR('{',
//    ROW(_,_,Y,X,_),
//    ROW(_,_,X,_,_),
//    ROW(_,X,_,_,_),
//    ROW(_,_,X,_,_),
//    ROW(_,_,Y,X,_)),
//  CHAR('|',
//    ROW(_,_,X,_,_),
//    ROW(_,_,X,_,_),
//    ROW(_,_,X,_,_),
//    ROW(_,_,X,_,_),
//    ROW(_,_,X,_,_)),
//  CHAR('}',
//    ROW(_,X,Y,_,_),
//    ROW(_,_,X,_,_),
//    ROW(_,_,_,X,_),
//    ROW(_,_,X,_,_),
//    ROW(_,X,Y,_,_)),
//  CHAR('~',
//    ROW(_,X,_,_,X),
//    ROW(X,_,X,X,_),
//    ROW(_,_,_,_,_),
//    ROW(_,_,_,_,_),
//    ROW(_,_,_,_,_)),

  0
};

}
