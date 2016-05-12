#
# Copyright (C) 2006-2011 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

CRYPTO_MENU:=Cryptographic API modules

CRYPTO_MODULES = \
	ALGAPI2=crypto_algapi \
	BLKCIPHER2=crypto_blkcipher

crypto_confvar=CONFIG_CRYPTO_$(word 1,$(subst =,$(space),$(1)))
crypto_file=$(LINUX_DIR)/crypto/$(word 2,$(subst =,$(space),$(1))).ko
crypto_name=$(if $(findstring y,$($(call crypto_confvar,$(1)))),,$(word 2,$(subst =,$(space),$(1))))

define AddDepends/crypto
  SUBMENU:=$(CRYPTO_MENU)
  DEPENDS+= $(1)
endef

define KernelPackage/crypto-aead
  TITLE:=CryptoAPI AEAD support
  KCONFIG:= \
	CONFIG_CRYPTO_AEAD \
	CONFIG_CRYPTO_AEAD2
  FILES:=$(LINUX_DIR)/crypto/aead.ko
  AUTOLOAD:=$(call AutoLoad,09,aead,1)
  $(call AddDepends/crypto, +LINUX_4_4:kmod-crypto-null)
endef

$(eval $(call KernelPackage,crypto-aead))


define KernelPackage/crypto-hash
  TITLE:=CryptoAPI hash support
  KCONFIG:=CONFIG_CRYPTO_HASH
  FILES:=$(LINUX_DIR)/crypto/crypto_hash.ko
  AUTOLOAD:=$(call AutoLoad,02,crypto_hash,1)
  $(call AddDepends/crypto)
endef

$(eval $(call KernelPackage,crypto-hash))


define KernelPackage/crypto-manager
  TITLE:=CryptoAPI algorithm manager
  DEPENDS:=+kmod-crypto-aead +kmod-crypto-hash +kmod-crypto-pcompress
  KCONFIG:= \
	CONFIG_CRYPTO_MANAGER \
	CONFIG_CRYPTO_MANAGER2
  FILES:=$(LINUX_DIR)/crypto/cryptomgr.ko
  AUTOLOAD:=$(call AutoLoad,09,cryptomgr,1)
  $(call AddDepends/crypto)
endef

$(eval $(call KernelPackage,crypto-manager))


define KernelPackage/crypto-pcompress
  TITLE:=CryptoAPI Partial (de)compression operations
  KCONFIG:= \
	CONFIG_CRYPTO_PCOMP=y \
	CONFIG_CRYPTO_PCOMP2
  FILES:=$(LINUX_DIR)/crypto/pcompress.ko
  AUTOLOAD:=$(call AutoLoad,09,pcompress)
  $(call AddDepends/crypto)
endef

$(eval $(call KernelPackage,crypto-pcompress))


define KernelPackage/crypto-user
  TITLE:=CryptoAPI userspace interface
  DEPENDS:=+kmod-crypto-hash +kmod-crypto-manager
  KCONFIG:= \
	CONFIG_CRYPTO_USER_API \
	CONFIG_CRYPTO_USER_API_HASH \
	CONFIG_CRYPTO_USER_API_SKCIPHER
  FILES:= \
	$(LINUX_DIR)/crypto/af_alg.ko \
	$(LINUX_DIR)/crypto/algif_hash.ko \
	$(LINUX_DIR)/crypto/algif_skcipher.ko
  AUTOLOAD:=$(call AutoLoad,09,af_alg algif_hash algif_skcipher)
  $(call AddDepends/crypto)
endef

$(eval $(call KernelPackage,crypto-user))


define KernelPackage/crypto-wq
  TITLE:=CryptoAPI work queue handling
  KCONFIG:=CONFIG_CRYPTO_WORKQUEUE
  FILES:=$(LINUX_DIR)/crypto/crypto_wq.ko
  AUTOLOAD:=$(call AutoLoad,09,crypto_wq)
  $(call AddDepends/crypto)
endef
$(eval $(call KernelPackage,crypto-wq))

define KernelPackage/crypto-rng
  TITLE:=CryptoAPI random number generation
  DEPENDS:=+kmod-crypto-hash +kmod-crypto-hmac +kmod-crypto-sha256
  KCONFIG:= \
	CONFIG_CRYPTO_DRBG \
	CONFIG_CRYPTO_DRBG_HMAC=y \
	CONFIG_CRYPTO_DRBG_HASH=n \
	CONFIG_CRYPTO_DRBG_MENU \
	CONFIG_CRYPTO_JITTERENTROPY \
	CONFIG_CRYPTO_RNG2
  FILES:= \
	$(LINUX_DIR)/crypto/drbg.ko@ge4.2 \
	$(LINUX_DIR)/crypto/jitterentropy_rng.ko@ge4.2 \
	$(LINUX_DIR)/crypto/krng.ko@lt4.2 \
	$(LINUX_DIR)/crypto/rng.ko
  AUTOLOAD:=$(call AutoLoad,09,drbg@ge4.2 jitterentropy_rng@ge4.2 krng@lt4.2 rng)
  $(call AddDepends/crypto)
endef

$(eval $(call KernelPackage,crypto-rng))


define KernelPackage/crypto-iv
  TITLE:=CryptoAPI initialization vectors
  DEPENDS:=+kmod-crypto-manager +kmod-crypto-rng +kmod-crypto-wq
  KCONFIG:= CONFIG_CRYPTO_BLKCIPHER2 CONFIG_CRYPTO_ECHAINIV
  FILES:= \
	$(LINUX_DIR)/crypto/eseqiv.ko \
	$(LINUX_DIR)/crypto/chainiv.ko \
	$(LINUX_DIR)/crypto/echainiv.ko@ge4.3
  AUTOLOAD:=$(call AutoLoad,10,eseqiv chainiv echainiv@ge4.3)
  $(call AddDepends/crypto)
endef

$(eval $(call KernelPackage,crypto-iv))


define KernelPackage/crypto-echainiv
  TITLE:=Encrypted Chain IV Generator
  DEPENDS:=+kmod-crypto-aead
  KCONFIG:=CONFIG_CRYPTO_ECHAINIV
  FILES:=$(LINUX_DIR)/crypto/echainiv.ko
  AUTOLOAD:=$(call AutoLoad,09,echainiv)
  $(call AddDepends/crypto)
endef

$(eval $(call KernelPackage,crypto-echainiv))


define KernelPackage/crypto-seqiv
  TITLE:=CryptoAPI Sequence Number IV Generator
  DEPENDS:=+kmod-crypto-aead +kmod-crypto-rng
  KCONFIG:=CONFIG_CRYPTO_SEQIV
  FILES:=$(LINUX_DIR)/crypto/seqiv.ko
  AUTOLOAD:=$(call AutoLoad,09,seqiv)
  $(call AddDepends/crypto)
endef

$(eval $(call KernelPackage,crypto-seqiv))


define KernelPackage/crypto-hw-caam
  TITLE:=Freescale CAAM driver (SEC4)
  DEPENDS:=@TARGET_imx6||TARGET_mpc85xx +kmod-crypto-aead +kmod-crypto-authenc +kmod-crypto-hash +kmod-random-core
  KCONFIG:= \
	CONFIG_CRYPTO_HW=y \
	CONFIG_CRYPTO_DEV_FSL_CAAM \
	CONFIG_CRYPTO_DEV_FSL_CAAM_JR \
	CONFIG_CRYPTO_DEV_FSL_CAAM_CRYPTO_API \
	CONFIG_CRYPTO_DEV_FSL_CAAM_AHASH_API \
	CONFIG_CRYPTO_DEV_FSL_CAAM_RNG_API \
	CONFIG_CRYPTO_DEV_FSL_CAAM_RINGSIZE=9 \
	CONFIG_CRYPTO_DEV_FSL_CAAM_INTC=n \
	CONFIG_CRYPTO_DEV_FSL_CAAM_DEBUG=n
  FILES:= \
	$(LINUX_DIR)/drivers/crypto/caam/caam.ko \
	$(LINUX_DIR)/drivers/crypto/caam/caamalg.ko \
	$(LINUX_DIR)/drivers/crypto/caam/caamhash.ko \
	$(LINUX_DIR)/drivers/crypto/caam/caam_jr.ko \
	$(LINUX_DIR)/drivers/crypto/caam/caamrng.ko
  AUTOLOAD:=$(call AutoLoad,09,caam caamalg caamhash caam_jr caamrng)
  $(call AddDepends/crypto)
endef

$(eval $(call KernelPackage,crypto-hw-caam))


define KernelPackage/crypto-hw-talitos
  TITLE:=Freescale integrated security engine (SEC) driver
  DEPENDS:=+kmod-crypto-manager +kmod-crypto-hash +kmod-random-core +kmod-crypto-authenc
  KCONFIG:= \
	CONFIG_CRYPTO_HW=y \
	CONFIG_CRYPTO_DEV_TALITOS \
	CONFIG_CRYPTO_DEV_TALITOS1=y \
	CONFIG_CRYPTO_DEV_TALITOS2=y
  FILES:= \
	$(LINUX_DIR)/drivers/crypto/talitos.ko
  AUTOLOAD:=$(call AutoLoad,09,talitos)
  $(call AddDepends/crypto)
endef

$(eval $(call KernelPackage,crypto-hw-talitos))


define KernelPackage/crypto-hw-padlock
  TITLE:=VIA PadLock ACE with AES/SHA hw crypto module
  DEPENDS:=+kmod-crypto-manager
  KCONFIG:= \
	CONFIG_CRYPTO_HW=y \
	CONFIG_CRYPTO_DEV_PADLOCK \
	CONFIG_CRYPTO_DEV_PADLOCK_AES \
	CONFIG_CRYPTO_DEV_PADLOCK_SHA
  FILES:= \
	$(LINUX_DIR)/drivers/crypto/padlock-aes.ko \
	$(LINUX_DIR)/drivers/crypto/padlock-sha.ko
  AUTOLOAD:=$(call AutoLoad,09,padlock-aes padlock-sha)
  $(call AddDepends/crypto)
endef

$(eval $(call KernelPackage,crypto-hw-padlock))


define KernelPackage/crypto-hw-ccp
  TITLE:=AMD Cryptographic Coprocessor
  DEPENDS:=+kmod-crypto-authenc +kmod-crypto-hash +kmod-crypto-manager +kmod-random-core
  KCONFIG:= \
	CONFIG_CRYPTO_HW=y \
	CONFIG_CRYPTO_DEV_CCP=y \
	CONFIG_CRYPTO_DEV_CCP_CRYPTO \
	CONFIG_CRYPTO_DEV_CCP_DD
  FILES:= \
	$(LINUX_DIR)/drivers/crypto/ccp/ccp.ko \
	$(LINUX_DIR)/drivers/crypto/ccp/ccp-crypto.ko
  AUTOLOAD:=$(call AutoLoad,09,ccp ccp-crypto)
  $(call AddDepends/crypto)
endef

$(eval $(call KernelPackage,crypto-hw-ccp))


define KernelPackage/crypto-hw-geode
  TITLE:=AMD Geode hardware crypto module
  DEPENDS:=+kmod-crypto-manager
  KCONFIG:= \
	CONFIG_CRYPTO_HW=y \
	CONFIG_CRYPTO_DEV_GEODE
  FILES:=$(LINUX_DIR)/drivers/crypto/geode-aes.ko
  AUTOLOAD:=$(call AutoLoad,09,geode-aes)
  $(call AddDepends/crypto)
endef

$(eval $(call KernelPackage,crypto-hw-geode))


define KernelPackage/crypto-hw-hifn-795x
  TITLE:=HIFN 795x crypto accelerator
  DEPENDS:=+kmod-random-core +kmod-crypto-manager
  KCONFIG:= \
	CONFIG_CRYPTO_HW=y \
	CONFIG_CRYPTO_DEV_HIFN_795X \
	CONFIG_CRYPTO_DEV_HIFN_795X_RNG=y
  FILES:=$(LINUX_DIR)/drivers/crypto/hifn_795x.ko
  AUTOLOAD:=$(call AutoLoad,09,hifn_795x)
  $(call AddDepends/crypto,+kmod-crypto-des)
endef

$(eval $(call KernelPackage,crypto-hw-hifn-795x))


define KernelPackage/crypto-hw-ppc4xx
  TITLE:=AMCC PPC4xx hardware crypto module
  DEPENDS:=@TARGET_ppc40x||TARGET_ppc44x
  KCONFIG:= \
	CONFIG_CRYPTO_HW=y \
	CONFIG_CRYPTO_DEV_PPC4XX
  FILES:=$(LINUX_DIR)/drivers/crypto/amcc/crypto4xx.ko
  AUTOLOAD:=$(call AutoLoad,90,crypto4xx)
  $(call AddDepends/crypto,+kmod-crypto-manager +kmod-crypto-hash)
endef

define KernelPackage/crypto-hw-ppc4xx/description
  Kernel support for the AMCC PPC4xx HW crypto engine.
endef

$(eval $(call KernelPackage,crypto-hw-ppc4xx))


define KernelPackage/crypto-hw-omap
  TITLE:=TI OMAP hardware crypto modules
  DEPENDS:=@TARGET_omap
  KCONFIG:= \
	CONFIG_CRYPTO_HW=y \
	CONFIG_CRYPTO_DEV_OMAP_AES \
	CONFIG_CRYPTO_DEV_OMAP_DES \
	CONFIG_CRYPTO_DEV_OMAP_SHAM
ifneq ($(wildcard $(LINUX_DIR)/drivers/crypto/omap-des.ko),)
  FILES:= \
	$(LINUX_DIR)/drivers/crypto/omap-aes.ko \
	$(LINUX_DIR)/drivers/crypto/omap-des.ko \
	$(LINUX_DIR)/drivers/crypto/omap-sham.ko
  AUTOLOAD:=$(call AutoLoad,90,omap-aes omap-des omap-sham)
else
  FILES:= \
	$(LINUX_DIR)/drivers/crypto/omap-aes.ko \
	$(LINUX_DIR)/drivers/crypto/omap-sham.ko
  AUTOLOAD:=$(call AutoLoad,90,omap-aes omap-sham)
endif
  $(call AddDepends/crypto,+kmod-crypto-manager +kmod-crypto-hash)
endef

define KernelPackage/crypto-hw-omap/description
  Kernel support for the TI OMAP HW crypto engine.
endef

$(eval $(call KernelPackage,crypto-hw-omap))


define KernelPackage/crypto-authenc
  TITLE:=Combined mode wrapper for IPsec
  DEPENDS:=+kmod-crypto-manager +LINUX_4_4:kmod-crypto-null
  KCONFIG:=CONFIG_CRYPTO_AUTHENC
  FILES:=$(LINUX_DIR)/crypto/authenc.ko
  AUTOLOAD:=$(call AutoLoad,09,authenc)
  $(call AddDepends/crypto)
endef

$(eval $(call KernelPackage,crypto-authenc))

define KernelPackage/crypto-cbc
  TITLE:=Cipher Block Chaining CryptoAPI module
  DEPENDS:=+kmod-crypto-manager
  KCONFIG:=CONFIG_CRYPTO_CBC
  FILES:=$(LINUX_DIR)/crypto/cbc.ko
  AUTOLOAD:=$(call AutoLoad,09,cbc)
  $(call AddDepends/crypto)
endef

$(eval $(call KernelPackage,crypto-cbc))

define KernelPackage/crypto-ctr
  TITLE:=Counter Mode CryptoAPI module
  DEPENDS:=+kmod-crypto-manager +kmod-crypto-seqiv +kmod-crypto-iv
  KCONFIG:=CONFIG_CRYPTO_CTR
  FILES:=$(LINUX_DIR)/crypto/ctr.ko
  AUTOLOAD:=$(call AutoLoad,09,ctr)
  $(call AddDepends/crypto)
endef

$(eval $(call KernelPackage,crypto-ctr))

define KernelPackage/crypto-ccm
 TITLE:=Support for Counter with CBC MAC (CCM)
 DEPENDS:=+kmod-crypto-ctr +kmod-crypto-aead
 KCONFIG:=CONFIG_CRYPTO_CCM
 FILES:=$(LINUX_DIR)/crypto/ccm.ko
 AUTOLOAD:=$(call AutoLoad,09,ccm)
 $(call AddDepends/crypto)
endef

$(eval $(call KernelPackage,crypto-ccm))

define KernelPackage/crypto-pcbc
  TITLE:=Propagating Cipher Block Chaining CryptoAPI module
  DEPENDS:=+kmod-crypto-manager
  KCONFIG:=CONFIG_CRYPTO_PCBC
  FILES:=$(LINUX_DIR)/crypto/pcbc.ko
  AUTOLOAD:=$(call AutoLoad,09,pcbc)
  $(call AddDepends/crypto)
endef

$(eval $(call KernelPackage,crypto-pcbc))

define KernelPackage/crypto-crc32c
  TITLE:=CRC32c CRC module
  DEPENDS:=+kmod-crypto-hash
  KCONFIG:=CONFIG_CRYPTO_CRC32C
  FILES:=$(LINUX_DIR)/crypto/crc32c_generic.ko
  AUTOLOAD:=$(call AutoLoad,04,crc32c_generic,1)
  $(call AddDepends/crypto)
endef

$(eval $(call KernelPackage,crypto-crc32c))


define KernelPackage/crypto-des
  TITLE:=DES/3DES cipher CryptoAPI module
  KCONFIG:=CONFIG_CRYPTO_DES
  FILES:=$(LINUX_DIR)/crypto/des_generic.ko
  AUTOLOAD:=$(call AutoLoad,09,des_generic)
  $(call AddDepends/crypto)
endef

$(eval $(call KernelPackage,crypto-des))


define KernelPackage/crypto-deflate
  TITLE:=Deflate compression CryptoAPI module
  DEPENDS:=+kmod-lib-zlib
  KCONFIG:=CONFIG_CRYPTO_DEFLATE
  FILES:=$(LINUX_DIR)/crypto/deflate.ko
  AUTOLOAD:=$(call AutoLoad,09,deflate)
  $(call AddDepends/crypto)
endef

$(eval $(call KernelPackage,crypto-deflate))


define KernelPackage/crypto-fcrypt
  TITLE:=FCRYPT cipher CryptoAPI module
  KCONFIG:=CONFIG_CRYPTO_FCRYPT
  FILES:=$(LINUX_DIR)/crypto/fcrypt.ko
  AUTOLOAD:=$(call AutoLoad,09,fcrypt)
  $(call AddDepends/crypto)
endef

$(eval $(call KernelPackage,crypto-fcrypt))

define KernelPackage/crypto-ecb
  TITLE:=Electronic CodeBook CryptoAPI module
  DEPENDS:=+kmod-crypto-manager
  KCONFIG:=CONFIG_CRYPTO_ECB
  FILES:=$(LINUX_DIR)/crypto/ecb.ko
  AUTOLOAD:=$(call AutoLoad,09,ecb)
  $(call AddDepends/crypto)
endef

$(eval $(call KernelPackage,crypto-ecb))


define KernelPackage/crypto-hmac
  TITLE:=HMAC digest CryptoAPI module
  DEPENDS:=+kmod-crypto-hash +kmod-crypto-manager
  KCONFIG:=CONFIG_CRYPTO_HMAC
  FILES:=$(LINUX_DIR)/crypto/hmac.ko
  AUTOLOAD:=$(call AutoLoad,09,hmac)
  $(call AddDepends/crypto)
endef

$(eval $(call KernelPackage,crypto-hmac))


define KernelPackage/crypto-cmac
  TITLE:=Support for Cipher-based Message Authentication Code (CMAC)
  DEPENDS:=+kmod-crypto-hash
  KCONFIG:=CONFIG_CRYPTO_CMAC
  FILES:=$(LINUX_DIR)/crypto/cmac.ko
  AUTOLOAD:=$(call AutoLoad,09,cmac)
  $(call AddDepends/crypto)
endef

$(eval $(call KernelPackage,crypto-cmac))


define KernelPackage/crypto-gcm
  TITLE:=GCM/GMAC CryptoAPI module
  DEPENDS:=+kmod-crypto-ctr +kmod-crypto-ghash +kmod-crypto-null
  KCONFIG:=CONFIG_CRYPTO_GCM
  FILES:=$(LINUX_DIR)/crypto/gcm.ko
  AUTOLOAD:=$(call AutoLoad,09,gcm)
  $(call AddDepends/crypto)
endef

$(eval $(call KernelPackage,crypto-gcm))


define KernelPackage/crypto-gf128
  TITLE:=GF(2^128) multiplication functions CryptoAPI module
  KCONFIG:=CONFIG_CRYPTO_GF128MUL
  FILES:=$(LINUX_DIR)/crypto/gf128mul.ko
  AUTOLOAD:=$(call AutoLoad,09,gf128mul)
  $(call AddDepends/crypto)
endef

$(eval $(call KernelPackage,crypto-gf128))


define KernelPackage/crypto-ghash
  TITLE:=GHASH digest CryptoAPI module
  DEPENDS:=+kmod-crypto-gf128 +kmod-crypto-hash
  KCONFIG:=CONFIG_CRYPTO_GHASH
  FILES:=$(LINUX_DIR)/crypto/ghash-generic.ko
  AUTOLOAD:=$(call AutoLoad,09,ghash-generic)
  $(call AddDepends/crypto)
endef

$(eval $(call KernelPackage,crypto-ghash))


define KernelPackage/crypto-md4
  TITLE:=MD4 digest CryptoAPI module
  DEPENDS:=+kmod-crypto-hash
  KCONFIG:=CONFIG_CRYPTO_MD4
  FILES:=$(LINUX_DIR)/crypto/md4.ko
  AUTOLOAD:=$(call AutoLoad,09,md4)
  $(call AddDepends/crypto)
endef

$(eval $(call KernelPackage,crypto-md4))


define KernelPackage/crypto-md5
  TITLE:=MD5 digest CryptoAPI module
  DEPENDS:=+kmod-crypto-hash
  KCONFIG:= \
	CONFIG_CRYPTO_MD5 \
	CONFIG_CRYPTO_MD5_OCTEON
  FILES:=$(LINUX_DIR)/crypto/md5.ko
  AUTOLOAD:=$(call AutoLoad,09,md5)
  $(call AddDepends/crypto)
endef

define KernelPackage/crypto-md5/octeon
  FILES+=$(LINUX_DIR)/arch/mips/cavium-octeon/crypto/octeon-md5.ko
  AUTOLOAD:=$(call AutoLoad,09,octeon-md5)
endef

$(eval $(call KernelPackage,crypto-md5))


define KernelPackage/crypto-michael-mic
  TITLE:=Michael MIC keyed digest CryptoAPI module
  DEPENDS:=+kmod-crypto-hash
  KCONFIG:=CONFIG_CRYPTO_MICHAEL_MIC
  FILES:=$(LINUX_DIR)/crypto/michael_mic.ko
  AUTOLOAD:=$(call AutoLoad,09,michael_mic)
  $(call AddDepends/crypto)
endef

$(eval $(call KernelPackage,crypto-michael-mic))


define KernelPackage/crypto-sha1
  TITLE:=SHA1 digest CryptoAPI module
  DEPENDS:=+kmod-crypto-hash
  KCONFIG:= \
	CONFIG_CRYPTO_SHA1 \
	CONFIG_CRYPTO_SHA1_OCTEON
  FILES:=$(LINUX_DIR)/crypto/sha1_generic.ko
  AUTOLOAD:=$(call AutoLoad,09,sha1_generic)
  $(call AddDepends/crypto)
endef

define KernelPackage/crypto-sha1/octeon
  FILES+=$(LINUX_DIR)/arch/mips/cavium-octeon/crypto/octeon-sha1.ko
  AUTOLOAD:=$(call AutoLoad,09,octeon-sha1)
endef

$(eval $(call KernelPackage,crypto-sha1))


define KernelPackage/crypto-sha256
  TITLE:=SHA224 SHA256 digest CryptoAPI module
  DEPENDS:=+kmod-crypto-hash
  KCONFIG:= \
	CONFIG_CRYPTO_SHA256 \
	CONFIG_CRYPTO_SHA256_OCTEON
  FILES:=$(LINUX_DIR)/crypto/sha256_generic.ko
  AUTOLOAD:=$(call AutoLoad,09,sha256_generic)
  $(call AddDepends/crypto)
endef

define KernelPackage/crypto-sha256/octeon
  FILES+=$(LINUX_DIR)/arch/mips/cavium-octeon/crypto/octeon-sha256.ko
  AUTOLOAD:=$(call AutoLoad,09,octeon-sha256)
endef

$(eval $(call KernelPackage,crypto-sha256))


define KernelPackage/crypto-sha512
  TITLE:=SHA512 digest CryptoAPI module
  DEPENDS:=+kmod-crypto-hash
  KCONFIG:= \
	CONFIG_CRYPTO_SHA512 \
	CONFIG_CRYPTO_SHA512_OCTEON
  FILES:=$(LINUX_DIR)/crypto/sha512_generic.ko
  AUTOLOAD:=$(call AutoLoad,09,sha512_generic)
  $(call AddDepends/crypto)
endef

define KernelPackage/crypto-sha512/octeon
  FILES+=$(LINUX_DIR)/arch/mips/cavium-octeon/crypto/octeon-sha512.ko
  AUTOLOAD:=$(call AutoLoad,09,octeon-sha512)
endef

$(eval $(call KernelPackage,crypto-sha512))


define KernelPackage/crypto-misc
  TITLE:=Other CryptoAPI modules
  DEPENDS:=+kmod-crypto-manager
  KCONFIG:= \
	CONFIG_CRYPTO_ANUBIS \
	CONFIG_CRYPTO_BLOWFISH \
	CONFIG_CRYPTO_CAMELLIA \
	CONFIG_CRYPTO_CAST5 \
	CONFIG_CRYPTO_CAST6 \
	CONFIG_CRYPTO_FCRYPT \
	CONFIG_CRYPTO_KHAZAD \
	CONFIG_CRYPTO_SERPENT \
	CONFIG_CRYPTO_TEA \
	CONFIG_CRYPTO_TGR192 \
	CONFIG_CRYPTO_TWOFISH \
	CONFIG_CRYPTO_TWOFISH_COMMON \
	CONFIG_CRYPTO_TWOFISH_586 \
	CONFIG_CRYPTO_WP512
  FILES:= \
	$(LINUX_DIR)/crypto/anubis.ko \
	$(LINUX_DIR)/crypto/camellia_generic.ko \
	$(LINUX_DIR)/crypto/cast_common.ko \
	$(LINUX_DIR)/crypto/cast5_generic.ko \
	$(LINUX_DIR)/crypto/cast6_generic.ko \
	$(LINUX_DIR)/crypto/khazad.ko \
	$(LINUX_DIR)/crypto/tea.ko \
	$(LINUX_DIR)/crypto/tgr192.ko \
	$(LINUX_DIR)/crypto/twofish_common.ko \
	$(LINUX_DIR)/crypto/wp512.ko \
	$(LINUX_DIR)/crypto/twofish_generic.ko \
	$(LINUX_DIR)/crypto/blowfish_common.ko \
	$(LINUX_DIR)/crypto/blowfish_generic.ko \
	$(LINUX_DIR)/crypto/serpent_generic.ko
  $(call AddDepends/crypto)
endef

ifndef CONFIG_TARGET_x86_64
  define KernelPackage/crypto-misc/x86
    FILES+=$(LINUX_DIR)/arch/x86/crypto/twofish-i586.ko
  endef
endif

$(eval $(call KernelPackage,crypto-misc))


define KernelPackage/crypto-null
  TITLE:=Null CryptoAPI module
  KCONFIG:=CONFIG_CRYPTO_NULL
  FILES:=$(LINUX_DIR)/crypto/crypto_null.ko
  AUTOLOAD:=$(call AutoLoad,09,crypto_null)
  $(call AddDepends/crypto, +kmod-crypto-hash)
endef

$(eval $(call KernelPackage,crypto-null))


define KernelPackage/crypto-test
  TITLE:=Test CryptoAPI module
  KCONFIG:=CONFIG_CRYPTO_TEST
  FILES:=$(LINUX_DIR)/crypto/tcrypt.ko
  $(call AddDepends/crypto,+kmod-crypto-manager)
endef

$(eval $(call KernelPackage,crypto-test))


define KernelPackage/crypto-xts
  TITLE:=XTS cipher CryptoAPI module
  DEPENDS:=+kmod-crypto-gf128 +kmod-crypto-manager
  KCONFIG:=CONFIG_CRYPTO_XTS
  FILES:=$(LINUX_DIR)/crypto/xts.ko
  AUTOLOAD:=$(call AutoLoad,09,xts)
  $(call AddDepends/crypto)
endef

$(eval $(call KernelPackage,crypto-xts))


define KernelPackage/crypto-mv-cesa
  TITLE:=Marvell crypto engine
  DEPENDS:=+kmod-crypto-manager @TARGET_kirkwood||TARGET_orion||TARGET_mvebu
  KCONFIG:=CONFIG_CRYPTO_DEV_MV_CESA
  FILES:=$(LINUX_DIR)/drivers/crypto/mv_cesa.ko
  AUTOLOAD:=$(call AutoLoad,09,mv_cesa)
  $(call AddDepends/crypto)
endef

$(eval $(call KernelPackage,crypto-mv-cesa))
