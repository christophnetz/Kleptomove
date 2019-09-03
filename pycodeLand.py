# code to summarise data from simulation output

###############################
# importing libraries and paths
###############################
# check python path
import sys

# should yield python 3.7 file path
for p in sys.path:
    print(p)

import pandas as pd  # similar to dplyr! yay!
import os  # has list dir functions etc
import numpy as np  # some matrix functions
from scipy import misc
import matplotlib.pyplot as plt

# check the current working directory
os.getcwd()
currentWd = p  # os.path.dirname(os.path.abspath(__file__)) #os.getcwd()

# check again
print(currentWd)

# gather image output
outputFolder = os.path.join(currentWd, "landout_k")  # os.path.abspath("output")
# check for the right folder
if "landout_k" not in outputFolder:
    raise Exception('seems like the wrong output folder...')

############################
# list files and filter by name
############################

# gather contents of the folder
imgFiles = list()
for root, directories, filenames in os.walk(outputFolder):
    for filename in filenames:
        imgFiles.append(os.path.join(root, filename))

# filter filenames to match foodlandscape
imgFiles = list(filter(lambda x: "foodlandscape" in x, imgFiles))

# read the images in using a function and access the second channel (green)


def funcReadAndSelect (x):
    assert "str" in str(type(x)), "input doesn't seem to be a filepath"
    image = misc.imread(x)
    image = image[:, :, 1]  # selects the second channel which is green
    return image


#########################
# read the generation and simulation id
#########################
# write a function for this


def funcImgNames (x):
    assert "str" in str(type(x)), "input doesn't seem to be a filepath"
    assert "foodlandscape" in x, "input is not a foodlandscape"
    names = ((x.split("foodlandscape")[1]).split(".")[0]).split("sim")
    return names


# get the image identity to match to parameters later
imgId = list(map(funcImgNames, imgFiles))
# make a pd df
imgId = pd.DataFrame(imgId, columns=['gen','sim'])


# read in images
imgData = list(map(funcReadAndSelect, imgFiles[:10]))
# test import by showing the n/2th landscape
plt.imshow(imgData[9])

###############################
# section for Moran's i
###############################

# begin moran's I calc
# import pysal library
import pysal.lib
from pysal.explore.esda.moran import Moran

# get image size, assuming square
landsize = (imgData[1].shape[0])
# create a spatial weights matrix
w = pysal.lib.weights.lat2W(landsize, landsize)


# write function to get moran's I from a list of matrices
def funcMoranI (x):
    assert "array" in str(type(x)), "input doesn't seem to be an array"
    assert len(x.shape) == 2, "non 2-d array, input must be a 2d array"
    mi = Moran(x, w)
    return mi.I


# map moran func across list, convert to df in future
dfMoran = list(map(funcMoranI, imgData))