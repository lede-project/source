# Use the default kernel version if the Makefile doesn't override it

LINUX_RELEASE?=1

LINUX_VERSION-3.18 = .77
LINUX_VERSION-4.4 = .93
LINUX_VERSION-4.9 = .65

LINUX_KERNEL_HASH-3.18.77 = a4e95962f619dd7088441323c2ecb3107f3004244804f1a99e044ac96db10a94
LINUX_KERNEL_HASH-4.4.93 = ed349314f16e78a6571b5f8884f6452782aef6c26b81bcc7ccdac44ecd917c36
LINUX_KERNEL_HASH-4.9.65 = 24ba70877549a3cf25dc5f12efd260d3e957bce64c087de98baf8968ee514895

ifdef KERNEL_PATCHVER
  LINUX_VERSION:=$(KERNEL_PATCHVER)$(strip $(LINUX_VERSION-$(KERNEL_PATCHVER)))
endif

split_version=$(subst ., ,$(1))
merge_version=$(subst $(space),.,$(1))
KERNEL_BASE=$(firstword $(subst -, ,$(LINUX_VERSION)))
KERNEL=$(call merge_version,$(wordlist 1,2,$(call split_version,$(KERNEL_BASE))))
KERNEL_PATCHVER ?= $(KERNEL)

# disable the md5sum check for unknown kernel versions
LINUX_KERNEL_HASH:=$(LINUX_KERNEL_HASH-$(strip $(LINUX_VERSION)))
LINUX_KERNEL_HASH?=x
