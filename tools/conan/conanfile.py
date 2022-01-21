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
        if False == True:
        # Use meson to build the library
            meson = Meson(self)
            meson.configure(source_folder=self.source_folder + "/tools/meson", build_folder="build")
            meson.build()
            meson.install()    # Create the .a file on the output path

        cmake = CMake(self)
        cmake.configure(source_folder=self.source_folder + "/tools/cmake")
        cmake.build()

    # __________________________________________________________________________
    #
    def package(self):
        self._copy_source(False)
        self.copy("*", src="lib", keep_path=False)

    # ==========================================================================
    # Cronologic Custom Methods
    #
    def _copy_source(self, host_context):
        # Set source directory indirection, based on the caller context
        # host/package
        if bool(host_context) == True:
            # Copy from original source code to `/export_source`
            # Current directory is '/tools/conan'
            proj_src_indir = "../../"
        else:
            # Copy from `/export_source` to `/package/PackageID`
            # Current directory is `/export_source`
            proj_src_indir = ""

        self.copy("src/*", src=proj_src_indir)
        self.copy("include/*", src=proj_src_indir)
        self.copy("tools/*", src=proj_src_indir)
        self.copy("CMakeLists.txt", src=proj_src_indir)
        self.copy("Makefile", src=proj_src_indir)
        self.copy("MakefileCommon.mk", src=proj_src_indir)
        self.copy("README.md", src=proj_src_indir)
        self.copy("LICENSE", src=proj_src_indir)
        self.copy(".clang-format", src=proj_src_indir)
        self.copy(".gitignore", src=proj_src_indir)
        # No copy of Build, .vscode, etc...

    # __________________________________________________________________________
