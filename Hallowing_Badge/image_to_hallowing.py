#!/usr/bin/python

from PIL import Image
import os
import sys
import struct

image_name = "image2.png"

script_path = os.path.dirname(sys.argv[0])
images_path = os.path.join(script_path, "images")
image_path = os.path.join(images_path, image_name)

img = Image.open(image_path)
w = img.width
h = img.height

px = img.load()

name = os.path.splitext(image_name)[0]
raw_path = os.path.join(images_path, "%s.raw" % name)
raw_file = open(raw_path, "w+")

print "// Data from %s" % image_path
print ""
print "#define %s_ROWS %d" % (name.upper(), h)
print "#define %s_COLS %d" % (name.upper(), w)
print ""
print "const uint16_t %sData[%s_ROWS][%s_COLS] = {" % (name, name.upper(), name.upper())
for row in range(h):
	print("  {    // row %d" % row)
	line = "    "
	for col in range(w):
		pixel_data = px[col, row]
		if len(pixel_data) == 3:
			(r, g, b) = pixel_data
			a = 255
		else:
			(r, g, b, a) = pixel_data
		if a == 0:   # If alpha is zero then it should be white
			r = g = b = 255
		r = r >> 3   # 5 bits
		g = g >> 2   # 6 bits
		b = b >> 3   # 5 bits
		rgb16 = (r << 11) + (g << 5) + b;
		raw_file.write(struct.pack('>H', rgb16))
		# swapping bytes
		rgb16 = struct.unpack("<H", struct.pack(">H", rgb16))[0]
		line += "0x%04x, " % rgb16
		if col % 16 == 15:
			print(line)
			line = "    "
	if line.strip():
		print line
	print "  },"
	
print("};")
raw_file.close()
