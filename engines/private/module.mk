MODULE := engines/private
 
MODULE_OBJS := \
	metaengine.o \
    code.o \
	cursors.o \
	symbol.o \
	funcs.o \
    grammar.o \
	lexer.o \
	private.o
 
MODULE_DIRS += \
	engines/private
 
# This module can be built as a plugin
ifeq ($(ENABLE_PRIVATE), DYNAMIC_PLUGIN)
PLUGIN := 1
endif
 
# Include common rules 
include $(srcdir)/rules.mk

# Detection objects
DETECT_OBJS += $(MODULE)/detection.o
