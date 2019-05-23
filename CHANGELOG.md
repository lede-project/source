## Omega Firmware Changelog
Log of all changes made to the Onion Omega LEDE Firmware


### Version Syntax

The Omega Firmware version is in the following format:
```
A.B.C bXYZ
```

The individual parts correspond to the following:
* A is the ULTRA version
* B is the Major version
* C is the Minor version
* XYZ is the build number

For example:
```
0.0.6 b164
```

The build number will continuously increment over the course of development.

### Versions
Definining the differences in each version change

#### 0.3.2
**Whole Omega2 family on OpenWRT 18.06 based firmware**

#### 0.3.1
**Stable Omega2 Pro Firmware**

Firmware and package repos match that of lede-17.01 branch

#### 0.3.0
**Based on OpenWRT 18.06**

Moved to OpenWRT-18.06 based firmware
* Linux Kernel 4.14

#### 0.2.2
**Latest I2C Library**

Latest version of the I2C library, no more device client

> Based on lede-17.01 branch

#### 0.2.1
**OnionOS Release**

Next step forward for the Omega2

* OnionOS is now part of the firwmare, it features:
	* An App Manager
	* A new and improved, built-in Setup Wizard 

> Based on lede-17.01 branch

#### 0.2.0
**WiFi Warp Core Release**

A whole slew of updates, the most major being the new WiFi Warp Core - Enhanced MT7688 WiFi Driver

* Setup Wizard updated for compatibility new WiFi Warp Core and be more communicative with the user 
* Console Settings app updated for compatibility with WiFi Warp Core
* Updates to onion ubus package, base-www, et al

> Based on lede-17.01 branch

#### 0.1.10
**Fast-GPIO Fix Release**

* Fixed issue where `/dev/mem` device was missing, causing `fast-gpio` to seg-fault every time!

> Based on lede-17.01 branch

#### 0.1.9
**Cloud Fix Release**

* Cloud connectivity issue is now resolved
* Reset button on the Docks now works

> Based on lede-17.01 branch

#### 0.1.8
**Bug Fix Release**

* Fixed reboot for Omega2+
* Omega2+ can now read SD cards

> Based on lede-17.01 branch

#### 0.1.7
**Onion Repo Firmware**

* **Largest Change:** Now hosting and devices using only Onion repos for packages
* Fix for Node v4.3.1 and support for all Node-related packages

> Based on lede-17.01 branch

#### 0.1.6
**Omega2 operating Firmware**

* **Largest Change:** Separated Omega2 and Omega2+ firmwares
* Networking and firewall fixes for max connectivity
* Bug and stability fixes for Setup Wizard and the Console

> Based on lede-17.01 branch

#### 0.1.5
**Omega2 Factory Firmware**

Slew of Omega2 features:
* Compatability with Omega2 WiFi
* Using new WiFi Manager
* Overhauled Setup Wizard
* New Console

> Based on lede-17.01 branch

#### 0.1.4
**Repo Fix Release.**

* Fix for issue where Onion Repo Keys were not being installed by default, Omega users could not install Onion packages
* Created new release so all users get the fix!


#### 0.1.3
**Brin J2 Release.**

* Updates to Device Client architecture, now supports additional setup modes and compatibility with latest Cloud services
* Updates to I2C Expansion Drivers: additional control added for Relay Expansion


#### 0.1.2
**Brin J1 Release.**

Firmware updates for Onion Cloud App Store

* Added onion-helper ubus service
* Updates to I2C Expansion Drivers


#### 0.1.1
**Brin M1 Release.**

Firmware updates for stable Onion Cloud support!

* Stable device client
* Support for GPS Expansion
* Onion sh library fix: no longer creating blank log files


#### 0.1.0
**Brin A2 Release.**

Firmware includes support for the Onion Cloud!

Console Updates:
* Settings App now includes Cloud Settings Tab, used to register the device with the Cloud, and then to view the cloud registration info
* OLED App can now save converted image files to the Omega

#### 0.0.8
**Ando A1 Release.**

Added the following features to the firmware:
* Bluetooth audio support using Onion customized packages
* Network Manager that will automatically try to connect to recognized networks
* Fix for Node.JS and NPM where HTML requests were erroneously resolving to IPv6

Console Updates:
* Added Programming Calculator app
* Added Webcam App
* Overhauled WiFi settings apps

#### 0.0.7
**Modern Node Release.**

Added kernel changes to support NodeJs v4.3.1

Updates to Onion Drivers


#### 0.0.6
**Ando J1 Console Release.**

Added Resistor Calculator app.

Implemented universal notification system.

Added error notification to I2C app to indicate Expansion cannot be found.

Settings app update: added loading icon to wifi setup (sta and ap), added download progress bar to firmware upgrade, added Omega LED blinking before Factory Reset

#### 0.0.5
**Ando N2 Console Release.**

Added apps for the PWM, Relay, and OLED Expansions, expanded the GPIO Control App.

Added taskbar for apps.

#### 0.0.4
Early November release of the Console. This series of releases is code-named Ando.

Contains bug fixes, extends functionality of existing apps, added GPIO control app.

#### 0.0.3
First release of the Console

#### 0.0.2
Post production firmware

#### 0.0.1
Initial firmware sent to be flashed at the factory




### Build Notes
Defining the changes in each build. *Note that if a number is missing, that build failed the deployment process.*

#### b222
*May 23, 2019*

* Now building firmware for Omega2 LTE
* Made configuration modular - one config file for omega2 (includes omega2, omega2p, omega2pro), and one for omega2lte

#### b221
*April 23, 2019*

* Added `devmem` command to busybox 

#### b220
*April 12, 2019*

* Fixed USB autorun functionality for OpenWRT 18.06 based firmware
    * Added flexibility, checks for `/mnt/sda1` as well as `/mnt/sda` for USB mount points

#### b219
*April 5, 2019*

* Upgraded `p44-ledchain` package to version 2.0-7, latest available from Plan44, provides better support for older WS2812 LEDs
* Added `watch` utility to busybox

#### b218
*Mar 1, 2019*

* Added Python3 version of spidev module to package repo
* Memory leak fixes and code clean-up of fast-gpio utility
* omega2-ctrl utility gpiomux option now has an alias: pinmux

#### b217 
*Feb 26, 2019*

**Moving to v0.3.2**

* Removed console-install-tool
* Building firmware for Omega2 and Omega2+ as well

#### b216
*Feb 22, 2019*

* Fix for Omega2 Pro DTS file - OS can now use all 128 MB of RAM

#### b215
*Feb 21, 2019*

* Added rtl8821 kernel driver to package repo
* WiFi Warp Core - Additional fix for MAC address allocation

#### b214
*Feb 15, 2019*

* `python-spidev` module now features a half-duplex write-then-read xfer3 function

#### b213
*Feb 10, 2019*

* Dummy build

#### b212
*Feb 8, 2019*

* WiFi Warp Core update
	* Adjusted MAC address allocation for client WiFi interface - now correctly setting apcli0 MAC address in such a way that will not collide with eth0 interface MAC address

#### b211
*Feb 7, 2019*

* Upped to version `0.3.1` so all Omega2 Pros in the wild get upgraded to latest firmware

#### b210
*Jan 31, 2019*

* Now compiling and adding to package repos same packages as lede-17.01 branch

#### b209
*Jan 31, 2019*

* Update Omega2 Pro Init script
	* Now checks that OS is running from internal flash before executing
	* Relies on rc.local to be triggered
* Updated Onion-Script for correct Moscow timezone

#### b208
*Jan 29, 2019*

* Now building all kernel modules that were built in the lede-17.01 branch
	* Required update of wifi warp core binary for compatibility with kernel
* Updated oupgrade progrom to fix syntax error

#### b207
*Jan 16, 2019*

* Firmware now includes same packages as previous firwmares built on lede-17.01 branch
* Building all OnionOS Apps
* Added Node, NPM, and Node-Red packages to software repo

#### b206
*Jan 12, 2019*

* Removed deprecated Onion packages from .config file
* Updated O2 Pro init script to operate more intelligently

#### b205
*Dec 9, 2018*

* Hardware PWM is now supported!

#### b204
*Dec 4, 2018*

* Added blockd to firmware
	* Modified package so it will run earlier in the boot sequence (before network init)
	* Enabling anonymous mounting - so that it acts like mountd
* Ledchain kernel module will now be automatically inserted at boot
* Added LED morse trigger to firmware
* Sysupgrade will preserve `/root` directory

#### b203
*Nov 30, 2018*

* Omega2 Pro DTS is now correct:
	* System status LED on GPIO43
	* Wifi status LED on GPIO44
	* Note that this coincides with updated u-boot
* Writing wifi LED to wireless config
	* Had to update ramips.sh file to bring back `ramips_board_name` function

#### b202 
*Nov 28, 2018*

* Added patches to implement support for same SPI behaviour as in lede-17.01 based firmware

#### b201
**Updating to version 0.3.0**
*Nov 23, 2018*

* Initial cut of OpenWRT-18.06 firmware
* Support for:
	* WiFi Warp Core
	* I2C enhancements
		* Clock Stretching
		* ACK/NAK Handling (i2cdetect command support)
		* Unlimited message length
	* Reboot

#### b200
**Updating to version 0.2.2**
*Nov 7, 2018*

* Latest version of Onion I2C library
	* Now supports reading and writing multiple bytes to address with multiple bytes
* Removed device client from firmware

#### b199
*Oct 19, 2018*

* OnionOS
	* Updated App Manager to load available updates quicker and has ability to update OnionOS as well if required
	* Setup Wizard styling updates
* Added USB hotplug action to autorun a script
* Added sftp server into firmware

#### b198
*Oct 16, 2018*

**Updating to version 0.2.1**

* OnionOS 
	* OnionOS is now included in firmware 
	* Now includes a new and improved built-in Setup Wizard
	* Created OnionOS App to install the Legacy Console and make it available as an OnionOS App
	* Code Editor is no longer bundled with OnionOS - can be installed through App Manager
	* uci defaults script to configure mosquitto for mqtt over websocket
* Removed old setup wizard and base-www packages

#### b197
*Oct 5, 2018*

* OnionOS updates
	* Addition of App Manager to install and remove OnionOS apps!
	* Code editor no longer comes installed by default - space saving measure
* mosquitto and mosquitto client now installed in base firmware

#### b196
*Sept 10, 2018*

* Added Python3 module for ADC Expansion
* Factory partition is now writeable

#### b195 
*August 31, 2018*

* Added support for use of hardware PWM units on GPIO18 and 19
* Added python3 modules for Onion PWM, OLED, and Relay Expansions

#### b194
*August 1, 2018*

* Added kernel patch to stop the annoying "buggy DT" kernel warning on boot
* Added I2C kernel patch, now have support for:
	* ACK/NAK handling - so i2cdetect command works properly now
	* Clock stretching - compatible with more types of I2C devices
	* Unlimited message length - previously capped at 64 bytes
	* Supports repeated start sequence

#### b193
*July 31, 2018*

* Updated OnionOS and Added Sensor Monitor App

#### b192
*July 12, 2018*

* Added telnet and telnetd to busybox
* Updated `python-adc-exp` package
* Updated onion script to include setting of timezone

#### b191
*July 11, 2018*

* Added `adc-exp` and associated Python module to Onion package repo
* Now building kernel modules required for sw spi bitbang on gpio - included in Onion package repo

#### b190 
*July 3, 2018*

* Updated Onion Script
* Pushed out OnionOS and Camera App updates

#### b189
*July 3, 2018* 

* Upped omega2-ctrl package version to reflect code changes a while ago
* New version of OnionOS released, as well as a new Camera app

#### b188
*June 12, 2018*

* New I2C Expansion Drivers Version
	* Added `omegaMotors` Python module to `pyPwmExp`
	* Updates to i2c library functions:
		* upped buffer size to 256 bytes
		* fixed issue where buffer copy actually copies an extra byte
		* fixed readRaw function - correctly clearing buffer
* Update hostname tool new version
	* Now sets wwan hostname so Omega gets recognized by name in router client lists

#### b187
*June 4, 2018*

* Added Onion command line utility (first iteration - only configures ethernet port for now) 
* Warp Core Update - Added network testing tool
* Added kernel modules to Onion package repo: `kmod-pps-ldisc`, `kmod-usb-net-rndis`, `kmod-usb-net-cdc-ether`

#### b186
*Apr 30, 2018*

* Added `asterix15` and `baresip` packages to repo

#### b185
*Apr 26, 2018*

* Added slew of packages to Onion repo
	* `nodogsplash` captive portal + `kmod-ipt-ipopt` *added new routing repo*
	* `kmod-pps` + `kmod-pps-gpio`
	* `bind` server, tools, and lib
	* `etherwake`
	* `mdns` daemon + utils

#### b184
*Apr 17, 2018*

* OnionOS Update
	* Added Battery Level Monitoring w/ Power Dock 2 App
	* Implemented automatic app loading based on what is installed 

#### b183
*Apr 15, 2018*

* Power Dock 2 package update
	* Can now optionally output battery percentage as well
	* Added continuous mode of operation: report battery level at a specified interval

#### b182
*Apr 12, 2018*

* Warp Core Update
	* improved processing of connection parameters
	* fixed an issue where some WPA1 crypto ciphers weren't properly recorded
	* added debug command to wifisetup to aid in end-user wifi connection debug

#### b181
*Apr 11, 2018*

* Updated OnionOS Editor App to support making projects outside of the `/www` directory

#### b180
*Apr 10, 2018*

* Addition of OnionOS and supporting Apps (Editor and NFC-RFID Expansion)
* Update base-www to support OnionOS

#### b179
*Apr 10, 2018*

* Added `libfreefare` and associated utilites to package repo, also added as dependency to `nfc-exp` package

#### b178
*Apr 6, 2018*

* Updated `onion-ubus` and `wifi-warp-core` for better support for open and WPA1 networks

#### b177
*Apr 2, 2018*

* Added support for I2S - MAX98090 codec
* Updated Wifi Warp Core 
	* Better reconnect timing
	* Print statement when apcli0 link is up
* Added `nfc` packages to the package repo

#### b176
*Mar 26, 2018*

* SPI kernel driver - integrated patch that alleviates issues
	* full-duplex transmissions are replaced with half-duplex
	* able to transmit larger "packets" of data at a time (no longer just 16 bytes before CS deasserts
* Added Python `spidev` module
* package updates
	* console install tool - updated to work regardless of network configuration, specifically, works with ethernet connectivity now
	* i2c exp driver -  i2c library updated to fix memory overwrite bug
	* python gpio module - updated to alleviate bug with setting output gpios
	* avrdude - upped version number to avoid installing broken v6.3 
* Added `nfc-utils` and `libnfc` packages

#### b175
*Mar 26, 2018*

* Added WiFi Warp Core
	* Integrates `wifisetup` command
	* wifisetup updated to take in base64 arguments
	* Removal of old wifi-manager and driver
* Onion Console v0.1.4: works with WiFi Warp Core, better UX - uses base64 encoding to ensure more flexibility with input, fixed terminal app - now accessible regardless of network configuration
* Setup Wizard v0.3: works with WiFi Warp Core, more communicative with the user, better UX - uses base64 encoding to ensure more flexibility with input
* Onion Ubus v0.3: updates to wifi scan to reflect new data coming from wifi scan (RSSI, encryption strings), fixes for wifisetup
* Tools to perform base64 and URL encode/decode installed 
* Hostname Fix tool: compatible with Warp Core, more flexible - can work with any root basename


#### b174
*Sept 19, 2017*

* Added `power-dock2` application to Onion Package Repo
* New versions of existing packages:
	* Onion Console v0.1.3: works on mobile & other bug fixes
	* i2c-exp-driver v0.5: new api function to read without specifying an address
	* spi-gpio-driver v0.2: merged typo fix PR

#### b173
*August 29, 2017*

* Added `python-dev` package to Onion Package Repo

#### b172 
*August 24, 2017*

* Added back mountd package to firmware
* Added new `dht-sensor` program to Onion Package Repo

#### b171
*July 26, 2017*

* Added `mosquitto-client-ssl` to Onion Package Repo

#### b170
*July 19, 2017*

* Moved over to using `OnionIoT/source` repo

#### b169
*July 10, 2017*

* Latest version of `base-www` package added to Onion Package Repo
* Added installation of `default-jdk` to `onion/lede` Dockerfile
* Added nfc-utils to Onion Package Repo

#### b168
*July 7, 2017*

* Added `classpath` and `classpath_tools` packages to Onion Package Repo

#### b167
*July 7, 2017*

* Added Node Red IBM Watson IoT software package to Onion Package Repo

#### b166
*July 7, 2017*

Build system fix-up
* Switched to LEDE 17.01 since it's more stable
* Switched over to new Omega2 Master branch for Onion package feed

#### b165
*April 20, 2017*

* Fixed ogps package, dms to decimal conversion fixed, latitude and longitude is now decoded correct!

#### b164
*April 18, 2017*

* Updated infrastructure for easier building in docker container

#### b163
*March 6, 2017*

* Added shadow and sudo modules for user management to Onion Repo

#### b162
*March 3, 2017*

* Added Kernel Modules for Atheros 9k wireless chipsets (largely USB-based) to Onion repo

#### b161
*March 2, 2017*

Added following packages to Onion repo:
* avahi-utils
* midnight commander (mc)
* strongswan vpn

#### b160
*February 28, 2017*

**Updating to version 0.1.10 - Major Omega2 Bugs fixed**

* Fixed `fast-gpio`, `/dev/mem` device is now included in the hardware again
* Included POSIX MQUEUE config in firmware

#### b159
*February 28, 2017*

* Added tcpdump package to Onion repo

#### b158
*February 28, 2017*

* Added python twisted library to Onion repo

#### b157
*February 21, 2017*

* Added kernel modules and packages required for USB cellular modem

#### b156
*February 16, 2017*

* Overhauled Node modules for Relay, PWM, and OLED Expansions

#### b155
*February 8, 2017*

* Added mosquitto broker package to repo

#### b154
*February 6, 2017*

* Fix for issue with hostname fixing tool that popped up, now works correctly in all cases

#### b153 
*February 6, 2017*

* Now also compiling the `bc` package for the repo

#### b152
*February 3, 2017*

* Blynk Library now at v0.4.5

#### b151
*February 2, 2017*

* Added patch to fix 1-wire master issue where virtual nodes were not accepted as gpios
* Infra: added option for quick (essential) compile
* Infra: reorganized patches

#### b150 
*January 20, 2017*

* Addition of micropython and several Python modules to package repo:
    * pillow
    * urllib3
    * simplejson

#### b149
*January 19, 2017*

* `device-client` fix: resolves issue where the Omega cannot successfully connect to the Onion cloud

#### b148
*January 18, 2017*

* Compiling filesystem utility packages for repo:
    * block-mount
    * e2fsprogs
    * f2fsck, f2fs-tools
    * mk2fs
    * ntfs-utils
    * ntfs-3g, ntfs-3g-low, ntfs-3g-utils
    * sysfsutils
    * tune2fs

#### b147
*January 17, 2017*

* Compiling additional packages for repo:
    * PHP: Enabled PHP7 filter extension
    * Perl: Added a slew of Perl packages

#### b146
*January 17, 2017*

* Now using official LEDE implementation of Onion Omega2 and Omega2+ since Onion's PR was merged!
    * Got rid of Omega patches
    * The reset button on the Docks now works!
* Omega LED is now called omega2:amber:system and omega2p:amber:system
    * Updated uci-defaults files to account for this when setting system.@led[0]

#### b145
*January 12, 2017*

* Added Perl interpreter as a package

#### b144
*January 12, 2017*

**Updating to version 0.1.8 - Major Omega2 Bugs fixed**

* just upped the version number

#### b143
*January 12, 2017*

* Added kernel patch for SD card - Omega2+ can now read SD cards!

#### b142
*January 12, 2017*

* Added patch for mtd device driver - Omega2+ can now reboot!

#### b141
*January 11, 2017*

* Updated kernel to include DEV_KMEM - fix for broken fast-gpio
* Added configfs kernel module

#### b140
*December 30, 2016*

* Updated Console Support for Node Red
* Added Callback Functionality For Expansion Node Modules

#### b139
*December 29, 2016*

**Updating to version 1.7.0 - Onion Repos Up & Running**

* Now deploying all compiled packages to repo.onion.io
    * Associated deploy package script changes
    * Associated changes to uci defaults script that configures the opkg distfeeds configuration
    * Addition of new key pair for Onion repos
* Added makefile steps to allow for easier buildroot automation
* Makefile now overrides default lede packages with onion-customized makefiles for avrdude, pulse audio, and bluez
* Addition of MANY compiled packages, notable additions:
    * Blynk Library
    * PulseAudio, Bluez, Shairport-sync
    * npm
    * All python and python3
    * php7
    * ruby
    * mosquitto
    * iodine
    * samba
    * coreutils
* Added instructions on how to work with `usign`

#### b138, b137
*December 28, 2016*

* Fixed compilation of Onion's Node v4.3.1
* Building all Node packages as well:
    * OLED, Relay, PWM Expansion Add-ons
    * onoff - for GPIOs
    * nodeusb
    * Blynk Library

#### b136
*December 11, 2016*

* Upped to version 1.6.0
* Addition of NodeJs as language

#### b135
*December 6, 2016*

* Now using full `wpad` package (instead of `wpad-mini`)
    * Built in support for AES encrypted networks
    * Support for enterprise encrypted wifi
* Console Updates
    * Hostname fix
    * Wifi network setup fix
    * Wifi ap setup fix

#### b134
*December 1, 2016*

* Added patch for led morse trigger to buildroot

#### b133
*November 30, 2016*

* Added support for controlling the Omega LED using sysfs
    * LED flashes during boot and then turns solid after about 90 seconds

#### b132
*November 25, 2016*

* Console:
    * Install tool runs properly on boot
    * No longer runs as a service as a result
    * Console cloud registration works similarly to setup wizard
* Wifisetup checks for password lengths to match encryption types
* Setup Wizard runs console-install-tool when install console is checked off
* Arduino Dock 2 package renamed

#### b131
*November 24, 2016*

* Firewall and network changes to allow ethernet use on Omega2 and Omega2+
    * uci-default sets a new config for ethernet
    * Network config file specifies `eth0` as the `wan` device instead of `eth0.2`
* Console revamped
    * Install pages styled
    * Install tool now checks for and installs the app
    * Webcam app points to correct IP address for stream
* Wifimanager no longer boots twice
* Wifisetup command line fixes

#### b130
*November 22, 2016*

* Setup Wizard changes for cloud device registration
    * Setup wizard now waits for message from modal to continue onto the next step
    * Cloud Device registration fits the entire body of the modal rather than having a small border

#### b129
*November 22, 2016*

Support and differentiation for Omega2 and Omega2+
* Created separate profiles for the two devices
    * Different Flash sizes
    * Different memory sizes
    * Different model names
* Updated sh lib to include both devices, includes updates to:
    * oupgrade
    * onion ubus (affects wifimanager)
* Updates to oupgrade process, now inlcudes Omega2+
* Infrastructure update for handling new firmwares

#### b128
*November 20, 2016*

* Updated hostname fix tool to reset WiFi AP to have the correct name

#### b127
*November 18, 2016*

* Fixed firewall: now forwards from wwan to wlan by default
* Updated Setup Wizard modals
* Console Installation tool now restarts rpcd to complete base console installation

#### b126
*November 18, 2016*

* Latest code for Setup Wizard, onion ubus wifi scan, console install tool
* Added Console packages

#### b125
*November 18, 2016*

**Updating to version 0.1.5 - Omega2 Factory Firmware**

* Added support for detecting SD Card plug-in

#### b124
*November 18, 2016*
* Enable AP at boot

#### b123
*November 17, 2016*

Infrastructure updates:
* Uploading to Onion repo server is now 100% working

#### b122
*November 16, 2016*

* Update to buildroot cleanup
* Added tool to fix Omega-0000 hostname issue

#### b121
*November 11, 2016*

Big update with many fixes:
* Switched back to using closed-source Ralink WiFi driver
    * Removed the open source drivers - they were making throughput really low
* Updated `/etc/config` files to suit the new wifi driver
* Updated package feeds to use Onion Omega2 feed
* Now using new wifimanager
    * Dropped wdb40
* Setup Wizard is installed by default - Console is installed later through opkg
* Updated onion uci config to suit the new console
* Made updates to how the device hostname is made - needs more work
* Fixed wget from https addresses
* Fixed uhttpd

#### b120
*October 24, 2016*

* Using Onion customized mt76 wifi kernel driver

#### b119
*October 24, 2016*

* Updates to dependencies required for filesystem packages

#### b118
*October 10, 2016*

* Updated infrastructure scripts with some documentation

#### b117
*October 10, 2016*

* Fixed compilation issue due to missing NLS support

#### b116
*October 7, 2016*

* Fixed kernel patch to do fewer things, just focusing on `/dev/i2c-0` and `/dev/mem`

#### b115
*October 7, 2016*

* Building a variety of packages found in the found in the original Omega firmware

#### b114
*Sept 22, 2016*

* Added kernel configuration patch:
    * Enables `/dev/i2c-0` and `/dev/mem` devices, among other things

#### b113
*Sept 21, 2016*

* Added additional kernel modules and baseline packages
* Not working
    * I2C - no `/dev/i2c-0` device
    * wdb40 - likely needs code update
    * Need to test SPI, UART1, SD Card, I2S
    * Omega LED not showing up in `/sys/class/leds`

#### b112
*Sept 21, 2016*

* Added Onion packages, not everything is working, need to investigate which packages need to be added again

#### b111
*Sept 21, 2016*

* Finalized base system files for correct network operation
* Producing firmware with 100% working WiFi

#### b110
*Sept 20, 2016*

* Testing changes to base system files for correct network operation

#### b109
*Aug 29, 2016*

* Moved away from OpenWRT to LEDE Buildroot

#### b108
*Aug 24, 2016*

* Manually added required kernel modules and packages for mt7688

#### b107
*Aug 10, 2016*

* Console Updates for compatibility with Omega2
    * Status App: different specs, and different methods for gathering data
    * Settings App: different method for setting WiFi config
    * GPIO App: updated image and GPIO layout for Omega2
    * Fix for shellinabox compilation

#### b106
*Aug 5, 2016*

* Fixed compilation for shellinabox, added to firmware
* Package selection largely matches the Omega1 buildroot
    * Not compiling some node packages due to errors

#### b105
*Aug 4, 2016*

**First firmware to be added to image repo!**

* Fixes for wdb40 program and script: should work 100% now
* Added Console to the firmware
* Changed device type to omega2
* Infrastructure updates: updated Firmware API calls

#### b104
*Aug 3, 2016*

* Updated `network` and `wireless` configuration files, now works out of the box with wdb40 and sta networks
* Updates to oupgrade to differentiate between the different device types and select different repos to look for firmware updates
* Fixes to makefile for applying Omega2 patch

#### b103
*Aug 2, 2016*

Wifi works!
* Enabled wifi driver bake-in

#### b102
*Aug 2, 2016*

* Generated new signature for Omega2 package repo (for repo as well as on device)
* Updated deploy scripts to push packages and images to Omega2 repo

#### b101
*Jul 29, 2016*

* Added omega2-ctrl utility for low-level hardware control
* added libmraa as package to build

#### b100
*Jul 27, 2016*

Initial Omega2 Firmware


***Original Omega Firmware below***

#### b330
*Jul 8, 2016*

**Firmware 0.1.4 Release**

* Fixed issue where keys for Onion Package Repo were not being installed by default

#### b329
*Jul 6, 2016*

* Finalized power-dock application for Power Dock release

#### b328
*Jun 24, 2016*

**Firmware 0.1.3 Release**

* Updated device-client architecture to support additional setup modes and compatibility with Cloud services

#### b327, b326
*Jun 13, 2016*

* Updated I2C Exp Driver relay expansion w/ cli usage instructions

#### b325
*Jun 10, 2016*

**Firmware 0.1.2 Release**

Console Updates:
* Fixed dropdown issue in GPIO and OLED apps

Firmware Updates:
* Added onion-helper, a c-based ubus service to facilitate better cloud interaction
* Updated I2C Expansion Drivers, specifically the Relay
    * Extended to use all GPIOs available on MCP chip
    * Can now read relay state
    * Updates to ubus service to allow reading relay state
* Removed power-dock testing application from firmware

#### b324
*May 31, 2016*

* Added power-dock application to firmware (for testing purposes)

#### b323
*May 31, 2016*

* I2C Expansion Driver version upped to 0.3
    * relay-exp app can now:
    * Control all 8 MCP chip GPIOs
    * Accept full I2C device address from the command line

#### b322, b321
*May 29, 2016*

* Library management script update: changed structure of output

#### b320, b317, b316
*May 24, 2016*

* Added hooks for build server and library management

#### b315
*May 20, 2016*

* Added I2C Expansion Node modules

#### b314, b313
*May 18, 2016*

* Added rpcsys ubus service package to firmware
* Updated onion-helper ubus service download function
    * Better error handling
    * Ability to run download in background

#### b312
*May 17, 2016*

* Updates to onion-helper ubus service

#### b311, b310
*May 12, 2016*

* Adding Transmission Console App to onion repo

#### b309
*May 9, 2016*

* Adding onion-helper c-based ubus service as a package

#### b308
*April 29, 2016*

**Firmware 0.1.1 Release**

Console Updates
* Now at Brin M1 codename
* Settings App will no longer be cached

#### b307
*April 28, 2016*

* device-client version upped to 0.5
    * Push out updates that fix connection hang in the event of a network outage

#### b306
*April 20, 2016*

* updates to the sh library to fix bug where blank tmp files kept getting created
* updates to fast-gpio to fix blank tmp files bug
* added ubus service to launch processes in the background

#### b304
*April 15, 2016*

* device-client version upped to 0.4
    * Push out the updates that fix the curl and threading memory leaks
* Updates to firmware database access scripts to decrease verbosity

#### b303
*April 13, 2016*

Build Server related:

Updated firmware database access scripts to match new firmware database web API

#### b302
*April 11, 2016*

Added `ogps` to the onion packages repo.

#### b301
*April 10, 2016**

Added script to use with buildroot automated compilation: Checks compile exit codes and retries up to 2 additional times, returns last compile exit code

#### b300
*April 8, 2016*

**Version 0.1.0 Firmware**

* Added support for the Onion Cloud
* Updated to latest version of the Console
    * Settings App: Added Cloud Settings tab
    * OLED App can now save converted images to the Omega

#### b299
*April 4, 2016*

* New wdb40 version: fixed issue with connecting to SSIDs with spaces

#### b297
*April 4, 2016*

* Added `tmux` back into the firmware
* New device-client version for repo: fixed device ID and secret location

#### b296
*April 2, 2016*

**Version 0.0.8 Firmware**

Updated to latest version of Console and WDB40

Console:
* Added Calculator and Webcam Apps
* Overhauled the WiFi Settings Apps

WDB40:
* Updated usage instructions

#### b295
*April 2, 2016*

* Removed deprecated ubus-intf package (functionality replaced by onion-ubus gpio method, saving space)
* Replaced wifisetup and wifisaint packages with wdb40 network manager
* Updated onion-ubus with new wdb40 install paths

#### b294
*April 2, 2016*

* Added Onion customized bluez and pulseaudio packages to Onion Repo
* Modified deployment script to correctly upload images and packages

#### b293
*Mar 31, 2016*

Added kmod-input-uinput package into the build.

#### b290, b289
*Mar 30, 2016*

* Kernel config update now done by appending to config file from script (instead of overwriting), increased build stability
* Added uClibc toolchain patch that makes web requests default to IPv4 (Required for NodeJS to properly get HTML)
* Updated onion-ubus: modified and fixed gpio method

#### b288
*Mar 19, 2016*

Added Onion GPIO Python module package to Onion Repo

#### b283
*Mar 9, 2016*

Fix for failing build (caused by having some new options missing in the kernel config)

#### b282
*Mar 7, 2016*

Updates to the Onion SPI library
* better handling of device registration
* usage printout is nicer

#### b281
*Mar 5, 2016*

* Added NodeJS v4.3.1 as a package
    * Along with associated kernel changes
* Updates to Onion SPI library
* arduino-dock, avrdude, and tmux packages are no longer included in the build

#### b278
*Feb 23, 2016*

* Now building onion I2C python library as an optional module (in onion repo)
* Added Onion spi library, command line spi tool, and python module as modules in repo

#### b277
*Feb 22, 2016*

I2C Expansions Software Updates
* Extended oled library functionality
    * functions for: text/image column addressing, writing a single byte to the screen, setting cursor by row and pixel
* Implemented Python object for the i2c library
* Updated all c-python modules with better error checking:
    * checks for correct arguments
    * checks for successful C function calls

#### b275
*Jan 27, 2016*

Added `whois` command to Busybox

#### b271
*Jan 25, 2016*

Updates to firmware database access scripts

#### b270
*Jan 25, 2016*

Added `kmod-w1-slave-therm` kernel package for One-Wire temperature sensor family. Updated PATH variable to match new OpenWRT default.

#### b269
*Jan 21, 2016*

Added `kmod-dma-buf` package to resolve build issue

#### b267
*Jan 20, 2016*

Added one-wire kernel modules:
* `kmod-w1`
* `kmod-w1-gpio-custom`
* `kmod-w1-master-gpio`

#### b266
Added gpio-test package to Onion Package Repo

#### b265
Enabled GPIO Edge irq patch

Kernel Modules added:
* kmod-bluetooth
* kmod-gpio-irq - enable interrupts from gpio

#### b264
**Ando J1 Console Release**

Upgrading to version 0.0.6

Console Updates:
* Added Resistor Calculator App
* Implemented universal notification system
* Added error notification to I2C expansion apps if expansion cannot be found
* Cosmetic updates to Settings app, making it more user friendly
Firmware Update:
* ~~Added an additional patch for GPIO Edge irq~~
* Removed GPIO Edge irq patches

#### b262
Added support for GPIO Edge irq

#### b260
Added usb audio card kernel modules (`kmod-usb-audio` and `kmod-sound-core`)

#### b259
oupgrade update:
* added ca-certificates packages to base build
* oupgrade script now points to https version of repo info file, various code clean-up
* onion sh lib: added function to wget a file from a url

#### b258
* i2c library update: implemented writeBuffer function that takes an address and a data buffer, and then a private writeBuffer function that creates a buffer with the i2c register address at element 0, and the data in the rest of the elements, then uses this to perform the i2c write
* i2c-exp cli apps: changed verbosity option to be cumulative
* neopixel tool:
    * updated to use new i2c library writeBuffer function
    * fixed setBuffer class function: now returns valid EXIT_SUCCESS/EXIT_FAILURE

#### b256, b255
python package update: added base packages to setup OmegaExpansion and OmegaArduinoDock Python Packages

#### b254
neopixel-tool fix: set pixel can now accept hex codes with `0x` prefix

#### b252
oled-exp fix: diagonal scrolling no longer leaves out the bottom row

#### b251, b250, b249
Neopixel Class:
* added brightness control to c++ class (added to cli app, c-lib, python module, and python example)
* added overloaded Init function that sets pin, length of strip, and performs init
* fixes for dynamic memory clean-up

#### b247
Added utilities and libraries for controlling Neopixels with the Arduino Dock

#### b245
oled-exp driver fix: resolved issue with column addressing for text and images, all images should display correctly now

#### b244 - b241
**Invalid builds**
*resolving build server + strider deployment issues*

#### b240, b239
Added i2c-exp python libraries as packages

#### b238, b237, b236, b235
oled-exp changes
* fix for oledDraw: now reset the column addressing to 0-127 so images are displayed correctly
* changed name of contrasting setting function to oledSetBrightness
* fix: correctly set the cursor for columns (setcursor function now sets column addressing to 0-125 since changing the column addressing resets the column cursor pointer)

#### b234
* Added g=grep alias to profile
* Updated pwm-exp usage printout to include oscillator sleep functionality

#### b233
Console Bug Fix: permission denied error did not result in relogin

#### b232
**Ando N2 Console Release**
Upgrading to version 0.0.5
Console Updates:
* Added apps for the PWM, Relay, and OLED Expansions
* Extended the GPIO Control App to include the Expansion Dock as well
* Added a taskbar to easily switch between apps

#### b231
**Pre Console Release Test Build #2**
Fixes:
* Base system
    * Typo fix for ll alias
    * Onion Repo Key: now installs with correct name and as a signed signature file

#### b230
**Pre Console Release Test Build**
Added functionality:
* Onion UBUS Service
    * added i2c-scan functionality: get a list of addresses of all connected I2C slaves
* I2C Expansions
    * pwm-exp
    * Implemented ability to set oscillator to sleep (disable all pwm signals)
    * Added check to see if oscillator is running, if not, start it up (automatic initialization, running with -i at the beginning is no longer necessary)
    * oled-exp
    * Developed driver for all oled exp functionality
      * init, clear, turn display on/off, invert display, dim the display, set the cursor, writing a message, scrolling (horizontal and diagonal), displaying an image
    * ubus integration for all three apps (pwm-exp, relay-exp, oled-exp)
    * created static libraries of the driver code for all three expansions (pwm, relay, oled)
* SH Library
    * json argument parser: added better parsing, now wraps values with double quotes if they have certain special characters
* Packages
    * Added Onion specific keys for signing Onion packages
    * Compiled NodeJs v0.10.5, available as a package in Onion Repo
* Base System
    * Will now preserve /root during a sysupgrade (firmware update)
    * Enabled access to the Onion Package Repo (opkg feed setup)
    * Added automatic ll alias
* Deploy
    * Sign Onion Repo packages with Onion key
    * Deploy all packages to the Onion Repo

#### b229
Integration of i2c-exp-drivers into ubus via rpcd

#### b228
Package repo keys feed now uses the keys from the Buildroot (this) Repo

#### b227
Resolved conflict with OpenWRT NodeJS package, fixes for packages signature generation

#### b225
Moved keys to buildroot repo. Changed scripts to push new signature of Packages file.

#### b223
Added nodejs package, deploy also pushes Onion packages to the repo

#### b222
Added signature keys for Onion Package Repo

#### b221
* Arduino Dock update: added placeholder lua sketch that is used by Arduino IDE when flashing
* i2c library: updated read function to return a buffer array, meant to increase compatibility as a library
* debug library: added an onion debug library, implements print statements with a severity level for now
* i2c-exp-drivers: updated to reflect i2c lib changes and debug lib addition
* ads1x15 driver: updated to reflect i2c lib changes and debug lib addition

#### b220
Added arduino-dock package: all utilities required for use of Arduino Dock

#### b219
Console GPIO App fix

#### b217
Added avrdude programmer and its' dependencies to build

#### b216
Added kmod-hid, kmod-usb-hid, and kmod-usb-printer modules to build.

#### b214
New console release.
Incrementing version number.

#### b213
Added sftp server module

#### b212
* updates to i2c lib, fixed issue with write/read function status return
* added driver for ads1x15 chip as built-in module
* added modules kmod-video-uvc and mjpg-streamer for use with USB webcams

#### b211
* fast-gpio: implemented print verbosity control, along with json output support
* onion ubus: added fast-gpio support (pwm times-out)

#### b209
* i2c library: headers and static library files are now properly installed and can be used by other packages
* added app to use ADS1X15 ADC+PGA chip

#### b208
* i2c library: implemented output verbosity and some code clean-up
* sh lib: added clean-up of empty log files
* added chpasswd utility to busybox

#### b207
i2c lib: expanded to perform multi-byte i2c read and write operations

#### b206, b205
relay-exp tool now supports different dip-switch settings to change I2C address, added better error checking

#### b204
Changes to i2c expansions:
* changed the repo name
* clean-up of pwm-exp print outs
* exposed i2c and mcp23008 libraries

#### b203, b202
Added login utility for busybox
Changed shellinabox to launch busybox login utility

#### b199
Fix for shellinabox default PATH variable

#### b198, b197
Added relay-exp application driver for the Relay expansion
Added i2c-tools as built-in module

#### b196
Infra update: automatically updates the firmware db when a new image is released

#### b193
New version of pwm-exp:
* fix for setting pwm frequency
* command line arguments can now be float
* supports only doing pwm init from command line
* added input mode where pulse width and period can be used
* added error checking for i2c reads

#### b192, b191
Added support for ext2/3/4 filesystems

#### b190
New version of fast-gpio: now disables running pwm if set or set-direction commands are used

#### b189, b188
Added Onion Console, removed setup wizard. Incremented to version 0.0.3

#### b187
Removed Pwm Expansion test script

#### b186, b185
Temporary: added test script for pwm expansion

#### b184
Changed shellinabox default css

#### b183, b182
Added Onion PWM Expansion Driver

#### b181
Updated shellinabox daemon management again: enabled auto-start

#### b180
Added i2c-tools package as a module

#### b179, b178
Login banner and shellinabox changes
179: Updated shellinabox package to pull from Onion repo, fixed issue with build setup script
178: Changed login banner, changes to shellinabox daemon management (no longer auto-starts or auto-respawns)

#### b177
Onion sh lib updates for use with the Console

#### b176
Added GPS expansion usb drivers

#### b175
Added several usb-serial drivers

#### b174, b173, b172
Fixes for fast-gpio
* b174: added ability to set and read direction of pins

#### b171
Added libugpio and gpioctl packages to the build

#### b170
Following changes:
* oupgrade: added build number to text-based check
* wifisetup: added option to specify Omega's IP addr when setting up AP network
* added fast-gpio
* added onion-sh-lib, only used by fast-gpio for now

#### b169
Added fix for opkg sources

#### b168
Added omega-led service to onion-ubus
Enables ubus control of the LED built-in to the Omega

#### b167, b166
Updates to oupgrade and onion-ubus
* oupgrade: JSON output update: Now outputs build numbers for device and repo, and build_mismatch boolean (if the device and repo are the same version, checks the build numbers)
* Added dir-list to onion ubus service: returns an array of all directories within a specified directory

#### b165, b164, b163
Added shellinabox to the build. Shellinabox provides an AJAX interface to the command line shell.

#### b159
Updated to Version 0.0.2
