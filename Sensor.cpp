#include <Arduino.h>
#include <pocketBME280.h>
#include "Sensor.h"

// BME280: read pressure, temperature & humidity
// Developed with the Adafruit library, but I've made an effort to keep this all integer-based, fast and compact (eg anti-aliasing)
// So it bugged me that it dragged in the floating point library. A bit of googling and I found pocketBME280.  
// https://github.com/angrest/pocketBME280
// Saves ~4k of program storage space


namespace Sensor
{
pocketBME280 bme;

void Init()
{
  Wire.begin();
  bme.begin();
}

int32_t Round(int32_t value, int divisor)
{
  // return value/divisor, rounded
  int32_t result = value/divisor;
  if ((abs(result) % divisor) >= (divisor / 2))
    result += (result < 0)?-1:+1;
  return result;
}

int Round(uint32_t value, int divisor)
{
  // return value/divisor, rounded
  int result = value/divisor;
  if ((result % divisor) >= (divisor / 2))
    result++;
  return result;
}

bool Read(int& pressurehPa, int& temperatureC, int& humidityPercent)
{
  // return true if read all three
  bme.startMeasurement();
  int ctr = 100; // avoid infinite loop
  while (!bme.isMeasuring() && ctr--) 
    delay(1);
  while (bme.isMeasuring() && ctr--)
    delay(1);
  if (!ctr)
    return false;
  temperatureC = Round(bme.getTemperature(), 100);
  pressurehPa = Round(bme.getPressure(), 100);
  humidityPercent = Round(bme.getHumidity(), 1024);
#if defined(DEMO) && defined(DEBUG)
  temperatureC = 25;
  pressurehPa = 1015;
  humidityPercent = 32;
#endif  
  return true;
}
};
