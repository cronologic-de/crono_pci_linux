# _____________________________________________________________________________
# Compiler flags
#
GCC 		:= g++

# _____________________________________________________________________________
# Custom Functions
#

# $1: File Full Path, wildcard are accepted
define CRONO_MAKE_CLEAN_FILE
	$(if $(filter-out "","$(wildcard $1)"),-rm $1,)
endef

# $1: File Name
# $2: DIR
# $3: CFLAGS
# $4: LDFLAGS
# Compiles the cpp file, and move .o to DIR
define CRONO_MAKE_CPP_FILE_RULE 
	mkdir -p $(2)
	$(GCC) $(INC) $(3) -c $(1).cpp $(4)
	mv $(1).o $(2)/$(1).o 
endef

# $1: File Name
# $2: DIR
# $3: CFLAGS
# $4: LDFLAGS
# Compiles the c file, and move .o to DIR
define CRONO_MAKE_C_FILE_RULE 
	mkdir -p $(2)
	$(GCC) $(INC) $(3) -c $(1).c $(4)
	mv $(1).o $(2)/$(1).o 
endef

# $1: Target 
# $2: Output Directory (e.g. `../../../build/xtdc4_ug_example/release_64`)
# $3: Build BIN path (e.g. `../../../build/bin/release_64`)
# Copy the target file to BIN directory, and move it to the output directory
define CRONO_COPY_TARGET
	mkdir -p $(3)
	cp -t $(3) $(1)
	mv $(1) $(2)/$(1)
endef

# $1: File Name
# $2: DIR
# $3: CFLAGS
# $4: LDFLAGS
# Compiles the cpp file, and move .o to DIR
define CRONO_MAKE_LIB_CPP_FILE_RULE 
	mkdir -p $(2)
	$(GCC) $(INC) $(3) -c $(1).cpp $(4) -fpic
	mv $(1).o $(2)/$(1).o 
endef

# $1: DIR
# $2: TARGET
# $3: Build BIN path (e.g. `../../../build/bin/release_64`)
define CRONO_MAKE_CLEAN_OUTPUT_FILES_RULE
	$(call CRONO_MAKE_CLEAN_FILE,$(1)/$(2))
	$(call CRONO_MAKE_CLEAN_FILE,$(1)/*.o)
	$(call CRONO_MAKE_CLEAN_FILE,$(1)/*.a)
	$(call CRONO_MAKE_CLEAN_FILE,$(3)/$(2))
endef

# $1: DIR
# $2: TARGET
# $3: SLNKNAME
# $4: SONAME
# $5: LIBNAME
# $6: STNAME
# $7: Build BIN path (e.g. `../../../build/bin/release_64`)
define CRONO_MAKE_CLEAN_LIB_OUTPUT_FILES_RULE
	$(call CRONO_MAKE_CLEAN_FILE,$(1)/$(2))
	$(call CRONO_MAKE_CLEAN_FILE,$(1)/$(3))
	$(call CRONO_MAKE_CLEAN_FILE,$(1)/$(4))
	$(call CRONO_MAKE_CLEAN_FILE,$(1)/$(5))
	$(call CRONO_MAKE_CLEAN_FILE,$(1)/$(6))
	$(call CRONO_MAKE_CLEAN_FILE,$(1)/*.o)
	$(call CRONO_MAKE_CLEAN_FILE,$(1)/lib/*.o)
	$(call CRONO_MAKE_CLEAN_FILE,$(7)/$(6))
endef

# $1: DIR
# $2: SLNKNAME
# $3: SONAME
# $4: LIBNAME
# $5: STNAME
# $6: OBJFILES
# $7: LDFLAGS
# $8: `crono_userspace(_64).a` Name
# $9: Build BIN path (e.g. `../../../build/bin/release_64`)
# Deacription: Create output directory, `SO`, `ST`, and `SLNK`
define CRONO_MAKE_US_LIBS_RULE
	@echo Using userspace library: $(1)/$(8)
	# Build userspace needed library if not found
	[ -f $(1)/$(8) ] || $(MAKE) -C ../userspace

	# Create the `/build/xyz_userspace` corresponding directory
	mkdir -p $(1)

	# Build the dynamic library in case needed 
	$(GCC) -shared -Wl,-soname,$(1)/$(3) -o $(1)/$(4) $(6) $(7) 

	# Cleanup the output folder from old build output files if found (symbolic link)
	$(call CRONO_MAKE_CLEAN_FILE, $(1)/$(2))

	# Create symbolic link file
	ln $(1)/$(4) $(1)/$(2)

	# Create a temporary folder to have all object files of both the `userspace` project, and this project
	mkdir -p $(1)/lib

	# Make sure `lib` /has no files from a previous build
	$(call CRONO_MAKE_CLEAN_FILE,$(1)/lib/*.o)

	# Extract corresponding `crono_userspace(_64).a` to `/lib` 
	# `ar --output` is not compatibile with older linux versions 
	ar x $(9)/$(8)
	mv *.o $(1)/lib 

	# Copy all project `.o` files from corresponding build folder to `/lib`
	cp -t $(1)/lib $(6) 

	# Build the `xyz-sepcific` static library from both files
	ar rcs $(1)/$(5) $(1)/lib/*.o

	# Display static library files for double checking
	# ar t $(1)/$(5)

	# Copy the library to /build/bin corresponding directory
	mkdir -p $(9)
	cp -t $(9) $(1)/$(5)
endef

# $1 Userspace Library Path
# $2 Userspace Library Source Directory Path
# $3 Build type (e.g. release_64)
define CRONO_MAKE_USRSPC_LIB
	@echo Using userspace library: $(1)
	# Build userspace needed library if not found
	[ -f $(1) ] || $(MAKE) -C $(2) $(3)
endef
