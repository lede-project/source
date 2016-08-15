ARCH:=mips64el
CPU_TYPE:=mips64
SUBTARGET:=le64
BOARDNAME:=Little Endian (64-bits)
FEATURES:=ramdisk

define Target/Description
	Build LE firmware images for MIPS Malta CoreLV board running in
	little-endian and 64-bits mode
endef
