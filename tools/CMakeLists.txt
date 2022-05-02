cmake_minimum_required(VERSION 3.13) 
set(TARGET_NAME "crono_pci_linux")
project(${TARGET_NAME})

# _____________________________________________________________________________
# Build Linux(x86_64) `crono_pci_linux.a` static library, for (Debug/Release) 
# configurations.
# Publish conan package to local cache as per `CRONO_PUBLISH_LOCAL_PKG` 
# argument.
# 
# Dependencies: 
# - `cronologic_linux_kernel-headers`
# 
# Prequisites: 
# - conan remote server(s) that have the traversal dependencies are added 
#   and authenticated.
# - Conan recipe is on the same folder.

# Arguments:
# `CRONO_PUBLISH_LOCAL_PKG`: Publish project packages to conan local cache
# - Passed as `-DCRONO_PUBLISH_LOCAL_PKG=N`
# - Values:
#        `N` : Do not publish.
#    Default : Publish
# _____________________________________________________________________________

# Install project tools, then include `crono_common.cmake` and initialize it
IF(NOT EXISTS "${CMAKE_SOURCE_DIR}/crono_common.cmake") 
        message(STATUS "Crono: Installing Cronologic Project Tools ...")
        execute_process(COMMAND "conan" install crono_project_tools/[~1.0.2]@ 
                -if ${CMAKE_SOURCE_DIR}) # CMAKE_SOURCE_DIR is the tools folder
ENDIF()
include(${CMAKE_SOURCE_DIR}/crono_common.cmake)
_crono_cmake_common_init()

# Check supported architecture
IF (NOT CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64")
        message(SEND_ERROR "Crono: unsupported arch <${CMAKE_SYSTEM_PROCESSOR}>")
ENDIF()

# Compilation settings
add_compile_definitions(USE_CRONO_KERNEL_DRIVER)

# Source files settings
set(SOURCE 
        ${PROJ_SRC_INDIR}/src/crono_kernel_interface.cpp
        ${PROJ_SRC_INDIR}/src/sysfs.cpp
)
set(HEADERS
        ${PROJ_SRC_INDIR}/include/crono_kernel_interface.h
        ${PROJ_SRC_INDIR}/include/crono_userspace.h
        ${PROJ_SRC_INDIR}/src/crono_kernel_private.h
)

# The target library
add_library(${TARGET_NAME} STATIC "${SOURCE}" "${HEADERS}")

# Remove the `lib` prefix from the library name
SET_TARGET_PROPERTIES(${TARGET_NAME} PROPERTIES PREFIX "")

# Add dependency to target
_crono_add_dep_header("${TARGET_NAME}" "cronologic_linux_kernel-headers")

# Publish packages on conan local cache _______________________________________ 
IF (NOT CRONO_PUBLISH_LOCAL_PKG STREQUAL "N")
        _crono_target_post_build_export_pkg("${CMAKE_SOURCE_DIR}")
ENDIF()
