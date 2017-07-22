#
# Makefile for Shortcut FE.
#

ifeq ($(SFE_SUPPORT_IPV6),)
SFE_SUPPORT_IPV6=y
endif
ccflags-$(SFE_SUPPORT_IPV6) += -DSFE_SUPPORT_IPV6

obj-m += shortcut-fe.o shortcut-fe-ipv6.o shortcut-fe-cm.o fast-classifier.o

shortcut-fe-objs := \
	sfe_ipv4.o

shortcut-fe-ipv6-objs := \
	sfe_ipv6.o

shortcut-fe-cm-objs := \
	sfe_cm.o
