#!/usr/bin/python
import sys
import os
from PIL import Image

# encode greyscale dial
# each row is 
# <number of bytes of pixel data> <pixel offset to first on disk> <data1> <data2>...
# data is 
#  0b0nnnnnnn for n background (white) pix
# or
#  0b1aaa1bbb or 0b1aaa0000, encode 2 or 1 pix, a, b, 000=full black, 111=lightest
# trailing backgound omitted

# also, offsets array


def ByteStr(b):
    return "0x" + hex(256 + b)[3:].upper()

    
def PaletteEntry(colour):
    # map colour to 8 shades of grey, todo, 7 (white) is wrong?
    return colour[0] / 32

def IsForeground(colour):
    # true if colour is black/grey
    return colour[2] != 255 and colour[0] == colour[1] and colour[1] == colour[2]
    
def PaletteEntry2(colour):
    # map colour to 00=black,01=dark grey, 10=light grey, 11=white
    if IsForeground(colour):
        return colour[0] / 85
    else:
        return 0b00000011

if len(sys.argv) != 2:
    sys.stdout.write("parameters: <input image file>\n")
    exit()
fileName = sys.argv[1]
bmp = Image.open(fileName)
#nonImageColour = (128, 0, 0,    255) # but its antialiased!
backgroundColour = (255, 255, 255,  255)
total_bytes = 0
count = 0
dialName = fileName.split(".")[0]
sys.stdout.write("// " + dialName +"\n")
sys.stdout.write("static const uint8_t pDialData[] PROGMEM =\n")
sys.stdout.write("{\n")
sys.stdout.write("// len,  offs,  data...\n")
offsetStr = ""
for row in range(bmp.height):
    # encode a row
    offsetStr += str(total_bytes) + ", "
    if row !=0 and row % 50 == 0:
        offsetStr += "\n  "
    col = 0
    while bmp.getpixel((col, row)) != backgroundColour:
      col += 1
    rowByteCount = 0
    encodedRow = ByteStr(col) + ",  "
    while col < bmp.width and (bmp.getpixel((col, row)) == backgroundColour or IsForeground(bmp.getpixel((col, row)))):
      if bmp.getpixel((col, row)) == backgroundColour:
        # background (white) pixels are 0b0nnnnnnn
        count = 0
        while (col + count) < bmp.width and bmp.getpixel((col + count, row)) == backgroundColour:
          count += 1
        if (col + count) == bmp.width or not IsForeground(bmp.getpixel((col + count, row))):
          break #omit trailing white
        col += count
        while count >= 128:
          encodedRow += ByteStr(127) + ", "
          rowByteCount += 1
          count -= 127
        encodedRow += ByteStr(count) + ", "
        rowByteCount += 1
        count -= 127
      elif IsForeground(bmp.getpixel((col, row))):
        # foreground pixels are 0b1aaa1bbb or 0b1aaa0000, encode 1 or 2 pix
        # 1st pix
        value = 128 + PaletteEntry(bmp.getpixel((col, row)))*16
        col += 1
        if IsForeground(bmp.getpixel((col, row))):
          # 2nd pix
          value += 8 + PaletteEntry(bmp.getpixel((col, row)))
          col += 1
        encodedRow += ByteStr(value) + ", "
        rowByteCount += 1

    line = "  " + ByteStr(rowByteCount) + ",  " + encodedRow + "\n"
    sys.stdout.write(line)
    total_bytes += rowByteCount + 2
      

sys.stdout.write("}; // (")
sys.stdout.write(str(total_bytes))
sys.stdout.write(" bytes)\n\n")    

sys.stdout.write("static const uint16_t pDialOffsetData[] PROGMEM =\n")
sys.stdout.write("{\n  ")
sys.stdout.write(offsetStr)
sys.stdout.write("\n}; // (")
sys.stdout.write(str(2*bmp.height))
sys.stdout.write(" bytes)\n\n")    


