
To add the 20u5622 or uwe5622 WiFi driver to your custom OpenWRT build, you'll need to do a bit of extra work since these drivers may not be included by default in the OpenWRT source tree. Here's how you can add them:
Step 1: Download the Driver Source Code
 
    Identify the Source Code:
        First, you'll need to find the source code for the 20u5622 or uwe5622 WiFi driver. This might be available from the manufacturer or from a third-party repository like GitHub. For instance, check if the driver is available on GitHub or another trusted source.
 
    Clone or Download the Source Code:
        If the driver source code is hosted on GitHub, you can clone it:
 
        bash
 
        git clone https://github.com/username/uwe5622-driver.git
 
        Replace https://github.com/username/uwe5622-driver.git with the actual URL.
 
Step 2: Add the Driver to OpenWRT Build System
 
    Navigate to the OpenWRT Package Directory:
 
    bash
 
cd openwrt/package
 
Create a New Directory for the Driver:
 
bash
 
mkdir -p kernel/uwe5622
cd kernel/uwe5622
 
Copy the Driver Source Files:
 
    Copy the downloaded or cloned driver files into this directory.
 
Create a Makefile for the Driver:
 
    Create a Makefile in the kernel/uwe5622 directory. This will instruct OpenWRT how to compile and install the driver.
    Here is a basic structure of the Makefile:
 
 
    include $(TOPDIR)/rules.mk
 
    PKG_NAME:=uwe5622
    PKG_RELEASE:=1
 
    include $(INCLUDE_DIR)/package.mk
 
    define KernelPackage/uwe5622
      SUBMENU:=Wireless Drivers
      TITLE:=uwe5622 WiFi Driver
      FILES:=$(PKG_BUILD_DIR)/uwe5622.ko
      AUTOLOAD:=$(call AutoLoad,50,uwe5622)
    endef
 
    define Build/Prepare
      $(CP) -r $(PKG_SOURCE_DIR)/* $(PKG_BUILD_DIR)/
    endef
 
    define Build/Compile
      $(MAKE) -C $(LINUX_DIR) \
        M=$(PKG_BUILD_DIR) \
        modules
    endef
 
    define Package/uwe5622/install
      $(INSTALL_DIR) $(1)/etc/modules.d
      $(INSTALL_DATA) ./files/uwe5622.conf $(1)/etc/modules.d/
    endef
 
    $(eval $(call KernelPackage,uwe5622))
 
Add the Driver to the Build System:
 
    Modify feeds.conf.default or create a custom feed for your driver package.
 
Update the Build System:
 
bash
 
    cd openwrt
    ./scripts/feeds update -a
    ./scripts/feeds install -a
 
Step 3: Include the Driver in Your Build
 
    Re-run menuconfig:
 
    bash
 
make menuconfig
 
    Navigate to Kernel Modules -> Wireless Drivers, and you should see your uwe5622 driver listed.
    Select it to include in the build.
 
Compile the Firmware:
 
bash
 
    make -j$(nproc)
 
        This will include the uwe5622 driver in the final firmware image.
 
Step 4: Flash the Firmware and Test
 
    Flash the New Image: Use the same process as before to flash your new OpenWRT image onto the microSD card.
    Test the WiFi: Boot your Orange Pi 3B and check if the WiFi interface is available.
 
Troubleshooting
 
    If the driver doesn't compile correctly, you may need to adjust the Makefile or add additional dependencies.
    Verify the correct kernel version is used. Sometimes driver sources need modifications to work with specific kernel versions.
    Ensure the correct device tree settings for the WiFi chip are applied, if necessary.
