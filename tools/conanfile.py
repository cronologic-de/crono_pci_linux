from conans import ConanFile, CMake, tools
from conans.errors import ConanInvalidConfiguration


class CronoPciLinuxConan(ConanFile):
    # __________________________________________________________________________
    # Values to be reviewed with every new version
    #
    version = "0.0.2"
    cronologic_linux_kernel_ref = "cronologic_linux_kernel-headers/0.0.2"

    # __________________________________________________________________________
    # Member variables
    #
    name = "crono_pci_linux"
    license = "GPL-3.0 License"
    author = "Bassem Ramzy <SanPatBRS@gmail.com>"
    url = "https://crono.jfrog.io/artifactory/prod/_/_/crono_pci_linux/" + version
    description = "Linux user mode driver to support large DMA ring buffers on a PCI bus"
    topics = ("cronologic", "pci", "abstraction", "linux")
    settings = "os", "compiler", "build_type", "arch"

    # ==========================================================================
    # Conan Methods
    #
    def validate(self):
        # Validate os, Linux only is supported
        if self.settings.os != "Linux":
            raise ConanInvalidConfiguration(
                "crono_pci_linux is only supported for Linux, <" +
                str(self.settings.os) + "> is not supported.")

        # Validate build_type, Release and Debug only are supported
        if self.settings.build_type != "Release"    \
                and self.settings.build_type != "Debug":
            self.output.error("Build type <" + str(self.settings.build_type) +
                              "> is not supported")
        
        self._crono_init()

    # __________________________________________________________________________
    #
    def requirements(self):
        self.requires(self.cronologic_linux_kernel_ref)

    # __________________________________________________________________________
    #
    def export_sources(self):
        self._copy_source(True)

    # __________________________________________________________________________
    #
    def build(self):
        cmake = CMake(self)
        cmake.configure(source_folder=self.source_folder + "/tools")
        cmake.build()

    # __________________________________________________________________________
    #
    def package(self):
        self._copy_source(False)
        # Library is not copied to package, as it's always built from source. 

    # __________________________________________________________________________
    #
    # def deploy(self):
    # Can't be used to deploy the library to the current folder from which the 
    # command (e.g. conan install) runs, as the library is not in the package 
    # in the first place, while self.copy() copies from the package.
    
    # ==========================================================================
    # Cronologic Custom Methods
    #
    def _copy_source(self, host_context):
        # Set source directory indirection, based on the caller context
        # host/package
        if bool(host_context) == True:
            # Copy from original source code to `/export_source`
            # Current directory is '/tools'
            proj_src_indir = ".."
        else:
            # Copy from `/export_source` to `/package/PackageID`
            # Current directory is `/export_source`
            proj_src_indir = ""

        self.copy("src/*", src=proj_src_indir)
        self.copy("include/*", src=proj_src_indir)
        self.copy("tools/*", src=proj_src_indir)
        self.copy("Makefile", src=proj_src_indir)
        self.copy("MakefileCommon.mk", src=proj_src_indir)
        self.copy("README.md", src=proj_src_indir)
        self.copy("LICENSE", src=proj_src_indir)
        self.copy(".clang-format", src=proj_src_indir)
        self.copy(".gitignore", src=proj_src_indir)
        # No copy of Build, .vscode, etc...

    # __________________________________________________________________________
    #
    def _crono_init(self):
        self.lib_name = self.name + ".a"
        # All paths are in lower case
        self.lib_build_rel_path = "build/lib" \
                        + "/" + str(self.settings.os).lower() \
                        + "/" + str(self.settings.arch) \
                        + "/" + str(self.settings.build_type).lower() 

    # __________________________________________________________________________
