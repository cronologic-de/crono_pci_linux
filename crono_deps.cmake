function(CRONO_BUILD_DEP DEP_PKG_REF CONAN_SRC_FLDR CRONO_BLD_FLDR)
	SET(CRONO_LOCAL_PKG_DIR ${CRONO_BLD_FLDR}/${DEP_PKG_REF})
	message(STATUS  "Crono: building dependency <${DEP_PKG_REF}>"
				" on <${CRONO_LOCAL_PKG_DIR}>"
				" from source <${CONAN_SRC_FLDR}>"
				" for <${CMAKE_BUILD_TYPE}> configuration.")

	SET(DEPS_PKG_FLDR_INFO_FILE_NAME "deps_pkg_folder_info.txt")

	# In case traversal requirements are not installed, install them.
	# `conanfile.txt`/`conanfile.py` should be available in the current directory
	execute_process(
		COMMAND conan install . --install-folder ${CRONO_LOCAL_PKG_DIR}/pkgsif
				-s build_type=${CMAKE_BUILD_TYPE}
		WORKING_DIRECTORY ${CONAN_SRC_FLDR}) 
	execute_process(
		COMMAND conan info . --install-folder ${CRONO_LOCAL_PKG_DIR}/pkgsif 
				--paths -n package_folder 
				OUTPUT_FILE ${CRONO_LOCAL_PKG_DIR}/${DEPS_PKG_FLDR_INFO_FILE_NAME} 
		WORKING_DIRECTORY ${CONAN_SRC_FLDR}) 

	# Read the output file of `crono info` to get the package path on cache
	file(READ ${CRONO_LOCAL_PKG_DIR}/${DEPS_PKG_FLDR_INFO_FILE_NAME} DEPS_PGK_FLDR_INFO)

	# Convert information to list, every line is a list item, so we can loop on 
	# them.
	STRING(REGEX REPLACE "\n" ";;" DEPS_PGK_FLDR_INFO "${DEPS_PGK_FLDR_INFO}")

	foreach(info_line ${DEPS_PGK_FLDR_INFO})
		string(FIND ${info_line} "package_folder: "  pkg_fldr_pos) 
		
		IF (${pkg_fldr_pos} GREATER 0)
		# info_line is of `package_folder`
			string(FIND ${info_line} ${DEP_PKG_REF} pkg_ref_pos) 

			# Validate it's the line of our dependency
			IF (${pkg_ref_pos} LESS_EQUAL 0)
				message(STATUS "Crono: skipping package folder <${info_line}>")
				continue()
			ELSE()
				message(STATUS "Crono: found required package info line <${info_line}>")
			ENDIF()

			# Get Package Name
			string(REPLACE "    package_folder: " "" pkg_path ${info_line})
			string(FIND ${info_line} "package/" pkg_pos) 
			MATH(EXPR pkg_pos "${pkg_pos} + 8")	# 8 of `package/` in the path
			# Extract 40-bytes length conan Package-ID
			string(SUBSTRING ${info_line} ${pkg_pos} 40 pkg_name) 
			
			# Copy package to local folder to build it, and get the library
			message(STATUS  "Crono: copying folder ${pkg_path} to local folder"
							" ${CRONO_LOCAL_PKG_DIR}/")
			execute_process(COMMAND cp -R ${pkg_path} ${CRONO_LOCAL_PKG_DIR})

			# Build the package using `cmake`
			set(PKG_CMAKE_DIR ${CRONO_LOCAL_PKG_DIR}/${pkg_name}/tools)
			set(PKG_BUILD_DIR ${CRONO_LOCAL_PKG_DIR}/${pkg_name})
			IF(EXISTS ${PKG_CMAKE_DIR})
				# CMake is found for this package, build it, without publishing
				# the package to local cache, as it's already there.
				message(STATUS "Crono: building cmake in <${PKG_CMAKE_DIR}>")
				execute_process(
					COMMAND cmake 	-B ${PKG_BUILD_DIR} 
									-DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
									-DCRONO_PUBLISH_LOCAL_PKG=N
					WORKING_DIRECTORY ${PKG_CMAKE_DIR})
				execute_process(
					COMMAND cmake --build ${PKG_BUILD_DIR}
					WORKING_DIRECTORY ${PKG_CMAKE_DIR})

				# The build directory is `PKG_BUILD_DIR/build`, copy it to 
				# Caller build folder CRONO_BLD_FLDR
				message(STATUS "Crono: copying build directory from"
					" <${PKG_BUILD_DIR}/build>"
					" to caller build directory <${CRONO_BLD_FLDR}>")
				execute_process(COMMAND cp -R ${PKG_BUILD_DIR}/build ${CRONO_BLD_FLDR})
				break()
			ELSE()
				message(STATUS "Crono: no cmake build found on <${PKG_CMAKE_DIR}>")
			ENDIF()
		ENDIF()
	endforeach()
endfunction()
