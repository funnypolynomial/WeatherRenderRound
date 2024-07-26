# WeatherRenderRound
Shows the weather – pressure (and trend), humidity and temperature on a small round dial.
![DSC06232](https://github.com/user-attachments/assets/f1afda82-29fc-4ff9-8e5f-8202a46a4598)


An Arduino project using a GC9a01 circular 1.28inch LCD, a BME280 pressure/temperature/humidity sensor and a Leonardo Tiny Atmega32U4 Main Board.

The dial was created with [Inksnek](https://github.com/funnypolynomial/inksnek) Python code running as an Extension in Inkscape v1.3.2 and exported as a PNG with +1 anti-alias.
A second Python script run-length-encoded the image as a PROGMEM array of bytes in the sketch.
The needles are drawn over the decompressed dial image using a **fixed-point** (uint16_t) **anti-aliased** line algorithm (with optional thickness). All the graphics are done from scratch, pocketBME280 is the only library.

![hpa](https://github.com/user-attachments/assets/a10af664-d7ba-48bc-ada3-ce294bb2591e)

Measurements of pressure, humidity and temperature come from a BME280 using the pocketBME280 Library.
Readings are taken every 5 minutes, the pressure trend needle shows the pressure 3 hours ago.

On the back is a small push-button, an LDR to dim the display at night and the pressure sensor. The sensor fits in a void in the PCB to reduce the profile.  
![DSC06211](https://github.com/user-attachments/assets/e83fe7ab-6d1a-4c43-b6b4-3ffd3d74016a)
On the front side of the PCB, in a larger space, the Leo sits **under** the LCD.
![DSC06176](https://github.com/user-attachments/assets/a8e158dd-964d-436a-b711-ad92a22a5f64)


The enclosure is a sandwich of 9 layers of transparent acrylic, also created in Python with my [Inksnek](https://github.com/funnypolynomial/inksnek) class, plus the PCB.

There's a schematic in PINS.h. The resources subdirectory includes python scripts etc and Gerber files.

Link to [project](https://hackaday.io/projects/hacker/2191) TBD
