MODULE := engines/tesoro

MODULE_OBJS = \
	tesoro.o \
	animation.o \
	console.o \
	metaengine.o \
	resources.o \
	scene.o

# This module can be built as a plugin
ifeq ($(ENABLE_TESORO), DYNAMIC_PLUGIN)
PLUGIN := 1
endif

# Include common rules
include $(srcdir)/rules.mk

# Detection objects
DETECT_OBJS += $(MODULE)/detection.o
