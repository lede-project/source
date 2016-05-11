include $(TOPDIR)/rules.mk

PKG_NAME:=prefer-full
PKG_VERSION:=1.0
PKG_RELEASE=1

PKG_LICENSE:=MIT
PKG_LICENSE_FILES:=

PKG_MAINTAINER:=Daniel Dickinson <cshorewrt@daniel.thecshore.com>

include $(INCLUDE_DIR)/package.mk

define Package/prefer-full
  SECTION:=base
  CATEGORY:=Base system
  PKGARCH:=all
  DEPENDS:= @FEED_packages \
+bridge \
+bzip2 \
+cifs-utils \
+coreutils \
+coreutils-basename \
+coreutils-cat \
+coreutils-chgrp \
+coreutils-chown \
+coreutils-chmod \
+coreutils-chroot \
+coreutils-cp \
+coreutils-cut \
+coreutils-date \
+coreutils-dd \
+coreutils-dirname \
+coreutils-du \
+coreutils-echo \
+coreutils-env \
+coreutils-expr \
+coreutils-false \
+coreutils-head \
+coreutils-id \
+coreutils-ln \
+coreutils-ls \
+coreutils-md5sum \
+coreutils-mkdir \
+coreutils-mkfifo \
+coreutils-mknod \
+coreutils-mktemp \
+coreutils-mv \
+coreutils-nice \
+coreutils-printf \
+coreutils-pwd \
+coreutils-readlink \
+coreutils-rm \
+coreutils-rmdir \
+coreutils-seq \
+coreutils-sleep \
+coreutils-sort \
+coreutils-sync \
+coreutils-tail \
+coreutils-tee \
+coreutils-test \
+coreutils-touch \
+coreutils-tr \
+coreutils-true \
+coreutils-uname \
+coreutils-uniq \
+coreutils-uptime \
+coreutils-wc \
+coreutils-yes \
+diffutils-cmp \
+dmesg \
+findutils-find \
+findutils-xargs \
+grep \
+hwclock \
+ip-full \
+iputils-ping \
+iputils-ping6 \
+less \
+logger \
+mount-utils \
+netcat \
+procps-ng \
+procps-ng-free \
+procps-ng-kill \
+procps-ng-pgrep \
+procps-ng-ps \
+procps-ng-top \
+procps-ng-uptime \
+shadow-utils \
+shadow-passwd \
+swap-utils \
+tar \
+vim
  TITLE:=Pull versions of commands instead of BusyBox versions
  DEFAULT:=y if PREFER_FULL_VS_BUSYBOX
endef

define Build/Prepare
	true
endef

define Build/Compile
	true
endef

define Package/prefer-full/install
	true
endef

$(eval $(call BuildPackage,prefer-full))
