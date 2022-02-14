# Base class for Cronologic Conan recipes.
from conans import ConanFile, CMake
from conans.errors import ConanInvalidConfiguration

class CronoConanBase(ConanFile):
    """
    Description
    --------------
    Base class for Cronologic Conan recipes.
    The class provides default behavior of Conan methods for both library and 
    executable outputs, as well as for both Windows and Linux.
    

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
    """

    export_source = False 
    
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
        """
        if self.export_source: 
            self._copy_source(True)

    # __________________________________________________________________________
    #
    def build(self):
        """
        Description
        --------------
        Builds the project using `CMakeLists.txt` on source forlder `/tool`.

        Validates that the package is created for one of the supported operating
        systems in `supported_os`.
        It raises `ConanInvalidConfiguration` in case the os is not supported.
        """
        self._crono_validate_os()

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
            self._copy_source(False)
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
        Deploys all files under `/lib` and `/bin` on the package folder.
        """
        self.copy("lib/*", keep_path=False)  
        self.copy("bin/*", keep_path=False)  

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
        self.config_build_rel_path=  "build"  \
                        + "/" + str(self.settings.os).lower() \
                        + "/" + str(self.settings.arch) \
                        + "/" + str(self.settings.build_type).lower() 
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
        if bool(host_context) == True:
            # Copy from original source code to `/export_source`
            # Current directory is '/tools/conan/'
            proj_src_indir = self.proj_src_indir
        else:
            # Copy from `/export_source` to `/package/PackageID`
            # Current directory is `/export_source`
            proj_src_indir = ""

        self.copy("src/*", src=proj_src_indir)
        self.copy("include/*", src=proj_src_indir)
        self.copy("tools/*", src=proj_src_indir)
        self.copy("CMakeLists.txt", src=proj_src_indir)
        self.copy("README.md", src=proj_src_indir)
        self.copy("LICENSE", src=proj_src_indir)
        self.copy(".clang-format", src=proj_src_indir)
        self.copy(".gitignore", src=proj_src_indir)
        # No copy of .vscode, etc...

    # __________________________________________________________________________
    # 
    def _crono_copy_lib_output(self, lib_name):
        """
        Description
        --------------
        Calls `self.copy()` for all files of `lib_name`, with extensions (lib, 
        dll, a) to `/lib` directory.

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
            self.copy("*.a", src=self.lib_build_rel_path, dst="bin", 
                keep_path=False)
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
                "Crono: Invalid `" + str(supported_os) + "`, it should have "+\
                    "either or both `Windows` and `Linux")

    # __________________________________________________________________________
    #
