#!/usr/bin/python

### rename firmware images based on device name

import json
import re
import os


# find the directory of the script
dirName = os.path.dirname(os.path.abspath(__file__))

buildInfoFile = "build_info.json"
imageDir = "../bin/targets/ramips/mt76x8/"

# get build info
def getBuildInfo():
	filePath = ('../%s'%buildInfoFile) 	# path relative to this directory
	filePath = '/'.join([dirName, filePath])
	with open(filePath, "r") as f:
		data = json.load(f);
		return data

# setup the build
def setupImages():
	info = getBuildInfo()
	
	print ('*'*20)
	for device in info['devices']:
		print('Renaming image for %s device'%device['symbol'])
		imageName = "%s/%s/openwrt-ramips-mt76x8-%s-squashfs-sysupgrade.bin"%(dirName, imageDir, device['symbol'])
		newImageName = "%s/%s/%s-v%s-b%s.bin"%(dirName, imageDir, device['symbol'], info['version'], info['build'])
		if os.path.exists(imageName):
			os.rename(imageName, newImageName)
	print ('*'*20)

if __name__ == "__main__":
	setupImages()
	
	

	

