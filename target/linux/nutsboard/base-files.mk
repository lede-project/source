define Build/Compile
	$(call Build/Compile/Default)
	$(TARGET_CC) -o $(PKG_BUILD_DIR)/uim $(PLATFORM_DIR)/src/uim/uim.c
endef

define Package/base-files/install-target
	mkdir -p $(1)/usr/bin
	$(CP) $(PKG_BUILD_DIR)/uim $(1)/usr/bin
endef


