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

# Build the Project
To build the project, run `make` command:
```CMD
$ make
```

## Target Output
The build target output is:
| Output | Builds | Description | 
| -------- | ------ | ----------- |
| `crono_userspace.a` | ``./build/bin/release_64/`` | The release version of the driver |
| `crono_userspace.a` | ``./build/bin/debug_64/`` | The debug version of the driver |

Temporary build files (e.g. `.o` files) are found under the directory ``./build/crono_userspace``.

## Makefiles and Build Versions
The following makefiles are used to build the project versions:
| Makefile | Builds | Description | 
| -------- | ------ | ----------- |
| ./Makefile | Debug </br> Release | Calls ALL makefiles in sub-directories. </br>This will build both the `debug` and `release` versions of the project.|
| ./userspace-src/Makefile | Debug </br> Release | This will build both the `debug` and `release` versions of the project.</br>Make options:</br>- **release_64**: Builds the release version.</br>- **debug_64**: Builds the debug version.</br>- **cleanrelease_64**: Cleans the release version.</br>- **cleandebug_64**: Cleans the debug version.</br>- all: release_64 debug_64.</br>- clean: cleanrelease_64 cleandebug_64.|
| ./MakefileCommon.mk | None | Contains the common functions used by makefile(s) |

## Build Prerequisites
### Ubuntu 
- Make sure that both `make` and `g++` packages are installed, or install them using: 
```CMD
sudo apt-get install make g++
```

### CentOS 
- Make sure that both `make` and `g++` packages are installed, or install them using: 
```CMD
sudo yum install g++ make
```

### Fedora
- Make sure that both `make` and `g++` packages are installed, or install them using: 
```CMD
sudo yum install gcc make
```
- If the kernel development is not installed on your system for the current kernel verision, you can install it as following
```CMD
sudo yum install kernel-devel-$(uname -r)
```

### Debian 
- Make sure that both `make` and `gcc` packages are installed, or install them using: 
```CMD
sudo apt-get install make gcc
```
- Make sure `modules` and `headers` of your current kernel version are installed, or install them using:
```CMD
sudo apt-get install linux-headers-$(uname -r) 
```

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


## Clean the Output Files 
To clean the project all builds output:
```CMD
make clean
```
Or, you can clean a specific build as following:
```CMD
.../userspace-src$ suod make cleandebug_64
.../userspace-src$ suod make cleanrelease_64
```

## More Details

### Preprocessor Directives
| Identifier | Description | 
| ---------- | ----------- |
|`CRONO_DEBUG_ENABLED` and `DEBUG`| Debug mode.|

---

