# cronologic_linux_usermode
Linux user mode driver to support large DMA ring buffers.

It has been developed by cronologic GmbH & Co. KG to support the drivers for its time measurement devices such as.
* [xHPTDC8](https://www.cronologic.de/products/tdcs/xhptdc8-pcie) 8 channel 13ps streaming time-to-digital converter
* [xTDC4](https://www.cronologic.de/products/tdcs/xtdc4-pcie) 4 channel 13ps common-start time-to-digital converter
* [Timetagger](https://www.cronologic.de/products/tdcs/timetagger) 500ps low cost TDCs
* [Ndigo6G-12](https://www.cronologic.de/products/adcs/ndigo6g-12) 6.4 Gsps 12 bit ADC digitizer board with pulse extraction and 13ps TDC.
* [Ndigo5G-10](https://www.cronologic.de/products/adcs/cronologic-ndigo5g-10) 5 Gsps 10 bit ADC digitizer board with pulse extraction

However, the module could be useful to any PCIe developer who is using large buffers that are scattered in PCI space but shall look contiguous in user space. 

The kernel mode part of the driver is available in a [separate repository](https://github.com/cronologic-de/cronologic_linux_kernel).

The initial code has been written by [Bassem Ramzy](https://github.com/Bassem-Ramzy) with support from [Richard Weinberger](https://github.com/richardweinberger). It is licensed unter [GPL3](LICENSE).

---

# The Project
The project generates `crono_userspace.a` static lbrary that is used by applications that need to access the kernel driver.

## Architecture
<p align="center">
  <img src="https://user-images.githubusercontent.com/75851720/135757078-e01d9b67-afff-400f-b3e8-d58bd814fed3.png" width="75%" height="75%"/>
</p>

The userspace in this project represents the `OS Abstraction Layer` in the architecture. It interfaces with the driver module via `ioctl`.

`Crono Kernel Layer` is introduced in the [Driver Kernel Module Repository](https://github.com/cronologic-de/cronologic_linux_kernel).

##  Directory Structure
    .
    ├── include        # Header files to be included by application as well
    ├── userspace-src  # Userspace source files
    ├── Makefile
    └── MakefileCommon.mk

## Supported Kernel Versions
The project is tested on Kernel Versions starting **5.0**.

## Supported Distributions
The project is tested on the following **64-bit** distributions:
- **Ubuntu** (ubuntu-20.04.1-desktop-amd64) 
  - 5.4.0-42-generic
  - 5.10.0-051000-generic
  - 5.11.0-37-generic
- **CentOS** (CentOS-Stream-8-x86_64-20210927):
  - 5.4.150-1.e18.elrepo.x86_64
  - 5.14.9-1.el8.elrepo.x86_64
- **Fedora** (Fedora-Workstation-Live-x86_64-34-1.2):
  - 5.14.9-200.fc34.x86_64

---
