#!/bin/sh

## This script will change the .config file to only build packages that are included in the firmware
## Results in a much faster compile time

sed -i '/kmod/! s/\(.*\)=m/\# \1 is not set/' .config

echo "> Done"

