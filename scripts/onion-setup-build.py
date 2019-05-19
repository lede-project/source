#!/usr/bin/python

### ensure firmware will have correct version info

import json
import re
import os
import argparse


# find the directory of the script
dirName = os.path.dirname(os.path.abspath(__file__))

buildInfoFile = "build_info.json"
baseConfigFile = ".config"

# get build info
def getBuildInfo():
	filePath = ('../%s'%buildInfoFile) 	# path relative to this directory
	filePath = '/'.join([dirName, filePath])
	with open(filePath, "r") as f:
		data = json.load(f);
		return data

# update the buildroot's version info
def updateFwInfo(build, version):
	filePath = "../files/etc/uci-defaults/12_onion_version" 	# path relative to this directory
	filePath = '/'.join([dirName, filePath])
	with open(filePath, "r+") as f:
		data = f.read()

		data = re.sub(r"set\ onion\.@onion\[0\]\.version='(.*)'", "set onion.@onion[0].version='%s'"%version, data )
		data = re.sub(r"set\ onion\.@onion\[0\]\.build='(.*)'", "set onion.@onion[0].build='%s'"%build, data )

		f.seek(0)
		f.write(data)
	
# increment build number
def incrementBuildNumber():
	buildNumber = 0
	filePath = ("../%s"%buildInfoFile) 	# path relative to this directory
	filePath = '/'.join([dirName, filePath])

	with open(filePath, "r+") as f:
		data = json.load(f)
		buildNumber = data['build'] + 1
		data['build'] = buildNumber

		f.seek(0)
		json.dump(data, f)

	return buildNumber
	
# create a symlink to the specified config file
def setConfigFile(configFile):
	linkConfigPath = '/'.join([dirName, '..', baseConfigFile])
	targetConfigPath = '/'.join([dirName, '..', configFile])

	# take care of existing config file
	if os.path.islink(linkConfigPath):
		os.unlink(linkConfigPath)
	elif os.path.isfile(linkConfigPath):
		os.remove(linkConfigPath)
		
	if os.path.isfile(targetConfigPath):
		print("   Config: %s"%configFile)
		os.symlink(targetConfigPath, linkConfigPath)
		

# setup the build
def setupBuild(buildNumberInput):
	info = getBuildInfo()
	buildNum = info['build']
	if buildNumberInput > 0:
		buildNum = buildNumberInput

	print "   Version: %s"%info["version"]
	print "   Build: %s"%buildNum
	
	updateFwInfo(buildNum, info["version"])


if __name__ == "__main__":
	parser = argparse.ArgumentParser(description='Prepare build system with version info.')
	parser.add_argument('-i', '--increment', action='store_true', help='increment build number')
	parser.add_argument('-b', '--build-number', type=int, default=0, help='set build number')
	parser.add_argument('-c', '--config', default='.config.O2', help='set config file for build system')
	args = parser.parse_args()
	
	if args.increment:
		incrementBuildNumber()
	
	print '*'*20
	
	setupBuild(args.build_number)
	setConfigFile(args.config)
	
	print '*'*20
	

	
