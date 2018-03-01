# Samples from a matplotlib colormap and prints a constant string to stdout, which can then be embedded in a C++ program

import numpy as np
import matplotlib.cm

nValues = 500;

# get a matplotlib colormap
# cmapName = 'Spectral'
# cmap = matplotlib.cm.get_cmap(cmapName)

# get a cmocean colormap
import cmocean
cmapName = 'phase'
cmap = cmocean.cm.phase


print("static const Colormap CM_" + cmapName.upper() + " = {")
print("    \"" + cmapName + "\",")

dataStr = "{"
for i in range(nValues):

    floatInd = float(i) / (nValues-1)
    color = cmap(floatInd)

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
