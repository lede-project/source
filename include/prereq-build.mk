#
# Copyright (C) 2006-2012 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/prereq.mk
include $(INCLUDE_DIR)/host.mk
include $(INCLUDE_DIR)/host-build.mk

SHELL:=sh
PKG_NAME:=Build dependency


# Required for the toolchain
$(eval $(call TestHostCommand,working-make, \
	Please install GNU make v3.81 or later. (This version has bugs), \
	$(MAKE) -v | grep -E 'Make (3\.8[1-9]|3\.9[0-9]|[4-9]\.)'))

$(eval $(call TestHostCommand,case-sensitive-fs, \
	LEDE can only be built on a case-sensitive filesystem, \
	rm -f $(TMP_DIR)/test.*; touch $(TMP_DIR)/test.fs; \
		test ! -f $(TMP_DIR)/test.FS))

$(eval $(call SetupHostCommand,gcc, \
	Please install the GNU C Compiler (gcc), \
	$(CC) --version | grep gcc, \
	gcc --version | grep gcc, \
	gcc49 --version | grep gcc, \
	gcc48 --version | grep gcc, \
	gcc47 --version | grep gcc, \
	gcc46 --version | grep gcc, \
	gcc --version | grep Apple.LLVM ))

$(eval $(call TestHostCommand,working-gcc, \
	Please reinstall the GNU C Compiler - it appears to be broken, \
	echo 'int main(int argc, char **argv) { return 0; }' | \
		gcc -x c -o $(TMP_DIR)/a.out -))

$(eval $(call SetupHostCommand,g++, \
	Please install the GNU C++ Compiler (g++), \
	$(CXX) --version | grep g++, \
	g++ --version | grep g++, \
	g++49 --version | grep g++, \
	g++48 --version | grep g++, \
	g++47 --version | grep g++, \
	g++46 --version | grep g++, \
	g++ --version | grep Apple.LLVM ))

$(eval $(call TestHostCommand,working-g++, \
	Please reinstall the GNU C++ Compiler - it appears to be broken, \
	echo 'int main(int argc, char **argv) { return 0; }' | \
		g++ -x c++ -o $(TMP_DIR)/a.out - -lstdc++ && \
		$(TMP_DIR)/a.out))

$(eval $(call TestHostCommand,ncurses, \
	Please install ncurses. (Missing libncurses.so or ncurses.h), \
	echo 'int main(int argc, char **argv) { initscr(); return 0; }' | \
		gcc -include ncurses.h -x c -o $(TMP_DIR)/a.out - -lncurses))

ifeq ($(HOST_OS),Linux)
  zlib_link_flags := -Wl,-Bstatic -lz -Wl,-Bdynamic
else
  zlib_link_flags := -lz
endif

$(eval $(call TestHostCommand,zlib, \
	Please install a static zlib. (Missing libz.a or zlib.h), \
	echo 'int main(int argc, char **argv) { gzdopen(0, "rb"); return 0; }' | \
		gcc -include zlib.h -x c -o $(TMP_DIR)/a.out - $(zlib_link_flags)))

# Xcode deprecated openssl, MacPorts doesn't work nicely for other packages
ifneq ($(HOST_OS),Darwin)
$(eval $(call TestHostCommand,libssl, \
	Please install the openssl library (with development headers), \
	echo 'int main(int argc, char **argv) { SSL_library_init(); return 0; }' | \
		gcc $(HOST_CFLAGS) -include openssl/ssl.h -x c -o $(TMP_DIR)/a.out - -lcrypto -lssl $(HOST_LDFLAGS)))
endif

$(eval $(call TestHostCommand,perl-thread-queue, \
	Please install the Perl Thread::Queue module, \
	perl -MThread::Queue -e 1))


$(eval $(call SetupHostCommand,tar,Please install GNU 'tar', \
	gtar --version 2>&1 | grep GNU, \
	gnutar --version 2>&1 | grep GNU, \
	tar --version 2>&1 | grep GNU))

$(eval $(call SetupHostCommand,find,Please install GNU 'find', \
	gfind --version 2>&1 | grep GNU, \
	find --version 2>&1 | grep GNU))

$(eval $(call SetupHostCommand,bash,Please install GNU 'bash', \
	bash --version 2>&1 | grep GNU))

$(eval $(call SetupHostCommand,patch,Please install GNU 'patch', \
	gpatch --version 2>&1 | grep 'Free Software Foundation', \
	patch --version 2>&1 | grep 'Free Software Foundation'))

$(eval $(call SetupHostCommand,diff,Please install diffutils, \
	gdiff --version 2>&1 | grep diff, \
	diff --version 2>&1 | grep diff))

$(eval $(call SetupHostCommand,cp,Please install GNU fileutils, \
	gcp --help, \
	cp --help))

$(eval $(call SetupHostCommand,seq,, \
	gseq --version, \
	seq --version))

$(eval $(call SetupHostCommand,awk,Please install GNU 'awk', \
	gawk --version 2>&1 | grep GNU, \
	awk --version 2>&1 | grep GNU))

$(eval $(call SetupHostCommand,grep,Please install GNU 'grep', \
	ggrep --version 2>&1 | grep GNU, \
	grep --version 2>&1 | grep GNU))

$(eval $(call SetupHostCommand,getopt, \
	Please install an extended getopt version that supports --long, \
	gnugetopt -o t --long test -- --test | grep '^ *--test *--', \
	/usr/local/bin/getopt -o t --long test -- --test | grep '^ *--test *--', \
	getopt -o t --long test -- --test | grep '^ *--test *--'))

$(eval $(call SetupHostCommand,stat,Cannot find a file stat utility, \
	gnustat -c%s $(TMP_DIR)/.host.mk, \
	gstat -c%s $(TMP_DIR)/.host.mk, \
	stat -c%s $(TMP_DIR)/.host.mk))

$(eval $(call SetupHostCommand,md5sum,, \
	gmd5sum /dev/null | grep d41d8cd98f00b204e9800998ecf8427e, \
	md5sum /dev/null | grep d41d8cd98f00b204e9800998ecf8427e, \
	$(SCRIPT_DIR)/md5sum /dev/null | grep d41d8cd98f00b204e9800998ecf8427e))

$(eval $(call SetupHostCommand,unzip,Please install 'unzip', \
	unzip 2>&1 | grep zipfile, \
	unzip))

$(eval $(call SetupHostCommand,bzip2,Please install 'bzip2', \
	bzip2 --version </dev/null))

$(eval $(call SetupHostCommand,wget,Please install GNU 'wget', \
	wget --version | grep GNU))

$(eval $(call SetupHostCommand,perl,Please install Perl 5.x, \
	perl --version | grep "perl.*v5"))

$(eval $(call SetupHostCommand,python,Please install Python 2.x, \
	python2.7 -V 2>&1 | grep Python, \
	python2 -V 2>&1 | grep Python, \
	python -V 2>&1 | grep Python))

$(eval $(call SetupHostCommand,svn,Please install the Subversion client, \
	svn --version | grep Subversion))

$(eval $(call SetupHostCommand,git,Please install Git (git-core) >= 1.7.12.2, \
	git --exec-path | xargs -I % -- grep -q -- --recursive %/git-submodule))

$(eval $(call SetupHostCommand,file,Please install the 'file' package, \
	file --version 2>&1 | grep file))

ifneq ($(HOST_OS),Darwin)
$(eval $(call SetupHostCommand,openssl,Please install the 'openssl' utility, \
	openssl version | grep '\(OpenSSL\|LibreSSL\)'))
endif


# Install ldconfig stub
$(eval $(call TestHostCommand,ldconfig-stub,Failed to install stub, \
	touch $(STAGING_DIR_HOST)/bin/ldconfig && \
	chmod +x $(STAGING_DIR_HOST)/bin/ldconfig))
