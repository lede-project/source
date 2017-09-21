#!/bin/sh
# (c)  Copyright 2013 dog hunter, LLC - All righs reserved  
# GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
# Domenico La Fauci
# Federico Musto

echo "Programming YUN console on ATMEL 32U4 firmware..."

[ -e /etc/linino/YunSerialTerminal.hex ] 

run-avrdude /etc/linino/YunSerialTerminal.hex

