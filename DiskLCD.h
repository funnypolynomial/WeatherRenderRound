#pragma once

// make an RGB word
#define RGB(_r, _g, _b) (word)((_b & 0x00F8) >> 3) | ((_g & 0x00FC) << 3) | ((_r & 0x00F8) << 8)

// optionally dump graphics cmds to serial:
//#define SERIALIZE
#ifdef SERIALIZE
#define SERIALISE_ON(_on) DiskLCD::_serialise=_on;
#else
#define SERIALISE_ON(_on)
#endif

namespace DiskLCD
{
  const int Diameter = 240;
  void Init();
  void On(bool on);
  void Brightness(byte pwm);
  unsigned long BeginFill(int x, int y,int w, int h);
  void FillColour(unsigned long size, word colour);

#ifdef SERIALIZE
  extern bool _serialise;
#endif  
};
