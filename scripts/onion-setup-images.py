#!/usr/bin/python

### rename firmware images based on device name

import json
import re
import os
import argparse


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
def setupImages(buildNumberInput):
	info = getBuildInfo()
	buildNum = info['build']
	if buildNumberInput > 0:
		buildNum = buildNumberInput
	
	print ('*'*20)
	for device in info['devices']:
		print('Renaming image for %s device'%device['symbol'])
		imageName = "%s/%s/openwrt-ramips-mt76x8-%s-squashfs-sysupgrade.bin"%(dirName, imageDir, device['symbol'])
		newImageName = "%s/%s/%s-v%s-b%s.bin"%(dirName, imageDir, device['symbol'], info['version'], buildNum)
		if os.path.exists(imageName):
			os.rename(imageName, newImageName)
	print ('*'*20)

if __name__ == "__main__":
        parser = argparse.ArgumentParser(description='Rename firmware images to include device and version info.')
        parser.add_argument('-b', '--build-number', type=int, default=0, help='set build number')
        args = parser.parse_args()

        setupImages(args.build_number)
	
	

	

