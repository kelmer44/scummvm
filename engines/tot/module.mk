MODULE := engines/tot

MODULE_OBJS = \
	tot.o \
	console.o \
	metaengine.o

# This module can be built as a plugin
ifeq ($(ENABLE_TOT), DYNAMIC_PLUGIN)
PLUGIN := 1
endif

# Include common rules
include $(srcdir)/rules.mk

# Detection objects
DETECT_OBJS += $(MODULE)/detection.o
