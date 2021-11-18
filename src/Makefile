# -----------------------------------------------------------------------------
# 						Crono Userspace Library 
# -----------------------------------------------------------------------------

include ${shell pwd}/../MakefileCommon.mk

#_____________________
# Set global variables
#
LIBINCPATH  := ../include
CRONODRVPATH:= ../driver 
INC         := -I${shell pwd}/$(LIBINCPATH) 

# Module Full Version: <Version number>.<minor number>.<release number>
LIBVER		:= .1
LIBFULLVER	:= $(LIBVER).0.1

#
# Compiler flags
#
GCC 		:= g++

# _____________________________________________________________________________
# Default build
#
all: release_64 debug_64
clean: cleanrelease_64 cleandebug_64

# _____________________________________________________________________________
# 64 Bit Release build settings
#
REL64DIR        := ../../build/linux/crono_pci_linux/release_64
REL64CFLAGS     := -g -Wall -m64 -DUSE_CRONO_KERNEL_DRIVER
REL64TARGET     := crono_pci_linux
REL64STNAME     := $(REL64TARGET).a
REL64LDFLAGS    := -m64
REL64OBJFILES   := $(REL64DIR)/crono_kernel_interface.o $(REL64DIR)/sysfs.o 	
REL64BINPATH    := ../../build/linux/bin/release_64
#
# 64 Bit Release rules
#
release_64: $(REL64DIR)/$(REL64STNAME) 

$(REL64DIR)/sysfs.o: sysfs.cpp \
		$(LIBINCPATH)/crono_userspace.h crono_linux_kernel.h
	mkdir -p $(REL64DIR)
	$(call CRONO_MAKE_LIB_CPP_FILE_RULE,sysfs,$(REL64DIR),$(REL64CFLAGS),$(REL64LDFLAGS))

$(REL64DIR)/crono_kernel_interface.o: crono_kernel_interface.cpp \
		$(REL64DIR)/sysfs.o $(LIBINCPATH)/crono_kernel_interface.h crono_linux_kernel.h
	$(call CRONO_MAKE_LIB_CPP_FILE_RULE,crono_kernel_interface,$(REL64DIR),$(REL64CFLAGS),$(REL64LDFLAGS))

$(REL64DIR)/$(REL64STNAME): $(REL64OBJFILES) 
	$(call CRONO_MAKE_STATIC_LIB_RULE,$(REL64DIR),$(REL64STNAME),$(REL64BINPATH))

cleanrelease_64: 
	$(call CRONO_MAKE_CLEAN_LIB_OUTPUT_FILES_RULE,	\
		$(REL64DIR),$(REL64TARGET),"","",$(REL64LIBNAME),$(REL64STNAME),$(REL64BINPATH))

# _____________________________________________________________________________
# 64 Bit Debug build settings
#
DBG64DIR        := ../../build/linux/crono_pci_linux/debug_64
DBG64CFLAGS     := -g -Wall -m64 -DUSE_CRONO_KERNEL_DRIVER -DCRONO_DEBUG_ENABLED -DDEBUG
DBG64TARGET     := crono_pci_linux
DBG64STNAME     := $(DBG64TARGET).a
DBG64LDFLAGS    := -m64
DBG64OBJFILES   := $(DBG64DIR)/crono_kernel_interface.o $(DBG64DIR)/sysfs.o 
DBG64BINPATH    := ../../build/linux/bin/debug_64
#
# 64 Bit Debug rules
#
debug_64: $(DBG64DIR)/$(DBG64STNAME) 

$(DBG64DIR)/sysfs.o: sysfs.cpp \
		$(LIBINCPATH)/crono_userspace.h crono_linux_kernel.h
	mkdir -p $(DBG64DIR)
	$(call CRONO_MAKE_LIB_CPP_FILE_RULE,sysfs,$(DBG64DIR),$(DBG64CFLAGS),$(DBG64LDFLAGS))

$(DBG64DIR)/crono_kernel_interface.o: crono_kernel_interface.cpp \
		$(DBG64DIR)/sysfs.o $(LIBINCPATH)/crono_kernel_interface.h crono_linux_kernel.h
	$(call CRONO_MAKE_LIB_CPP_FILE_RULE,crono_kernel_interface,$(DBG64DIR),$(DBG64CFLAGS),$(DBG64LDFLAGS))

$(DBG64DIR)/$(REL64STNAME): $(DBG64OBJFILES) 
	$(call CRONO_MAKE_STATIC_LIB_RULE,$(DBG64DIR),$(DBG64STNAME),$(DBG64BINPATH))
	
cleandebug_64: 
	$(call CRONO_MAKE_CLEAN_LIB_OUTPUT_FILES_RULE,	\
		$(DBG64DIR),$(DBG64TARGET),$(DBG64SLNKNAME),"","",$(DBG64STNAME),$(DBG64BINPATH))

# _____________________________________________________________________________
# General rules to avoid `Looking for an implicit rule for ...` message when using `-d` option
#
sysfs.cpp:
crono_userspace.cpp:
crono_kernel_interface.cpp:
../include/crono_kernel_interface.h:
Makefile:
release_64:
debug_64:
