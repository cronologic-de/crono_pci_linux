/**
 * @file crono_userspace.h
 * @author Bassem Ramzy
 * @brief File has public macros and function prototypes needed by userspace
 * applications that communicate with cronologic PCI driver.
 * @version 0.1
 * @date 2021-11-09
 *
 * @copyright Copyright (c) 2021
 *
 */
#ifndef _CRONO_USERSPACE_H_
#define _CRONO_USERSPACE_H_

#include "crono_kernel_interface.h"
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define SYS_BUS_PCIDEVS_PATH "/sys/bus/pci/devices"
#define PAGE_SIZE sysconf(_SC_PAGE_SIZE)

/**
 * Constructs the configuration file path into 'config_file_path'.
 * `config_file_path` is char [PATH_MAX], and all device attributes are
 * unsigned.
 */
#define CRONO_CONSTRUCT_CONFIG_FILE_PATH(config_file_path, domain, bus, dev,   \
                                         func)                                 \
        snprintf(config_file_path, PATH_MAX, "%s/%04x:%02x:%02x.%1u/config",   \
                 SYS_BUS_PCIDEVS_PATH, domain, bus, dev, func);

/**
 * Constructs the /sys/bus/pci/devices/DBDF symbolic link path into
 * 'dev_slink_path'. `dev_slink_path` is char [PATH_MAX], and all device
 * attributes are unsigned.
 */
#define CRONO_CONSTRUCT_DEV_SLINK_PATH(dev_slink_path, domain, bus, dev, func) \
        snprintf(dev_slink_path, PATH_MAX, "%s/%04x:%02x:%02x.%1u",            \
                 SYS_BUS_PCIDEVS_PATH, domain, bus, dev, func);

/**
 * Reads data from devices configuration space using sysfs.
 *
 * @param domain[in]: The domain number of the device, 2 bytes value.
 * @param bus[in]: The bus number of the device, 1 byte value.
 * @param dev[in]: The device number of the device, 1 byte value.
 * @param func[in]: The function number of the device, 4-bits value.
 * @param data[out]: Pointer to the buffer to which the read data will be
 * copied. Should be of sufficient size.
 * @param offset[in]: Offset in bytes in configuratin space, starting from
 * which, the data will be read.
 * @param size[in]: The size of the data to be read in bytes.
 * @param bytes_read[out]: Pointer to a variable that will contain the number of
 *  bytes read successfully. If NULL, it will be ignored.
 *
 * @return `CRONO_SUCCESS` in case of no error, or `errno` in case of error.
 */
int crono_read_config(unsigned domain, unsigned bus, unsigned dev,
                      unsigned func, void *data, pciaddr_t offset,
                      pciaddr_t size, pciaddr_t *bytes_read);

/**
 * Reads Vendor ID and Device ID from device configuration space using sysfs.
 *
 * @param domain[in]: The domain number of the device, 2 bytes value.
 * @param bus[in]: The bus number of the device, 1 byte value.
 * @param dev[in]: The device number of the device, 1 byte value.
 * @param func[in]: The function number of the device, 4-bits value.
 * @param pVendor[out]: A valid pointer to the buffer that will contain the
 * Vendor ID.
 * @param pDevice[out]: A valid pointer to the buffer that will contain the
 * Device ID.
 *
 * @return `CRONO_SUCCESS` in case of no error, or `errno` in case of error.
 */
int crono_read_vendor_device(unsigned domain, unsigned bus, unsigned dev,
                             unsigned func, unsigned *pVendor,
                             unsigned *pDevice);

/**
 * Writes data to devices configuration space using sysfs.
 *
 * @param domain[in]: The domain number of the device, 2 bytes value.
 * @param bus[in]: The bus number of the device, 1 byte value.
 * @param dev[in]: The device number of the device, 1 byte value.
 * @param func[in]: The function number of the device, 4-bits value.
 * @param data[in]: Pointer to the buffer to which the written data will be
 * copied. Should be of sufficient size.
 * @param offset[in]: Offset in bytes in configuratin space, starting from
 * which, the data will be written.
 * @param size[in]: The size of the data to be written in bytes.
 * @param bytes_written[out]: Pointer to a variable that will contain the number
 * of bytes written successfully. If NULL, it will be ignored.
 *
 * @return `CRONO_SUCCESS` in case of no error, or `errno` in case of error.
 */
int crono_write_config(unsigned domain, unsigned bus, unsigned dev,
                       unsigned func, void *data, pciaddr_t offset,
                       pciaddr_t size, pciaddr_t *bytes_written);

/**
 * Gets the size of the device configuration space in bytes using sysfs.
 *
 * @param domain[in]: The domain number of the device, 2 bytes value.
 * @param bus[in]: The bus number of the device, 1 byte value.
 * @param dev[in]: The device number of the device, 1 byte value.
 * @param func[in]: The function number of the device, 4-bits value.
 * @param pSize[out]: A valid pointer to the buffer that will contain the size.
 *
 * @return `CRONO_SUCCESS` in case of no error, or `errno` in case of error.
 */
int crono_get_config_space_size(unsigned domain, unsigned bus, unsigned dev,
                                unsigned func, pciaddr_t *pSize);

/**
 * Get /sys/devices subdirectory of the device specifid by passed DBDF passed.
 * e.g. /sys/devices/pci0000:00/0000:00:1c.7/0000:03:00.0
 *
 * @param domain[in]: The domain number of the device, 2 bytes value.
 * @param bus[in]: The bus number of the device, 1 byte value.
 * @param dev[in]: The device number of the device, 1 byte value.
 * @param func[in]: The function number of the device, 4-bits value.
 * @param pPath[out]: A valid pointer to the buffer that will contain the path.
 *  should be of size = [PATH_MAX]
 *
 * @return `CRONO_SUCCESS` in case of no error, or
 * `errno `-EINVAL` in case of error.
 */
int crono_get_sys_devices_directory_path(unsigned domain, unsigned bus,
                                         unsigned dev, unsigned func,
                                         char *pPath);

/**
 * Gets the sysfs path of the BAR0 file, e.g.
 * /sys/devices/pci0000:00/0000:00:1c.7/0000:03:00.0/resource0
 *
 * @param domain[in]: The domain number of the device, 2 bytes value.
 * @param bus[in]: The bus number of the device, 1 byte value.
 * @param dev[in]: The device number of the device, 1 byte value.
 * @param func[in]: The function number of the device, 4-bits value.
 * @param pPath[out]: A valid pointer to the buffer that will contain the path.
 *  should be of size = [PATH_MAX]
 *
 * @return `CRONO_SUCCESS` in case of no error, or
 * `-EINVAL' in case of error.
 */
int crono_get_BAR0_file_path(unsigned domain, unsigned bus, unsigned dev,
                             unsigned func, char *pPath);

/**
 * Gets the size of BAR0 resource file, e.g.
 * /sys/devices/pci0000:00/0000:00:1c.7/0000:03:00.0/resource0
 *
 * @param domain[in]: The domain number of the device, 2 bytes value.
 * @param bus[in]: The bus number of the device, 1 byte value.
 * @param dev[in]: The device number of the device, 1 byte value.
 * @param func[in]: The function number of the device, 4-bits value.
 * @param pSize[out]: A valid pointer to the buffer that will contain the size.
 *
 * @return `CRONO_SUCCESS` in case of no error, or
 * `-EINVAL` in case of error.
 */
int crono_get_BAR0_file_size(unsigned domain, unsigned bus, unsigned dev,
                             unsigned func, pciaddr_t *pSize);

/**
 * Memory is read-write pointer.
 * Caller should call munmap() to delete the mappings after finalizing the task.
 *
 * @param domain[in]: The domain number of the device, 2 bytes value.
 * @param bus[in]: The bus number of the device, 1 byte value.
 * @param dev[in]: The device number of the device, 1 byte value.
 * @param func[in]: The function number of the device, 4-bits value.
 * @param dwOffset[in]: The starting offset of data in BAR memory. It is not
 *  necessary to be page-aligned. If larger than BAR0 memory size, then `ENOMEM`
 *  is returned.
 * @param size[in/out]: size of data to be mapped BAR in bytes. If set to zero
 *  then the full BAR0 memory will be mapped, and `size` wil contain this mapped
 * size. Otherwise, it might be changed to have the TOTAL size of the memory
 * mapped in case 'dwOffset' is not page-aligned. e.g. if dwOffset=100, size=10;
 * then returned size will be 110 instead, this is the size passed to munmap().
 * @param base_mem_addr[out]: Will contain the address of the mapped memory,
 *  page-aligned, e.g. if dwOffset is 100 and page size is 4090, this variable
 * will contain memory address starting 0 of the page and not 100; and
 * `data_mem_addr` will contain the address of the byte order 100 (if not NULL).
 * This is the address passed to munmap().
 * @param data_mem_addr[out]: Will contain the address of data memory @
 * 'dwOffset' regardless of the `dwOffset` page-alignment. Ignored if NULL.
 *
 * @return `CRONO_SUCCESS` in case of no error, or
 * `-EINVAL` in case of error.
 */
int crono_get_BAR0_mem_addr(unsigned domain, unsigned bus, unsigned dev,
                            unsigned func, pciaddr_t dwOffset, pciaddr_t *size,
                            void **base_mem_addr, void **data_mem_addr);
#endif // #define _CRONO_USERSPACE_H_
