#pragma once

namespace Display
{
  const int NULL_READING = -9999;
  const int FONT_SIZE = 5;
  void UpdateNeedles(int PressureHPA, int PressureTrendHPA, int TemperatureC, int HumidityPC);

  void DrawSig(bool err);
  void DrawChar(int x, int y, char ch, word fg, word bg, int size);
  void DrawStr(int x, int y, const char* str, word fg, word bg, int size);
  char* FormatNNNN(int value, char* pStr, int MSD = 1000, char pad = ' ');
};
