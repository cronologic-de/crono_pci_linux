import sys
sys.path.append("conan_utils/")
from CronoConanBase import CronoConanBase

class CronoPciLinuxConan(CronoConanBase):
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
    topics = ["cronologic", "pci", "abstraction", "linux"]
    settings = ["os", "compiler", "build_type", "arch"]

    # `CronoConanBase` variables initialization and export
    supported_os = ["Linux"]
    proj_src_indir = ".."
    exports = "conan_utils/*.py"
    export_source = True

    # ==========================================================================
    # Conan Methods
    #
    def requirements(self):
        self.requires(self.cronologic_linux_kernel_ref)

    # __________________________________________________________________________
    #
    def package(self):
        super().package(pack_src=True, lib_name=self.name)

    # __________________________________________________________________________
