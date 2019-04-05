# Onion Omega2 Firmware Build System

This buildsystem for the LEDE Linux distribution has been modified by Onion Corporation to build firmware for the Onion Omega2 family of devices.

![Omega2](https://github.com/OnionIoT/Onion-Media/raw/master/Product%20Photos/Omega2/OM2-2.jpg)

**Onion Corporation is not responsible for any damage to your device caused by using custom firmware or packages not built and released by Onion Corporation.**

## Notes

* This build system generates firmware for the M7688 SoC with Onion's Enhanced Warp Core WiFi driver 
	* It is available as an [OpenWRT package](https://github.com/OnionIoT/OpenWRT-Packages/tree/openwrt-18.06/wifi-warp-core) for systems running Linux Kernel 4.14.81
		* **See [this post for more details on the Warp Core](https://onion.io/2bt-brand-new-os-release/)**

## Additional Reading

* See `CHANGELOG.md` for a listing of the changes for each firmware version and build
* See `DISCLAIMER.md` for Onion's disclaimer regarding this build system

# Using this Build System

This buildsystem can be used to create packages and firmware for the Omega2 device. There are two preferred ways to use the buildsystem:

1. Using the `onion/omega2-source` Docker image **Recommended method**
2. Running the build system on a Linux system

**We strongly recommend first building firmware with our configuration and THEN adding your own customizations** 

## Using the Docker Image

The Docker image takes care of all environmental configuration and is the recommended method. We recommend getting familiar with how Docker before trying this procedure.

Procedure:

1. Install Docker on your system: https://www.docker.com/get-started
2. Pull our Docker image by running:
```
docker pull onion/omega2-source
```
3. Once the image has been pulled to your computer, you can run your own container based on the image: 
```
docker run -it onion/omega2-source /bin/bash
```
4. Your container will now be up and running!

5. **Optional Step:** Configure the build system for a minimal build - only build packages that are included in the firmware - results in a much faster compile time
```
sh scripts/onion-minimal-build.sh
```

6. Compile the build system by running:
```
make
```

> **We recommend running Docker on a Linux system**. Some users have reported compilation issues when running Docker on Windows and Mac OS.

For more details, see: https://onion.io/2bt-cross-compiling-c-programs-part-1/

## Using a Linux System

**Not recommended for beginners** 

Procedure to setup the build system on a Linux System (Ubuntu Distro):

1. Setup Linux environment by installing the required packages:
```
sudo apt-get update
sudo apt-get install -y build-essential vim git wget curl subversion build-essential libncurses5-dev zlib1g-dev gawk flex quilt git-core unzip libssl-dev python-dev python-pip libxml-parser-perl default-jdk
```

2. Download the Build System from Github:
```
git clone https://github.com/OnionIoT/source.git
cd source
```

3. Prepare build system:
```
sh scripts/onion-feed-setup.sh
git checkout .config
```
> This will initialize & configure all the package feeds as well as setup the `.config` file to match this repo. With these commands, the firmware built will match the official firmware released by Onion.

4. **Optional Step:** Configure the build system for a minimal build - only build packages that are included in the firmware - results in a much faster compile time
```
sh scripts/onion-minimal-build.sh
```

5. Compile Build System:
```
make -j
```

## Updating the Build System
*Applies to both Docker and Linux System installations*

Once you've setup your build system and have been using it, you'll want to update it from time to time.

### Updating the Package Makefiles

To grab the latest package makefiles for the [Onion Package Repo](https://github.com/OnionIoT/openwrt-packages), run this command:

```
./scripts/feeds update onion
```

> Run this command if you're seeing compilation issues errors in Onion packages, this will likely fix the problem. If not, let us know on the [Onion Community](http://community.onion.io/).

### Grabbing the Latest Build System Code

To grab the latest code from this repo, run:

```
git pull
```

If you've made modifications to which packages are built/included in the firmware, these changes will be reflected in your local `.config` file. 

**Create a backup copy of your `.config` file before proceeding.** The `git pull` command will not work unless the `.config` file is restored to it's original state by running `git checkout .config` and running `git pull` again. **Note that this will REMOVE all of your customizations!** 


## Troubleshooting

If you're encountering errors during compilation, you'll likely see something like the following:

```
make -r world: build failed. Please re-run make with -j1 V=s to see what's going on
/root/source/include/toplevel.mk:198: recipe for target 'world' failed
make: *** [world] Error 1
```

To get more visibility into the error, re-run your compilation as the error prompt suggests:

```
make -j1 V=s
```

The error messages will point you in the direction of the package responsible for the compilation error.
* If the problematic package is from Onion, see the [Updating the Package Makefiles](#updating-the-build-system) section above. This will likely resolve your issue.
* If the problematic package is not related to Onion packages, it's likely due to environment issues or issues with the code. Please check with the original authors to get pointers for debugging.

> For Docker users, note that **we recommend running Docker on a Linux System**. Some users have reported compilation issues when running the build system in Docker on Windows and Mac OS, that are not observed on Docker on Linux.

# OpenWRT Linux Distribution

  _______                     ________        __
 |       |.-----.-----.-----.|  |  |  |.----.|  |_
 |   -   ||  _  |  -__|     ||  |  |  ||   _||   _|
 |_______||   __|_____|__|__||________||__|  |____|
          |__| W I R E L E S S   F R E E D O M
 -----------------------------------------------------

This is the buildsystem for the OpenWrt Linux distribution.

To build your own firmware you need a Linux, BSD or MacOSX system (case
sensitive filesystem required). Cygwin is unsupported because of the lack
of a case sensitive file system.

You need gcc, binutils, bzip2, flex, python, perl, make, find, grep, diff,
unzip, gawk, getopt, subversion, libz-dev and libc headers installed.

1. Run "./scripts/feeds update -a" to obtain all the latest package definitions
defined in feeds.conf / feeds.conf.default

2. Run "./scripts/feeds install -a" to install symlinks for all obtained
packages into package/feeds/

3. Run "make menuconfig" to select your preferred configuration for the
toolchain, target system & firmware packages.

4. Run "make" to build your firmware. This will download all sources, build
the cross-compile toolchain and then cross-compile the Linux kernel & all
chosen applications for your target system.

Sunshine!
	Your OpenWrt Community
	http://www.openwrt.org


