# Samples from a matplotlib colormap and prints a constant string to stdout, which can then be embedded in a C++ program

import numpy as np
import matplotlib.cm
import matplotlib.pyplot as plt
import colorsys

nValues = 500;

# get a matplotlib colormap
cmapName = 'hsv'
cmap = plt.get_cmap(cmapName)

# get a cmocean colormap
# import cmocean
# cmapName = 'phase'
# cmap = cmocean.cm.phase


print("static const Colormap CM_" + cmapName.upper() + " = {")
print("    \"" + cmapName + "\",")

def reduce_brightness_rgb(color, factor):
    r, g, b = color
    h, l, s = colorsys.rgb_to_hls(r, g, b)
    l = max(0, l * factor)  # Ensure lightness remains non-negative
    r, g, b = colorsys.hls_to_rgb(h, l, s)
    return tuple(x for x in (r, g, b))

dataStr = "{"
for i in range(nValues):

    floatInd = float(i) / (nValues-1)
    color = cmap(floatInd)
    color = (color[0], color[1], color[2])
    # color = reduce_brightness_rgb(color, 0.85) # optional: dim bright colormaps a little

    dataStr += "{" + str(color[0]) + "," + str(color[1]) + "," + str(color[2]) + "},"

dataStr += "}"
print(dataStr)

print("};")


# Generate a constant colormap
# constCmapName = 'const_red'
# constCmapColor = (196/255., 133/255., 133/255.)

# print("static const Colormap CM_" + constCmapName.upper() + " = {")
# print("    \"" + constCmapName + "\",")

# dataStr = "{"
# for i in range(nValues):
    # dataStr += "{" + str(constCmapColor[0]) + "," + str(constCmapColor[1]) + "," + str(constCmapColor[2]) + "},"

# dataStr += "}"
# print(dataStr)

# print("};")
