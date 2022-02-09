echo _______________________________________________________________________________
echo Overview:
echo .
echo This file is used on development environment to save time writing down all 
echo commands to build Windows project using MSVC MSBuild.
echo All steps and values should be aligned with the build instructions 
echo mentioned in the readme file 
echo https://github.com/cronologic-de/crono_pci_linux/blob/main/README.md.
echo .
echo Output is found on ../build/bfD for Debug, and ../build/bfR for Release. 
echo The batch cleans up the folders and rebuilds the code with every run.
echo .
echo Please review values under Custom Values section before you start.
echo _______________________________________________________________________________

DEBUG_BUILD_DIR="../build/bfD"
RELEASE_BUILD_DIR="../build/bfR"

read -p "Upload to conan cache ([Y]/N)? " CONAN_UPLOAD
echo ---> $CONAN_UPLOAD

echo _______________________________________________________________________________
echo crono: Building x64 Project Buildsystem ...
echo -------------------------------------------------------------------------------
# Clean x64 debug build directory up if already there
if [ -d $DEBUG_BUILD_DIR ]; then
    rm -rf $DEBUG_BUILD_DIR/CMakeFiles
    rm -f $DEBUG_BUILD_DIR/*.*
fi
cmake -B $DEBUG_BUILD_DIR -DCMAKE_BUILD_TYPE=Debug

# Clean x64 releaswe build directory up if already there
if [ -d $RELEASE_BUILD_DIR ]; then
    rm -rf $RELEASE_BUILD_DIR/CMakeFiles
    rm -f $RELEASE_BUILD_DIR/*.*
fi
cmake -B $RELEASE_BUILD_DIR -DCMAKE_BUILD_TYPE=Release

echo _______________________________________________________________________________
echo crono: Building x64 Project ...
echo -------------------------------------------------------------------------------
cmake --build $DEBUG_BUILD_DIR
cmake --build $RELEASE_BUILD_DIR

if [ "$CONAN_UPLOAD" != "N" ]; then
    echo _______________________________________________________________________________
    echo crono: Upload to conan cache ...
    echo -------------------------------------------------------------------------------
    conan create . -s build_type=Debug
    conan create . -s build_type=Release
fi
