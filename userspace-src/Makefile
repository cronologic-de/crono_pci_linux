# -----------------------------------------------------------------------------
# 						Crono Userspace Library 
# -----------------------------------------------------------------------------

include ${shell pwd}/../MakefileCommon.mk

#_____________________
# Set global variables
#
LIBINCPATH	:= ../include
CRONODRVPATH:= ../driver 
INC 		:= -I${shell pwd}/$(LIBINCPATH) -I/usr/include -I${shell pwd}/crono_dma \
			   -I${shell pwd}/$(CRONODRVPATH) -I${shell pwd}/components -I${shell pwd}/

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
REL64DIR 		:= ../build/crono_userspace/release_64
REL64CFLAGS 	:= -g -Wall -m64 -DUSE_CRONO_KERNEL_DRIVER
REL64TARGET		:= crono_userspace
REL64SLNKNAME	:= $(REL64TARGET).so
REL64SONAME		:= $(REL64SLNKNAME)$(LIBVER)
REL64LIBNAME	:= $(REL64SLNKNAME)$(LIBFULLVER)
REL64STNAME		:= $(REL64TARGET).a
REL64LDFLAGS	:= -m64
REL64OBJFILES 	:= $(REL64DIR)/crono_kernel_interface.o $(REL64DIR)/sysfs.o 	
REL64BINPATH	:= ../build/bin/release_64
#
# 64 Bit Release rules
#
release_64: $(REL64DIR)/$(REL64STNAME) $(REL64DIR)/$(REL64TARGET)

$(REL64DIR)/$(REL64TARGET): $(REL64DIR)/$(REL64STNAME) 
	$(GCC) $(INC) $(REL64CFLAGS) -o $(REL64TARGET) -l:$(REL64STNAME) -L$(REL64DIR)
	mv $(REL64TARGET) $(REL64DIR)/$(REL64TARGET)

$(REL64DIR)/$(REL64TARGET): $(REL64DIR)/crono_userspace.o $(REL64DIR)/$(REL64STNAME)
	$(GCC) $(INC) $(REL64CFLAGS) -o $(REL64TARGET) $(REL64DIR)/crono_userspace.o \
		-l:$(REL64STNAME) -L$(REL64DIR)
	mv $(REL64TARGET) $(REL64DIR)/$(REL64TARGET)

$(REL64DIR)/crono_userspace.o: crono_userspace.cpp $(REL64DIR)/sysfs.o $(REL64DIR)/crono_kernel_interface.o crono_userspace.h
	$(call CRONO_MAKE_LIB_CPP_FILE_RULE,crono_userspace,$(REL64DIR),$(REL64CFLAGS),$(REL64LDFLAGS))

$(REL64DIR)/sysfs.o: sysfs.cpp crono_userspace.h
	mkdir -p $(REL64DIR)
	$(call CRONO_MAKE_LIB_CPP_FILE_RULE,sysfs,$(REL64DIR),$(REL64CFLAGS),$(REL64LDFLAGS))

$(REL64DIR)/crono_kernel_interface.o: crono_kernel_interface.cpp $(REL64DIR)/sysfs.o $(LIBINCPATH)/crono_kernel_interface.h	
	$(call CRONO_MAKE_LIB_CPP_FILE_RULE,crono_kernel_interface,$(REL64DIR),$(REL64CFLAGS),$(REL64LDFLAGS))

$(REL64DIR)/$(REL64STNAME): $(REL64OBJFILES) 
	$(call CRONO_MAKE_COMUS_LIBS_RULE,$(REL64DIR),$(REL64SLNKNAME),$(REL64SONAME),$(REL64LIBNAME),$(REL64STNAME),$(REL64OBJFILES),$(REL64LDFLAGS),	\
	,$(REL64BINPATH))

cleanrelease_64: 
	$(call CRONO_MAKE_CLEAN_LIB_OUTPUT_FILES_RULE,	\
		$(REL64DIR),$(REL64TARGET),$(REL64SLNKNAME),$(REL64SONAME),$(REL64LIBNAME),$(REL64STNAME),$(REL64BINPATH))

# _____________________________________________________________________________
# 64 Bit Debug build settings
#
DBG64DIR 		:= ../build/crono_userspace/debug_64
DBG64CFLAGS 	:= -g -Wall -m64 -DUSE_CRONO_KERNEL_DRIVER -DCRONO_DEBUG_ENABLED -DDEBUG
DBG64TARGET		:= crono_userspace
DBG64SLNKNAME	:= $(DBG64TARGET).so
DBG64SONAME		:= $(DBG64SLNKNAME)$(LIBVER)
DBG64LIBNAME	:= $(DBG64SLNKNAME)$(LIBFULLVER)
DBG64STNAME		:= $(DBG64TARGET).a
DBG64LDFLAGS	:= -m64
DBG64OBJFILES	:= $(DBG64DIR)/crono_kernel_interface.o $(DBG64DIR)/sysfs.o 
DBG64BINPATH	:= ../build/bin/debug_64
#
# 64 Bit Debug rules
#
debug_64: $(DBG64DIR)/$(DBG64STNAME) $(DBG64DIR)/$(DBG64TARGET)

$(DBG64DIR)/$(DBG64TARGET): $(DBG64DIR)/crono_userspace.o $(DBG64DIR)/$(DBG64STNAME)
	$(GCC) $(INC) $(DBG64CFLAGS) -o $(DBG64TARGET) $(DBG64DIR)/crono_userspace.o \
		-l:$(DBG64STNAME) -L$(DBG64DIR)
	mv $(DBG64TARGET) $(DBG64DIR)/$(DBG64TARGET)

$(DBG64DIR)/crono_userspace.o: crono_userspace.cpp $(DBG64DIR)/sysfs.o $(DBG64DIR)/crono_kernel_interface.o crono_userspace.h
	$(call CRONO_MAKE_LIB_CPP_FILE_RULE,crono_userspace,$(DBG64DIR),$(DBG64CFLAGS),$(DBG64LDFLAGS))

$(DBG64DIR)/sysfs.o: sysfs.cpp crono_userspace.h
	mkdir -p $(DBG64DIR)
	$(call CRONO_MAKE_LIB_CPP_FILE_RULE,sysfs,$(DBG64DIR),$(DBG64CFLAGS),$(DBG64LDFLAGS))

$(DBG64DIR)/crono_kernel_interface.o: crono_kernel_interface.cpp $(DBG64DIR)/sysfs.o $(LIBINCPATH)/crono_kernel_interface.h
	$(call CRONO_MAKE_LIB_CPP_FILE_RULE,crono_kernel_interface,$(DBG64DIR),$(DBG64CFLAGS),$(DBG64LDFLAGS))

$(DBG64DIR)/$(REL64STNAME): $(DBG64OBJFILES) 
	$(call CRONO_MAKE_COMUS_LIBS_RULE,$(DBG64DIR),$(DBG64SLNKNAME),$(DBG64SONAME),$(DBG64LIBNAME),$(DBG64STNAME),$(DBG64OBJFILES),$(DBG64LDFLAGS),	\
	,$(DBG64BINPATH))
	
cleandebug_64: 
	$(call CRONO_MAKE_CLEAN_LIB_OUTPUT_FILES_RULE,	\
		$(DBG64DIR),$(DBG64TARGET),$(DBG64SLNKNAME),$(DBG64SONAME),$(DBG64LIBNAME),$(DBG64STNAME),$(DBG64BINPATH))

# _____________________________________________________________________________
# General rules to avoid `Looking for an implicit rule for ...` message when using `-d` option
#
sysfs.cpp:
crono_userspace.cpp:
crono_kernel_interface.cpp:
crono_driver_lib.h:
../include/crono_kernel_interface.h:
crono_userspace.h:
Makefile:
release_64:
debug_64:

# _____________________________________________________________________________
# Custom Functions
#

# $1: DIR
# $2: SLNKNAME
# $3: SONAME
# $4: LIBNAME
# $5: STNAME
# $6: OBJFILES
# $7: LDFLAGS
# $8: Empty
# $9: Build BIN path (e.g. `../../build/bin/release`)
# Deacription: Create output directory, `SO`, `ST`, and `SLNK` of common userspace
define CRONO_MAKE_COMUS_LIBS_RULE
	mkdir -p $(1)
	$(GCC) -shared -Wl,-soname,$(1)/$(3) -o $(1)/$(4) $(6) $(7) 
	$(call CRONO_MAKE_CLEAN_FILE, $(1)/$(2))
	ln $(1)/$(4) $(1)/$(2)
	ar rcs $(1)/$(5) $(6)

	# Copy the library to /build/bin corresponding directory
	mkdir -p $(9)
	cp -t $(9) $(1)/$(5)  
endef
