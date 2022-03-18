cmake_minimum_required(VERSION 2.9) 

set(TARGET_NAME "crono_pci_linux")
project(${TARGET_NAME})

# _____________________________________________________________________________
# Build Linux(x86_64) `crono_pci_linux.a` static library, for (Debug/Release) 
# configurations.
# Publish conan package to local cache as per `CRONO_PUBLISH_LOCAL_PKG` 
# argument.
# 
# Arguments:
# `CRONO_PUBLISH_LOCAL_PKG`: Publish project packages to conan local cache
# - Passed as `-DCRONO_PUBLISH_LOCAL_PKG=N`
# - Values:
#        `N` : Do not publish.
#    Default : Publish
#
# Prequisites: 
# - conan remote server(s) that have the traversal dependencies are added 
#   and authenticated.
# - Conan recipe is on the same folder.
# _____________________________________________________________________________

# _____________________________________________________________________________
# General Validations and Configurations
#
# Check supported architecture
# CMAKE_GENERATOR_PLATFORM is blank for Ubuntu x86_64 WSL, use instead 
# CMAKE_SYSTEM_PROCESSOR
IF (NOT CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64")
        message(SEND_ERROR "Crono: unsupported arch <${CMAKE_SYSTEM_PROCESSOR}>")
ENDIF()

# Compilation settings
add_compile_definitions(USE_CRONO_KERNEL_DRIVER)
add_compile_options(-Wall)

# Remove .cpp from .o file name
set(CMAKE_CXX_OUTPUT_EXTENSION_REPLACE ON) 

# Set flags as per the built type
if (CMAKE_BUILD_TYPE STREQUAL "Debug")
        add_compile_definitions(CRONO_DEBUG_ENABLED)
        add_compile_options(-g)
elseif(NOT CMAKE_BUILD_TYPE)
        message(STATUS "Crono: build type is set to default ${CMAKE_BUILD_TYPE}"
                " as none was specified.")
        set(CMAKE_BUILD_TYPE "Release")
endif ()

# cd indirection from /tools to the project source code, "." if no shift
set(PROJ_SRC_INDIR ..)  

# All paths are in lower case
string(TOLOWER ${CMAKE_BUILD_TYPE} CRONO_BUILD_TYPE)

# .a output directory
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY 
        ${CMAKE_BINARY_DIR}/build/linux/${CMAKE_SYSTEM_PROCESSOR}/${CRONO_BUILD_TYPE}/lib) 
message(STATUS "Crono: library output directory is <${CMAKE_ARCHIVE_OUTPUT_DIRECTORY}>")
        
# Make sure the HEADERS are the same files included internally, use it in both
# include_directories & set(HEADERS)
set(CRONO_INC_PATH ${PROJ_SRC_INDIR}/include)    

# Include directories paths
include_directories(${TARGET_NAME} PRIVATE ${CRONO_INC_PATH})

# Source files settings
set(SOURCE 
        ${PROJ_SRC_INDIR}/src/crono_kernel_interface.cpp
        ${PROJ_SRC_INDIR}/src/sysfs.cpp
)
set(HEADERS
        ${CRONO_INC_PATH}/crono_kernel_interface.h
        ${CRONO_INC_PATH}/crono_userspace.h
        ${PROJ_SRC_INDIR}/src/crono_linux_kernel.h
        ${PROJ_SRC_INDIR}/src/crono_kernel_private.h
)

# The target library
add_library(${TARGET_NAME} STATIC "${SOURCE}" "${HEADERS}")

# Remove the `lib` prefix from the library name
SET_TARGET_PROPERTIES(${TARGET_NAME} PROPERTIES PREFIX "")

# Publish packages on conan local cache _______________________________ 
IF (NOT CRONO_PUBLISH_LOCAL_PKG STREQUAL "N")
        add_custom_command(TARGET ${TARGET_NAME} POST_BUILD 
                COMMAND "echo" Crono: exporting Main Package to conan 
                              local cache, from current/build directory
                COMMAND "conan" export-pkg ${CMAKE_SOURCE_DIR} --force
                                # Source folder is related to build 
                                # directory
                                -sf ../..
                                -s build_type=${CMAKE_BUILD_TYPE}
                                -s arch=x86_64)
ENDIF()
