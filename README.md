# crono_pci_linux
Linux user mode driver to support large DMA ring buffers on a PCI bus.

It has been developed by cronologic GmbH & Co. KG for use in the drivers for its time measurement devices such as:
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
The project generates `crono_pci_linux.a` static lbrary that is used by applications that need to access the kernel driver.

## Architecture
<p align="center">
  <img src="https://user-images.githubusercontent.com/75851720/135757078-e01d9b67-afff-400f-b3e8-d58bd814fed3.png" width="75%" height="75%"/>
</p>

The userspace in this project represents the `OS Abstraction Layer` in the architecture. It interfaces with the driver module via `ioctl`.

`Crono Kernel Layer` is introduced in the [Driver Kernel Module Repository](https://github.com/cronologic-de/cronologic_linux_kernel).

##  Directory Structure
    .
    ├── include        # Header files to be included by application as well
    ├── src            # Userspace source files
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
- **CentOS** (CentOS-Stream-8-x86_64-20210927)(**)
  - 5.4.150-1.e18.elrepo.x86_64
  - 5.14.9-1.el8.elrepo.x86_64
- **Fedora** (Fedora-Workstation-Live-x86_64-34-1.2)(**)
  - 5.14.9-200.fc34.x86_64
- **Debian** (Debian GNU/Linux 11 (bullseye))(**)
  - 5.10.0-8-amd64
- **openSUSE** Leap 15.3(**)
  - 5.3.15-59.27-default

(**) Project code is built successfully, however, it was not tested on the devices.

---

# Build the Project
## Build Prerequisites
1. Copy the [`cronologic_linux_kernel` headers](https://github.com/cronologic-de/cronologic_linux_kernel/tree/main/include) under `src` folder.

## Build Using `make`
To build the project, run `make` command:
```CMD
$ make
```

### Target Output
The build target output is:
| Output | Builds | Description | 
| -------- | ------ | ----------- |
| `crono_pci_linux.a` | ``./build/bin/release_64/`` | The release version of the userspace static library |
| `crono_pci_linux.a` | ``./build/bin/debug_64/`` | The debug version of the userspace static library |

Temporary build files (e.g. `.o` files) are found under the directory ``./build/crono_pci_linux``.

### Makefiles and Build Versions
The following makefiles are used to build the project versions:
| Makefile | Builds | Description | 
| -------- | ------ | ----------- |
| ./Makefile | Debug </br> Release | Calls ALL makefiles in sub-directories. </br>This will build both the `debug` and `release` versions of the project.|
| ./src/Makefile | Debug </br> Release | This will build both the `debug` and `release` versions of the project.</br>Make options:</br>- **release_64**: Builds the release version.</br>- **debug_64**: Builds the debug version.</br>- **cleanrelease_64**: Cleans the release version.</br>- **cleandebug_64**: Cleans the debug version.</br>- all: release_64 debug_64.</br>- clean: cleanrelease_64 cleandebug_64.|
| ./MakefileCommon.mk | None | Contains the common functions used by makefile(s) |

### Build Prerequisites
| Distribution | Prerequisites | How to install prerequisites | 
| ------------ | ------------- | ---------------------------- |
| Ubuntu | `make` and `g++` | ```sudo apt-get install make g++``` |
| CentOS | `make` and `g++` | ```sudo yum install g++ make``` |
| Fedora | `make` and `g++` | ```sudo yum install g++ make``` <br>```sudo yum install kernel-devel-$(uname -r)```|
| Debian | `make` and `g++` | ```sudo apt-get install make g++``` |
| openSUSE | **sudo** access, `make` and `g++` | ```sudo zypper install make gcc-c++```<br>```sudo zypper in kernel-devel kernel-default-devel```<br>```sudo zypper up``` |

### General Notes
* You can check if `make` and `g++` are installed by running the following commands:
```CMD
make -v
```
and
```CMD
g++ -v
```

In Fedora, the following will be dipslayed in case `g++` is not installed,
```CMD
$ g++ -v
bash: g++: command not found...
Install package 'gcc-c++' to provide command 'g++'? [N/y]  
```
You just enter `y` and accept installing dependencies, if inquired.

### Clean the Output Files 
To clean the project all builds output:
```CMD
make clean
```
Or, you can clean a specific build as following:
```CMD
.../src$ sudo make cleandebug_64
.../src$ sudo make cleanrelease_64
```

## More Details

### Preprocessor Directives
| Identifier | Description | 
| ---------- | ----------- |
|`CRONO_DEBUG_ENABLED` and `DEBUG`| Debug mode.|

---

# Usage 
The library is provided mainly as a static library `crono_pci_linux.a` to be used by other applications to handle the devices.

## Header Files
All the provided APIs and macros are found in the header file [``crono_kernel_interface.h``](./include/crono_kernel_interface.h). 

Additionally, `BAR` and `Configuraion Space` utility functions prototypes are found in [``crono_userspace.h``](./include/crono_userspace.h). 

While, cronologic PCI driver module strucutres and definitions are found in the header file [``crono_linux_kernel.h``](./include/crono_linux_kernel.h), and is got from [`cronologic_linux_kernel`](https://github.com/cronologic-de/cronologic_linux_kernel/blob/main/include/crono_linux_kernel.h)
