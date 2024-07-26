#pragma once

// BME280: read pressure, temperature & humidity

namespace Sensor
{
  void Init();
  bool Read(int& pressurehPa, int& temperatureC, int& humidityPercent);
  int32_t Round(int32_t value, int divisor);
};
