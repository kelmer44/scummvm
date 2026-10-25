MODULE := engines/tesoro

MODULE_OBJS = \
	tesoro.o \
	animation.o \
	console.o \
	jim.o \
	metaengine.o \
	overlap.o \
	renderer.o \
	resources.o \
	scene.o \
	script.o \
	scriptexec.o

# This module can be built as a plugin
ifeq ($(ENABLE_TESORO), DYNAMIC_PLUGIN)
PLUGIN := 1
endif

# Include common rules
include $(srcdir)/rules.mk

# Detection objects
DETECT_OBJS += $(MODULE)/detection.o
