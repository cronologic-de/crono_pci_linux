# Base class for Cronologic Conan recipes.
import os
from conans import ConanFile, CMake
from conans.errors import ConanInvalidConfiguration

class CronoConanBase(ConanFile):
    """
    Description
    --------------
    Base class for Cronologic Conan recipes.
    The class provides default behavior of Conan methods for both library and 
    executable outputs, as well as for both Windows and Linux.

    It supports "Main Package", `-bin`, and `-headers` package types.

    For `-bin`, it assumes the following paths:
    - "build/<build_folder>/<os>/<arch>/<build_type>/<lib/bin>"
    - build_folder is:
      - `bfvs` for Windows x86_64
      - `bfvs32` for Windows x86
      - `bfD` for Linux Debug
      - `bfR` for Linux Release
    - + `/lib` for library, or `/bin` for executables.

    Use `conan export-pkg` with `-bin` and `-headers`, as they don't build
    nor export source.

    Attributes
    --------------
    supported_os : list of Strings
        List, of Supported OS, either `Windows` or `Linux` or both.
        Variable is not defined in the class, it MUST be defined in the 
        inherited class.

    proj_src_indir : str
        It has Project Source-code Directory Indirection. 
        It is the cd indirection from the recipe current directory (e.g. /tools) 
        to the project source code (i.e. the folder has /src).
        Variable is not defined in the class, it MUST be defined in the inherited 
        class.

    export_source : bool
        Is set to `True` if source is needed to be exported.
        Default value is `False`.

    exports : 
        Conan member attribute to export files, inherited class should export 
        this file in order to be able to upload the package.
    
    is_bin : bool
        Inherited class sets it to True if the package is `-bin`.
        If set, then, all methods will behave accordingly, e.g. no build, only
        binary output will be packaged.
        Default value is `False`.

    is_headers : bool
        Inherited class sets it to True if the package is `-headers`.
        If set, then, all methods will behave accordingly, e.g. no build, only
        include folder will be packaged.
        Default value is `False`.
    """

    export_source = False
    is_bin = False
    is_headers = False

    # ==========================================================================
    # Conan Methods
    #
    def validate(self):
        """
        Description
        --------------
        Validates that the package is created for one of the supported operating
        systems in `supported_os`.
        It raises `ConanInvalidConfiguration` in case the os is not supported.
        """
        self._crono_validate_os()

    # __________________________________________________________________________
    #
    def export_sources(self):
        """
        Description
        --------------
        ConanFile method, used to copy the source code from local machine to 
        the relevant build/<build-binary> folder of the package. 
        """
        if self.export_source: 
            self._copy_source(True)
        else:
            # Do Nothing 
            return 

    # __________________________________________________________________________
    #
    def build(self):
        """
        Description
        --------------
        Builds the project using `CMakeLists.txt` on source forlder `/tool`.
        It doesn't build in case of `is_bin` or `is_headers` is True.

        Validates that the package is created for one of the supported operating
        systems in `supported_os`.
        It raises `ConanInvalidConfiguration` in case the os is not supported.
        """
        self._crono_validate_os()

        if (self.is_bin):
            self.output.info("Crono: package is `-bin`, binary ouput will " + 
                "be copied directly from project build folder, without build.")
            return
        elif (self.is_headers):
            self.output.info("Crono: package is `-headers`, include will " + 
                "be copied directly from project build folder, without build.")
            return

        cmake = CMake(self)
        cmake.configure(source_folder=self.source_folder + "/tools")
        cmake.build()

    # __________________________________________________________________________
    #
    def package(self, pack_src=False, lib_name="", exec_name=""):
        """
        Parameters
        ----------
        pack_src : bool, optional
            Set to 'True` if you want to dd source code to package.

        lib_name : str, optional
            Name of the library file without extension. 
            When set, library file(s) (.lib, .dll, .a, .pdb) will be added to
            the package `/lib` folder. 

        exec_name : int, optional
            Name of the executable file without extension. 
            When set, executable file (.exe, Linux) will be added to the .
            package `/bin` folder. All files under `/lib` on the package will
            be copied as well, asssumed to be needed by the executable.
        """
        self._crono_init()
        if pack_src:
            self._copy_source(False) # False: Copy from build folder, assuming 
                                     # source is exported
        if self.is_headers:
            self._copy_source(True)  # True: copy from project folder, assuming
                                     # `conan export-pkg`` is called
        if lib_name != "":
            self._crono_copy_lib_output(lib_name)
        if exec_name != "":
            self._crono_copy_bin_output(exec_name)

    # __________________________________________________________________________
    #
    def deploy(self):
        """
        Description
        --------------
        Deploys all files fould under `/lib` and `/bin` on the package folder.
        In case of is_headers, it deploys files found under`/include` as well.
        """
        self.copy("lib/*", keep_path=False)
        self.copy("bin/*", keep_path=False)
        if self.is_headers:
            self.copy("include/*", keep_path=False)

    # __________________________________________________________________________
    #
    def package_id(self):
        """
        Description
        --------------
        Although code is compiler-arch-build_type-independent, hence, not 
        needed in the settings, but `CMake` and other build system integrations, 
        if they are building, they need a compiler defined, or they will fail.
        When adding "compiler" to the settings, Linux conan build fails 
        sometimes compiler version mismatch, the following lines are needed to 
        achieve some binary compatibility.

        `-headers` package has only `os` left to have a value. 
        `Main Package` and `-bin` set all compiler settings to "any"
        """
        self.output.info("Crono: Setting package to be compiler-independent.")
        self.info.settings.compiler = "any"
        self.info.settings.compiler.version = "any"
        self.info.settings.compiler.libcxx = "any"

        if self.is_headers:
            self.output.info("Crono: Setting `-headers` package to be "
                + "arch-build_type-independent.")
            self.info.settings.arch = "any"
            self.info.settings.build_type = "any"

    # ==========================================================================
    # Cronologic Custom Methods
    #
    def _crono_init(self):
        """
        Description
        --------------
        Sets the member variables `config_build_rel_path`, `lib_build_rel_path`, 
        and `bin_build_rel_path`.
        """
        # All paths are in lower case
        self.config_build_rel_path = "build"  \
            + "/" + str(self.settings.os).lower() \
            + "/" + str(self.settings.arch) \
            + "/" + str(self.settings.build_type).lower()
        
        if (self.is_bin):
            if self.settings.os == "Windows":
                if self.settings.arch == "x86_64":
                    self.config_build_rel_path = self.proj_src_indir \
                        + "/build/bfvs/" + self.config_build_rel_path
                else:
                    # `x86`
                    self.config_build_rel_path = self.proj_src_indir \
                        + "/build/bfvs32/" + self.config_build_rel_path
            elif self.settings.os == "Linux":
                if self.settings.build_type == "Debug":
                    self.config_build_rel_path = self.proj_src_indir \
                        + "/build/bfD/" + self.config_build_rel_path
                else:
                    # `Release`
                    self.config_build_rel_path = self.proj_src_indir \
                        + "/build/bfR/" + self.config_build_rel_path

        self.lib_build_rel_path = self.config_build_rel_path + "/lib"
        self.bin_build_rel_path = self.config_build_rel_path + "/bin"
    # __________________________________________________________________________
    #
    # Function is used by inherited classes
    def _copy_source(self, host_context):
        """
        Description
        --------------
        Set source directory indirection, based on the caller context host/package.
        It refers to source folder using `proj_src_indir` in case of `host_context`
        is `False`.

        Parameters
        ----------
        hos_context : bool
            Set to 'True` you copy to `/export_source`, of `False`, if you copy to 
            Package Binary Folder.
        """
        if self.is_bin:
            self.output.info("Crono: No source is copied for `-bin` package.")
            return 

        if host_context:
            # Copy from original source code to `/export_source`
            # Current directory is '/tools/conan/'
            proj_src_indir = self.proj_src_indir
        else:
            # Copy from `/export_source` to `/package/PackageID`
            # Current directory is `/export_source`
            proj_src_indir = ""

        self.copy("README.md", src=proj_src_indir)
        self.copy("LICENSE", src=proj_src_indir)
        self.copy("include/*", src=proj_src_indir)
        
        if (host_context and self.is_headers):
            return 

        self.copy("include/*", src=proj_src_indir)
        self.copy("src/*", src=proj_src_indir)
        self.copy("tools/*", src=proj_src_indir)
        self.copy(".clang-format", src=proj_src_indir)
        self.copy(".gitignoself.re", src=proj_src_indir)

        # No copy of .vscode, etc...

    # __________________________________________________________________________
    #
    def _crono_copy_lib_output(self, lib_name):
        """
        Description
        --------------
        Calls `self.copy()` for all files of `lib_name`, with extensions (lib, 
        dll, a, pdb) to `/lib` directory.
        It copies the library files from the "Library Build Relative Path" that
        is `build/<os>/<arch>/<build_type>/lib` on source of `self.copy`.

        Parameters
        ----------
        lib_name : str
            The library file name without extension
        """
        # Copy library
        if self.settings.os == "Windows":
            lib_file_name = lib_name + ".lib"
        elif self.settings.os == "Linux":
            lib_file_name = lib_name + ".a"

        self.output.info("Crono: copying library file <" + lib_file_name + 
            "> from `Build Folder` relative path <" + 
            self.lib_build_rel_path + ">" )

        self.copy(lib_file_name, src=self.lib_build_rel_path, 
            dst="lib", keep_path=False)

        if self.settings.os == "Windows":
            # Copy DLL for Windows
            self.copy(lib_name + ".dll", src=self.lib_build_rel_path,
                      dst="lib", keep_path=False)

            if self.settings.build_type == "Debug":
                # Copy .pdb for Debug
                self.copy("*.pdb", src=self.lib_build_rel_path,
                          dst="lib", keep_path=False)

    # __________________________________________________________________________
    #
    def _crono_copy_bin_output(self, exec_name):
        """
        Description
        --------------
        Calls `self.copy()` for all files of `exec_name`, with extensions (exe, 
        Linux-no extension) to `/bin` directory.
        It copies the executable file from the "Library Build Relative Path" 
        that is `build/<os>/<arch>/<build_type>/bin` on source of `self.copy`.

        Parameters
        ----------
        exec_name : str
            The executable file name without extension
        """
        # Copy library
        if self.settings.os == "Windows":
            exec_file_name = exec_name + ".exe"
        elif self.settings.os == "Linux":
            exec_file_name = exec_name

        self.output.info("Crono: copying executable file <" + exec_file_name + 
            "> from `Build Folder` relative path <" + 
            self.bin_build_rel_path + ">" )

        self.copy(exec_file_name, src=self.bin_build_rel_path, dst="bin",
                  keep_path=False)

        if self.settings.os == "Windows":
            self.copy("*.dll", src=self.lib_build_rel_path, dst="bin",
                      keep_path=False)

            if self.settings.build_type == "Debug":
                # Copy .pdb for Debug
                self.copy("*.pdb", src=self.lib_build_rel_path, dst="bin",
                          keep_path=False)

        elif self.settings.os == "Linux":
            self.copy("*.so*", src=self.lib_build_rel_path, dst="bin",
                      keep_path=False)

    # __________________________________________________________________________
    #
    def _crono_validate_windows_only(self):
        """
        Description
        --------------
        Validates the os is `Windows`.
        """
        if self.settings.os != "Windows":
            raise ConanInvalidConfiguration(
                "Crono: " + self.name + " is only supported for Windows"
                ", " + self.settings.os + " is not supported")

    # __________________________________________________________________________
    #
    def _crono_validate_linux_only(self):
        """
        Description
        --------------
        Validates the os is `Linux`.
        """
        if self.settings.os != "Linux":
            raise ConanInvalidConfiguration(
                "Crono: " + self.name + " is only supported for Linux"
                ", " + self.settings.os + " is not supported")

    # __________________________________________________________________________
    #
    def _crono_validate_windows_linux_only(self):
        """
        Description
        --------------
        Validates the os is either `Windows` or `Linux`.
        """
        if self.settings.os != "Windows" and self.settings.os != "Linux":
            raise ConanInvalidConfiguration(
                "Crono: " + self.name + " is only supported for Linux and Windows"
                ", " + self.settings.os + " is not supported")

    # __________________________________________________________________________
    #
    def _crono_validate_os(self, supported_os=""):
        """
        Description
        --------------
        Validates the os is one of the `supported_os` elements.

        Parameters
        ----------
        supported_os : str
            List of supported os. Is got from `self.supported_os` if not passed.
        """
        if supported_os == "":
            supported_os = self.supported_os

        if all(x in supported_os for x in ['Windows', 'Linux']):
            self._crono_validate_windows_linux_only()
        elif all(x in supported_os for x in ['Windows']):
            self._crono_validate_windows_only()
        elif all(x in supported_os for x in ['Linux']):
            self._crono_validate_linux_only()
        else:
            raise ConanInvalidConfiguration(
                "Crono: Invalid `" + str(supported_os) + "`, it should have " + 
                "either or both `Windows` and `Linux")

    # __________________________________________________________________________
    #
