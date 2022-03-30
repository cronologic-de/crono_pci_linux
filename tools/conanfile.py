from conans import ConanFile

class CronoPciLinuxConan(ConanFile):
    python_requires = "crono_utils/0.0.1"
    python_requires_extend = "crono_utils.CronoConanBase"

    # __________________________________________________________________________
    # Values to be reviewed with every new version
    #
    version = "0.0.1"
    cronologic_linux_kernel_ref = "cronologic_linux_kernel-headers/[~0.0.1]"

    # __________________________________________________________________________
    # Member variables
    #
    name = "crono_pci_linux"
    license = "GPL-3.0 License"
    author = "Bassem Ramzy <SanPatBRS@gmail.com>"
    url = "https://conan.cronologic.de/artifactory/pub/_/_/crono_pci_linux/" + version
    description = "Linux user mode driver to support large DMA ring buffers on a PCI bus"
    topics = ["cronologic", "pci", "abstraction", "linux"]
    settings = ["os", "compiler", "build_type", "arch", "distro"]

    # `CronoConanBase` variables initialization and export
    supported_os = ["Linux"]
    proj_src_indir = ".."
    export_source = True

    # ==========================================================================
    # Conan Methods
    #
    def requirements(self):
        self.requires(self.cronologic_linux_kernel_ref)

    # __________________________________________________________________________
    #
    def package(self):
        super().package(lib_name=self.name)

    # __________________________________________________________________________
