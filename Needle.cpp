#include <Arduino.h>
#include "DiskLCD.h"
#include "Dial.h"
#include "Needle.h"
#include "Config.h"

namespace Needle
{
// ====== Needle constants ======
#define PRESSURE_NEEDLE_X         (DiskLCD::Diameter/2)
#define PRESSURE_NEEDLE_Y         (DiskLCD::Diameter/2)
#define PRESSURE_NEEDLE_LEN       (DiskLCD::Diameter/2 - 10)
#define PRESSURE_NEEDLE_TAIL      (20)
#define PRESSURE_TREND_NEEDLE_LEN (2*PRESSURE_NEEDLE_LEN/3)

// From Barometer.py:
#define SUBSIDIARY_X              ((DiskLCD::Diameter/2)/3 + 3)
#define SUBSIDIARY_Y              ((DiskLCD::Diameter/2)/3 - 6)

#define SUBSIDIARY_NEEDLE_X       ((DiskLCD::Diameter/2) + SUBSIDIARY_X)
#define SUBSIDIARY_NEEDLE_Y       ((DiskLCD::Diameter/2) + SUBSIDIARY_Y)
#define SUBSIDIARY_NEEDLE_LEN     ((DiskLCD::Diameter/2)/5)
#define SUBSIDIARY_NEEDLE_TAIL    10

#define TEMPERATURE_NEEDLE_X      ((DiskLCD::Diameter/2) + SUBSIDIARY_X)
#define HUMIDITY_NEEDLE_X         ((DiskLCD::Diameter/2) - SUBSIDIARY_X)


// ====== Fixed point calculations ======
#define FIXED_SCALE_BITS 8
#if FIXED_SCALE_BITS == 8
// 0xII.FF
typedef uint16_t tFixed;
typedef uint32_t tFixedTemp;
#elif FIXED_SCALE_BITS == 16
// 0xIIII.FFFF
typedef uint32_t tFixed;
typedef uint64_t tFixedTemp;
would need to replace (16-bit) ints
#else
not implemented!
#endif

// Integer part
#define FIXED_INT_PART(x)  ((x) >> FIXED_SCALE_BITS)
// Fractional part
#define FIXED_FRAC_PART(x) ((x) & (((tFixed)1 << FIXED_SCALE_BITS) - 1))
// Fixed-point from int
#define FIXED_FROM_INT(i)  ((i) << FIXED_SCALE_BITS)


// Needle pixel data to erase
// coord0, coord1, steep, thick;   data...
// data is x of pixel strip at each y, or vice-versa (based on steep, data[2])
#define NEEDLE_DATA_SIZE (4 + 2)  // +2 for data
uint8_t pPressurePixelData[NEEDLE_DATA_SIZE + PRESSURE_NEEDLE_LEN + PRESSURE_NEEDLE_TAIL];
uint8_t pPressureTrendPixelData[NEEDLE_DATA_SIZE + PRESSURE_TREND_NEEDLE_LEN];
uint8_t pHumidityPixelData[NEEDLE_DATA_SIZE + SUBSIDIARY_NEEDLE_LEN + SUBSIDIARY_NEEDLE_TAIL];
uint8_t pTemperaturePixelData[NEEDLE_DATA_SIZE + SUBSIDIARY_NEEDLE_LEN + SUBSIDIARY_NEEDLE_TAIL];

// Use this number of most significant bits from the anti-aliasing "brightness" to look up the colour
#define PALETTE_BITS 3
#define PALETTE_SIZE (1 << PALETTE_BITS)
// 0th element is strongest colour (brightest)
word pBlackPalette[PALETTE_SIZE];
word pTrendPalette[PALETTE_SIZE];

// convert Hue, Saturation & Value to Red, Green & Blue
// based on web.mit.edu/storborg/Public/hsvtorgb.c, refactored
// h=0..255 (0..360), s=0..255 (0..100), v=0..255 (0..100)
word HSVtoRGB(byte h, byte s, byte v)
{
  byte region;
  unsigned short r, g, b, fpart;

  // one of 6 hue regions
  region = h / 43;
  // remainder within region, scaled 0-255
  fpart = (h - (region * 43)) * 6L;

  switch (region)
  {
  case 0:
    r = v;
    g = (v * (255 - ((s * (255 - fpart)) >> 8))) >> 8; // t
    b = (v * (255 - s)) >> 8; // p
    break;
  case 1:
    r = (v * (255 - ((s * fpart) >> 8))) >> 8; // q
    g = v;
    b = (v * (255 - s)) >> 8; // p
    break;
  case 2:
    r = (v * (255 - s)) >> 8; // p
    g = v;
    b = (v * (255 - ((s * (255 - fpart)) >> 8))) >> 8;  // t
    break;
  case 3:
    r = (v * (255 - s)) >> 8; // p
    g = (v * (255 - ((s * fpart) >> 8))) >> 8; // q
    b = v;
    break;
  case 4:
    r = (v * (255 - ((s * (255 - fpart)) >> 8))) >> 8;  // t
    g = (v * (255 - s)) >> 8; // p
    b = v;
    break;
  case 5:
    r = v;
    g = (v * (255 - s)) >> 8; // p
    b = (v * (255 - ((s * fpart) >> 8))) >> 8; // q
    break;
  default:
    return 0;
  }

  // RGB 565
  return ((b & 0xF8) >> 3) |
    ((g & 0xFC) << 3) |
    ((r & 0xF8) << 8);
}

// ====== Anti-alias line ======
const tFixed kMinShiftAngle = 0x00AAU;  // 0.66
const tFixed kMaxShiftAngle = 0x0100U;  // 1.0
void adjustEnd(bool steep, uint8_t thick, int x, int y, word* pPalette, uint8_t paletteIdx, tFixed gradient, bool isStart)
{
  // make the ends a little rounder, a simple heuristic, not pixel-perfect
  // strip of aXXXb goes to _aXb_ where b ia a', _ is background
  Dial::StartRead();
  int shift = 0;
  if (steep)
  {
    if (kMinShiftAngle < gradient && gradient < kMaxShiftAngle)
      shift = +1;
    else if ((tFixed)~kMaxShiftAngle < gradient && gradient < (tFixed)~kMinShiftAngle)
      shift = -1;
    if (isStart)
      shift = -shift;
    DiskLCD::BeginFill(x, y, 2 + thick, 1);
    DiskLCD::FillColour(1 + shift, Dial::Read(x, y));
    DiskLCD::FillColour(1, pPalette[paletteIdx]);
    DiskLCD::FillColour(1, pPalette[0]);
    DiskLCD::FillColour(1, pPalette[PALETTE_SIZE - 1 - paletteIdx]);
    DiskLCD::FillColour(1 - shift, Dial::Read(x + 4, y));
  }
  else
  {
    if (kMinShiftAngle < gradient && gradient < kMaxShiftAngle)
      shift = +1;
    else if ((tFixed)~kMaxShiftAngle < gradient && gradient < (tFixed)~kMinShiftAngle)
      shift = -1;
    if (isStart)
      shift = -shift;
    DiskLCD::BeginFill(x, y, 1, 2 + thick);
    DiskLCD::FillColour(1 + shift, Dial::Read(x, y));
    DiskLCD::FillColour(1, pPalette[paletteIdx]);
    DiskLCD::FillColour(1, pPalette[0]);
    DiskLCD::FillColour(1, pPalette[PALETTE_SIZE - 1 - paletteIdx]);
    DiskLCD::FillColour(1 - shift, Dial::Read(x, y + 4));
  }
}


void swap(int& a, int& b)
{
  int temp = a;
  a = b;
  b = temp;
}

// see also https://www.codeproject.com/Articles/13360/Antialiasing-Wu-Algorithm
// https://en.wikipedia.org/wiki/Xiaolin_Wu%27s_line_algorithm
// https://www.geeksforgeeks.org/anti-aliased-line-xiaolin-wus-algorithm/
// https://en.wikipedia.org/wiki/Line_drawing_algorithm thickness?

void drawLine(int x0, int y0, int x1, int y1, uint8_t thick, uint8_t* pPixelData, word* pPalette)
{
  // Draw anti-aliased line (x0,y0)-(x1,y1) with given thickness using given palette
  // Record draw info in pPixelData for ereasure
  bool steep = abs(y1 - y0) > abs(x1 - x0);
  bool endIsX0Y0 = false;
  // swap if slope > 1 or line is right-left
  if (steep)
  {
    swap(x0, y0);
    swap(x1, y1);
  }
  if (x0 > x1)
  {
    swap(x0, x1);
    swap(y0, y1);
    endIsX0Y0 = !endIsX0Y0;
  }
  
  bool negGradient = y0 > y1;

  // Compute the slope
  tFixed dx = FIXED_FROM_INT(x1 - x0);
  tFixed dy = negGradient ? FIXED_FROM_INT(y0 - y1) : FIXED_FROM_INT(y1 - y0);
  tFixed gradient = FIXED_FROM_INT(1);
  if (dx != FIXED_FROM_INT(0))
  {
    // fixed-point division
    tFixedTemp temp = ((tFixedTemp)dy << FIXED_SCALE_BITS) / dx;
    gradient = (tFixed)temp;
  }
  if (negGradient)  // negate AFTER the division
    gradient = ~gradient;
  tFixed intersectY = FIXED_FROM_INT(y0);

  *(pPixelData++) = (uint8_t)x0;
  *(pPixelData++) = (uint8_t)x1;
  *(pPixelData++) = steep;
  *(pPixelData++) = thick;
  
  // note some things for adjustEnd()
  uint8_t* pPixelDataStart = pPixelData;
  tFixed intersectYStart = intersectY;
  
  uint8_t paletteIdx = 0;
  // drawing loop 
  if (steep)
  {
    for (int y = x0; y <= x1; y++)
    {
      int iX = FIXED_INT_PART(intersectY);
      // horizontal window 2 pix from (iX - 1, y)
      DiskLCD::BeginFill(iX - 1, y, 2 + thick, 1);
      *(pPixelData++) = iX - 1;
      // the MS bits of fractional part gives us the palette idx
      paletteIdx = FIXED_FRAC_PART(intersectY) >> (FIXED_SCALE_BITS - 8 + (8 - PALETTE_BITS));
      DiskLCD::FillColour(1, pPalette[paletteIdx]);
      if (thick)
        DiskLCD::FillColour(thick, pPalette[0]);
      DiskLCD::FillColour(1, pPalette[PALETTE_SIZE - 1 - paletteIdx]);
      intersectY += gradient;
    }
    if (thick == 3)
    {
      // end adjustment doesn't impact speed of main loop
      if (endIsX0Y0)
        adjustEnd(true, thick, *pPixelDataStart,  x0, pPalette, FIXED_FRAC_PART(intersectYStart) >> (FIXED_SCALE_BITS - 8 + (8 - PALETTE_BITS)), gradient, false);
      else
        adjustEnd(true, thick, *(pPixelData - 1), x1, pPalette, paletteIdx, gradient, true);
    }
  }
  else
  {
    for (int x = x0; x <= x1; x++)
    {
      int iY = FIXED_INT_PART(intersectY);
      // vertical window 2 pix from (x, iY - 1)
      DiskLCD::BeginFill(x, iY - 1, 1, 2 + thick);
      *(pPixelData++) = iY - 1;
      // the MS bits of fractional part gives us the palette idx
      paletteIdx = FIXED_FRAC_PART(intersectY) >> (FIXED_SCALE_BITS - 8 + (8 - PALETTE_BITS));
      DiskLCD::FillColour(1, pPalette[paletteIdx]);
      if (thick)
        DiskLCD::FillColour(thick, pPalette[0]);
      DiskLCD::FillColour(1, pPalette[PALETTE_SIZE - 1 - paletteIdx]);
      intersectY += gradient;
    }
    if (thick == 3)
    {
      if (endIsX0Y0)
        adjustEnd(false, thick, x0, *pPixelDataStart, pPalette, FIXED_FRAC_PART(intersectYStart) >> (FIXED_SCALE_BITS - 8 + (8 - PALETTE_BITS)), gradient, false);
      else
        adjustEnd(false, thick, x1, *(pPixelData - 1), pPalette, paletteIdx, gradient, true);
    }
  }
}

void pointOnLine(int x0, int y0, int x1, int y1, int& pX, int& pY)
{
  // This is a COPY of the above line-draw algorithm, but much simplified, no drawing is done
  // Finds the point in the line (x0,y0)-(x1,y1) closest to (pX, pY) and CHANGES
  // (pX, pY) to be the difference between them.
  // It is used to reduce the offset look of the subsidiary needles from their fixed hubs
  // see DrawSubsidiary
  bool steep = abs(y1 - y0) > abs(x1 - x0);
  // swap if slope > 1 or line is right-left
  if (steep)
  {
    swap(x0, y0);
    swap(x1, y1);
  }
  if (x0 > x1)
  {
    swap(x0, x1);
    swap(y0, y1);
  }

  bool negGradient = y0 > y1;
  // Compute the slope
  tFixed dx = FIXED_FROM_INT(x1 - x0);
  tFixed dy = negGradient ? FIXED_FROM_INT(y0 - y1) : FIXED_FROM_INT(y1 - y0);
  tFixed gradient = FIXED_FROM_INT(1);
  if (dx != FIXED_FROM_INT(0))
  {
    // fixed-point division
    tFixedTemp temp = ((tFixedTemp)dy << FIXED_SCALE_BITS) / dx;
    gradient = (tFixed)temp;
  }
  if (negGradient)  // negate AFTER the division
    gradient = ~gradient;
  tFixed intersectY = FIXED_FROM_INT(y0);

  if (steep)
    for (int y = x0; y <= x1; y++)
    {
      int iX = FIXED_INT_PART(intersectY);
      // horizontal window 2 pix from (iX - 1, y)
      if (y == pY)
      {
        pX -= iX;
        pY = 0;
        return;
      }
      intersectY += gradient;
    }
  else
    for (int x = x0; x <= x1; x++)
    {
      int iY = FIXED_INT_PART(intersectY);
      if (x == pX)
      {
        pY -= iY;
        pX = 0;
        return;
      }
      intersectY += gradient;
    }
}

// Bhaskara I's sine approximation, r*sin(t) (t in degrees)
int32_t rsint(int32_t r, int32_t t)
{
  //  r*4L*t*(180L-t)/(40500L - t*(180L-t));
  while (t < 0) 
  {
    t += 180;
    r = -r;
  }
  while (t > 180)
  {
    t -= 180;
    r = -r;
  }
  int32_t numerator   = r*4L*t*(180L-t);
  int32_t denominator = (40500L - t*(180L-t));
  int32_t result = numerator/denominator;
  // round!
  if (result >= 0)
  {
    if ((numerator % denominator) >= denominator/2)
      result++;
  }
  else
  {
    numerator = abs(numerator);
    denominator = abs(denominator);
    if ((numerator % denominator) >= denominator/2)
      result--;
  }
  return result;

}

int32_t rcost(int32_t r, int32_t t)
{
  return rsint(r, 90L - t);
}

// ====== Circles ======
// Circle diameter must be *ODD*
// The circle is enclosed in a square of size diameter
// xxxRasterOffset[n] is the number of background pixels from the left edge of square over to the left edge of the circle
// where n is the row starting with 0 at the top
// n is 0...diameter/2. Diameter must be ODD the middle row is not stored since the offset must be 0
#define PRESSURE_HUB_SIZE 15
uint8_t pPressureHubRasterOffsets[PRESSURE_HUB_SIZE/2];
#define SUBSIDIARY_HUB_SIZE 7
uint8_t pSubsidiaryHubRasterOffsets[SUBSIDIARY_HUB_SIZE/2];

void BuildCircle(int diameter, uint8_t* offsets)
{
  // use Bresenham to pre-compute pixels on circle, and convert to raster row info
  int8_t radius = diameter/2;
  for (int i = 0; i < radius; i++)
    offsets[i] = 2*radius;
  int8_t r = radius, x = -r, y = 0, err = 2 - 2*r, cenX = r, cenY = r;
  do 
  {
    // update raster data with pixel info
    if ((cenX + x) <= radius && (cenY - y) < radius && x < offsets[cenY - y])
      offsets[cenY - y] = cenX + x;  // only care about upper-left edge, excluding the midline
    r = err;
    if (r <= y) 
      err += (++y)*2 + 1;
    if (r > x || err > y)
      err += (++x)*2 + 1;
  } while (x < 0);
}

void drawCircle(int x, int y, int d, word colour, uint8_t* offsets)
{
  // draw circle in colour using offsets table, centre (x,y)
  int r = d/2;
  x -= r;
  y -= r;
  for (int row = 0; row < d; row++)
  {
    if (row == r)
    {
      DiskLCD::FillColour(DiskLCD::BeginFill(x, y + row,  d,  1), colour);
    }
    else
    {
      int offs = (row > r)?offsets[d - row - 1]:offsets[row];
      DiskLCD::FillColour(DiskLCD::BeginFill(x + offs, y + row,  d - 2*offs,  1), colour);
    }
  }
}

void Init()
{
  // Init circles & palettes
  BuildCircle(PRESSURE_HUB_SIZE, pPressureHubRasterOffsets);
  BuildCircle(SUBSIDIARY_HUB_SIZE, pSubsidiaryHubRasterOffsets);

  // build the palettes, shades of grey and lighter shades of copper/brass
  // The anti-aliasing is against a *white* background
  for (int idx = 0; idx < PALETTE_SIZE; idx++)
  {
    int component = idx * 256 / PALETTE_SIZE;
    if (component > 255)
      component = 255;
    pBlackPalette[idx] = RGB(component, component, component);
    //DiskLCD::FillColour(DiskLCD::BeginFill(0 + idx*10, DiskLCD::Diameter/2, 10, 10), pBlackPalette[idx]);
    component = idx * 128 / PALETTE_SIZE;
    if (component > 127)
      component = 127;
    pTrendPalette[idx] = HSVtoRGB(37, 128 - component, 128 + component);
    //DiskLCD::FillColour(DiskLCD::BeginFill(0 + idx * 10, DiskLCD::Diameter/2+20, 10, 10), pTrendPalette[idx]);
  }
}

void Erase(const uint8_t* pPixelData)
{
  // Erase the needle by using the pixel data to repaint the 
  // line-drawing pixels white
  Dial::StartRead();
  // get the parameters
  int pixelCoord0 = *(pPixelData++);
  int pixelCoord1 = *(pPixelData++);
  bool pixelSteep = *(pPixelData++);
  uint8_t pixelThick = *(pPixelData++);  
  // a series of short vt or hz lines
  if (pixelSteep)
    for (int y = pixelCoord0; y <= pixelCoord1; y++)
    {
      int x = *(pPixelData++);
      DiskLCD::BeginFill(x, y, pixelThick + 2, 1);
      DiskLCD::FillColour(1, Dial::Read(x++, y));
      uint8_t t = pixelThick;
      while (t--)
        DiskLCD::FillColour(1, Dial::Read(x++, y));
      DiskLCD::FillColour(1, Dial::Read(x, y));
    }
  else
    for (int x = pixelCoord0; x <= pixelCoord1; x++)
    {
      int y = *(pPixelData++);
      DiskLCD::BeginFill(x, y, 1, pixelThick + 2);
      DiskLCD::FillColour(1, Dial::Read(x, y++));
      uint8_t t = pixelThick;
      while (t--)
        DiskLCD::FillColour(1, Dial::Read(x, y++));
      DiskLCD::FillColour(1, Dial::Read(x, y));
    }
}

void DrawPressure(int angleDeg)
{
  // Draw the pressure needle at the given angle, in degrees, 0 is straight up
  // It's the longest and has a tail
  drawLine(PRESSURE_NEEDLE_X - rsint(PRESSURE_NEEDLE_TAIL, angleDeg), PRESSURE_NEEDLE_Y + rcost(PRESSURE_NEEDLE_TAIL, angleDeg), 
           PRESSURE_NEEDLE_X + rsint(PRESSURE_NEEDLE_LEN, angleDeg), PRESSURE_NEEDLE_Y - rcost(PRESSURE_NEEDLE_LEN, angleDeg), 
           1, pPressurePixelData, pBlackPalette);
}

void ErasePressure()
{
   Erase(pPressurePixelData);
}

void DrawPressureTrend(int angleDeg)
{
  // Draw the pressure trend needle (pressure 3 hours ago)
  // It's shorter, has no tail, has a hub and is "copper/brass"
  drawLine(PRESSURE_NEEDLE_X, PRESSURE_NEEDLE_Y,
           PRESSURE_NEEDLE_X + rsint(PRESSURE_TREND_NEEDLE_LEN, angleDeg), PRESSURE_NEEDLE_Y - rcost(PRESSURE_TREND_NEEDLE_LEN, angleDeg), 
           3, pPressureTrendPixelData, pTrendPalette);
  drawCircle(PRESSURE_NEEDLE_X, PRESSURE_NEEDLE_Y, PRESSURE_HUB_SIZE, pTrendPalette[0], pPressureHubRasterOffsets);
}

void ErasePressureTrend()
{
   Erase(pPressureTrendPixelData);
}

void DrawSubsidiary(int angleDeg, int X, uint8_t* pPixelData)
{
  // Subsidary dials are small, have a tail and a hub
  int x0 = X - rsint(SUBSIDIARY_NEEDLE_TAIL, angleDeg);
  int y0 = SUBSIDIARY_NEEDLE_Y + rcost(SUBSIDIARY_NEEDLE_TAIL, angleDeg);
  int x1 = X + rsint(SUBSIDIARY_NEEDLE_LEN,  angleDeg);
  int y1 = SUBSIDIARY_NEEDLE_Y - rcost(SUBSIDIARY_NEEDLE_LEN, angleDeg);

  // Because of pixel rounding, the line from (x0,y0) to (x1,y1) may not pass through the 
  // needle's pivot point, may look offset against the hub.
  // Computed how far off it is in x & y (+/-1 or 0) and add that adjustment to the tail (x0,y0)
  // Looks much better
  int pX = X;
  int pY = SUBSIDIARY_NEEDLE_Y;
  pointOnLine(x0, y0, x1, y1, pX, pY);
  x0 += pX;
  y0 += pY;
    
  drawLine(x0, y0, 
           x1, y1, 
           1, pPixelData, pBlackPalette);
  drawCircle(X, SUBSIDIARY_NEEDLE_Y, SUBSIDIARY_HUB_SIZE, 0x0000, pSubsidiaryHubRasterOffsets);
}

void DrawTemperature(int angleDeg)
{
  DrawSubsidiary(angleDeg, TEMPERATURE_NEEDLE_X, pTemperaturePixelData);
}

void EraseTemperature()
{
   Erase(pTemperaturePixelData);
}

void DrawHumidity(int angleDeg)
{
  DrawSubsidiary(angleDeg, HUMIDITY_NEEDLE_X, pHumidityPixelData);
}

void EraseHumidity()
{
   Erase(pHumidityPixelData);
}

// "Mark" ambigram strokes (half of them)
static const int8_t pMarkShape[] PROGMEM =
{
  127, 
    0, 0,
    10, 0,
  127, 
    0, 0,
    20, -50,
    20, 50,
    40, 20,
    30, 0,
    60, -50,
  127, 
    60, -70,
    60, 50,
  127, 
    60, -50,
    90, 0,
    80, 40,
  127, 
    60, -50,
    80, -70,
  -127
};

#define pgm_read_char(_addr) (int8_t)pgm_read_byte(_addr)
void Splash()
{
  // draw grey->white rotating "Mark" ambigram on black
  // A Splash routine doesn't really belong in this file, but it does have things we need
  word pWhitePalette[PALETTE_SIZE];
  int x0, y0, x1 = 0, y1 = 0;
  for (int a = 0; a <= 180; a += 10)
  {
    if (a == 0 || a == 180)
    {
      // build the draw & erase palettes, use the needle black one, it will be remade in Init
      int base = a?0:128;
      for (int idx = 0; idx < PALETTE_SIZE; idx++)
      {
        int component = 255 - (base + idx * (256-base) / PALETTE_SIZE);
        if (component < 0)
          component = 0;
        pWhitePalette[idx] = RGB(component, component, component); // drawing grey/white on black
        //DiskLCD::FillColour(DiskLCD::BeginFill(0 + idx*10, DiskLCD::Diameter/2, 10, 10), pWhitePalette[idx]);
        pBlackPalette[idx] = RGB(0, 0, 0);  // erase to black
      }
    }
    for (int pass = 0; pass < 2; pass++)  // draw, erase
    {
      if (pass && a == 180)
        break;
      int t = -a;
      const int8_t* pData = pMarkShape;
      while (pgm_read_char(pData) != -127) // -127=end
      {
        bool move = pgm_read_char(pData) == +127; // +127=move
        if (move)
          pData++;
        int x = +pgm_read_char(pData++);
        int y = -pgm_read_char(pData++);
        if (!move)
        {
          x0 = x1;
          y0 = y1;
        }
        x1 = rcost(x, t) + rsint(y, t);
        y1 = rcost(y, t) - rsint(x, t);       
        if (!move)
        {    
          drawLine(DiskLCD::Diameter/2 + x0, DiskLCD::Diameter/2 + y0, 
                   DiskLCD::Diameter/2 + x1, DiskLCD::Diameter/2 + y1, 
                   1, pPressurePixelData, pass?pBlackPalette:pWhitePalette);
          // and mirror imaged line
          drawLine(DiskLCD::Diameter/2 - x0, DiskLCD::Diameter/2 - y0, 
                   DiskLCD::Diameter/2 - x1, DiskLCD::Diameter/2 - y1, 
                   1, pPressurePixelData, pass?pBlackPalette:pWhitePalette);
        }
      }
    }
  }
  delay(2000);  // linger
}

}
