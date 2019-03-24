#!/usr/bin/python

from PIL import Image
import os
import sys

image_name = "name.png"

script_path = os.path.dirname(sys.argv[0])
image_path = os.path.join(script_path, image_name)

img = Image.open(image_path)
w = img.width
h = img.height

px = img.load()

name = os.path.splitext(image_name)[0]

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
		(r, g, b, a) = px[col, row]
		if a == 0:   # If alpha is zero then it should be white
			r = g = b = 255
		r = r >> 3   # 5 bits
		g = g >> 2   # 6 bits
		b = b >> 3   # 5 bits
		rgb16 = (r << 11) + (g << 5) + b;
		line += "0x%04x, " % rgb16
		if col % 16 == 15:
			print(line)
			line = "    "
	print line
	print "  },"
	
print("};")
