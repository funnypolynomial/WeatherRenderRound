-- Dials --
barometer.py:
  Inksnek/Inkscape extension code to generate the dial(s).
  See self.metric variable to switch dials
dial_template.svg:
  Template for above

To generate the images from Inkscape:
Open the template
Run the Inksnek extension.
Select Export, Document.
Choose the file and PNG.
Hit the Settings button beside the Export format field.
No interlacing, RGB-8, No compression, 0.0 DPI. Antialias 1.
Hit Export.

dial_hPa.png:
  Metric dial image
dial_inHg.png:
  Imperial dial image.
encode_dial.py:
  Encodes an image as PROGMEM data
encode_dials.bat:
  Makes both dial's data

-- Enclosure
enclosure.py:
  Inksnek/Inkscape extension code to generate the cutting plan for the enclosure.
enclosure_template.svg:
  Template for above.
enclosure.svg:
  Laser cutting plan for enclosure


-- Board
See Gerber_WeatherRenderRound.zip