#!/usr/bin/python

from PIL import Image

image_path = "/Users/gustavoambrozio/Downloads/gustavo.bmp"

img = Image.open(image_path)
w = img.width
h = img.height

px = img.load()

print "#define ROWS %d" % h
print "#define COLS %d" % w
print "const uint16_t xData[ROWS][COLS] = {"
for row in range(h):
	print("// row %d" % row)
	print("  {")
	line = "    "
	for col in range(w):
		(r, g, b) = px[col, row]
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
