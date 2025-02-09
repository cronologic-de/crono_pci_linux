# Delete build folder and all dep_pkg, and crono_common.cmake
[ -d "../build" ] && rm -d -r ../build --verbose 
[ -f "./CMakeUserPresets.json" ] && rm ./CMakeUserPresets.json --verbose
[ -f "./crono_common.cmake" ] && rm ./crono_common.cmake --verbose

# Install crono_project_tool needed by CMake
CRONO_PROJ_TOOLS_PKG=$(grep -Po "install \Kcrono_project_tools/\[~\d+.\d+.\d+\]" CMakeLists.txt)
conan install $CRONO_PROJ_TOOLS_PKG@ -if . --update
