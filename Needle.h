#pragma once

namespace Needle
{
  void Init();
  void Splash();
  
  void DrawPressure(int angleDeg);
  void ErasePressure();
  
  void DrawPressureTrend(int angleDeg);
  void ErasePressureTrend();
  
  void DrawTemperature(int angleDeg);
  void EraseTemperature();
  
  void DrawHumidity(int angleDeg);
  void EraseHumidity();
};
