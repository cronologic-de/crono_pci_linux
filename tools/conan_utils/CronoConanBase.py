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

    Use `conan export-pkg` with `-bin` and `-headers`, as they don't build
    nor export source.

    For `-bin`, it assumes the binaries path as following:
    "build/<os>/<arch>/<build_type>/<lib/bin>"

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

        # In `export_source()`, `copy_from_local` is `True` when calling 
        # `_copy_source`.
        if self.export_source: 
            self._copy_source(copy_from_local=True)
        elif self.is_headers:
            self._copy_source(copy_from_local=True,headers_only=True)
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
        Validates that the package is created for one of the supported operating
        """
        self._crono_validate_os()

        if (self.is_headers):
            self.output.info("Crono: package is `-headers`, `include` should " + 
                "be copied directly from project folder without build.")
            return

        cmake = CMake(self)
        # When package is built, don't allow cmake to publish the package, 
        # as it will be locked by conan to avoid mutual depenednecy.
        cmake.definitions["CRONO_PUBLISH_LOCAL_PKG"] = "N"
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
        # In `package()`, `copy_from_local` is `False` when calling 
        # `_copy_source`.
        if self.is_headers:
            self._copy_source(copy_from_local=False, headers_only=True)
        else:
        # Not `-headers` package
            if pack_src:
                self._copy_source(copy_from_local=False)  

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

        self.lib_build_rel_path = self.config_build_rel_path + "/lib"
        self.bin_build_rel_path = self.config_build_rel_path + "/bin"
    # __________________________________________________________________________
    #
    # Function is used by inherited classes
    def _copy_source(self, copy_from_local, headers_only=False):
        """
        Description
        --------------
        Set source directory indirection, based on the caller context local/build,
        and copy files accordingly.

        Parameters
        ----------
        copy_from_local : bool
            Set to 'True` to copy from project local folder. 
            Set to `False` to copy from conan binary folder.
        """
        if copy_from_local:
            # Copy from original source code to `/export_source`
            # Current directory is '/tools/conan/'
            proj_src_indir = self.proj_src_indir
            self.output.info("Crono: copying source from local folder ...")
        else:
            # Copy from `/export_source` to `/package/PackageID`
            # Current directory is `/export_source`
            proj_src_indir = ""
            self.output.info("Crono: copying source from conan binary folder ...")

        # Copy common files to all package types
        self.copy("README.md", src=proj_src_indir)
        self.copy("LICENSE", src=proj_src_indir)
        self.copy("include/*", src=proj_src_indir)
        
        if (headers_only):
        # All needed files are copied, do nothing more
            return 

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
        Call `self.copy()` for all files of `lib_name`, with extensions (lib, 
        dll, a, pdb) to `/lib` directory.
        Copy the library files from `self.lib_build_rel_path`.

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

        # Make sure the file exists to copy
        lib_full_path = self.build_folder + "/" + self.lib_build_rel_path \
                + "/" + lib_file_name
        if not os.path.exists(lib_full_path):
            raise ConanInvalidConfiguration(
                "Crono: file <" + lib_full_path + "> is not found." 
                + " Please make sure project is built on this path."
                + " You may use --build-folder, or copy binaries on that path.")
            return 
        
        # Copy the library
        self.copy(lib_file_name, src=self.lib_build_rel_path, dst="lib", 
            keep_path=False)

        # Copy additional additional from the build folder that might be needed
        # by the library
        if self.settings.os == "Windows":
            # Copy DLL for Windows
            self.copy(lib_name + ".dll", src=self.lib_build_rel_path, dst="lib",
                keep_path=False)

            if self.settings.build_type == "Debug":
                # Copy .pdb for Debug
                self.copy("*.pdb", src=self.lib_build_rel_path, dst="lib", 
                    keep_path=False)

    # __________________________________________________________________________
    #
    def _crono_copy_bin_output(self, exec_name):
        """
        Description
        --------------
        Calls `self.copy()` for all files of `exec_name`, with extensions (exe, 
        Linux-no extension) to `/bin` directory.
        It copies the executable files from `self.bin_build_rel_path`.

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

        exec_full_path = self.build_folder + "/" + self.bin_build_rel_path 
        self.output.info("Crono: copying executable file <" + exec_full_path + 
            "> to package" )

        # Make sure the file exists to copy
        if not os.path.exists(exec_full_path):
            raise ConanInvalidConfiguration(
                "Crono: file <" + exec_full_path + "> is not found." 
                + " Please make sure project is built on this path."
                + " You may use --build-folder, or copy binaries on that path.")
            return 
        self.copy(exec_file_name, src=self.bin_build_rel_path, dst="bin",
            keep_path=False)

        # Copy additional additional from the build folder that might be needed
        # by the executable
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
