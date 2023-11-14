/**
 * @file crono_linux_kernel.h
 * @author Bassem Ramzy
 * @brief File has public macros and function prototypes needed by userspace
 * applications that communicate with cronologic PCI driver module directly.
 * @version 0.1
 * @date 2021-11-09
 *
 * @copyright Copyright (c) 2021
 *
 */
#ifndef _CRONO_LINUX_KERNEL_H_
#define _CRONO_LINUX_KERNEL_H_

#ifdef CRONO_KERNEL_MODE
#include <linux/kernel.h>
#else
#include <stddef.h>
#include <stdint.h>
#endif // #ifdef CRONO_KERNEL_MODE

typedef uint64_t DMA_ADDR;

/**
 * @brief
 * Buffer info communicated with user space for scatter/gather memory
 */
typedef struct {
        // Buffer Information
        void *addr;  // Physical address of buffer, allocated by userspace.
        size_t size; // Size of the buffer in bytes.

        // Pages Information
        DMA_ADDR *pages; // Pages Physical Addresses, allocated by userspace,
                         // and filled by Kernel Module. Count of elements =
                         // `pages_count`.
        DMA_ADDR upages; // Is used exchangeably with `pages`. It
                         // is mainly provided for backward compatibility
                         // with kernel versions earlier than 5.6
        uint32_t pages_count; // Count pages in `pages`

        // Kernel Information
        int id; // Internal kernel ID of the buffer
} CRONO_SG_BUFFER_INFO;

/**
 * @brief
 * Buffer info communicated with user space for contiguous memory
 */
typedef struct {
        void *addr;  // Physical address of buffer, in userspace
        size_t size; // Size of the buffer in bytes.

        // Kernel Information
        int id; // Internal kernel ID of the buffer
} CRONO_CONTIG_BUFFER_INFO;

/**
 * CRONO PCI Driver Name passed in pci_driver structure, and is found under
 * /sys/bus/pci/drivers after installing the driver module.
 */
#define CRONO_PCI_DRIVER_NAME "crono_pci_driver"
/**
 * Maximum size of the misdev name string under /dev
 */
#define CRONO_DEV_NAME_MAX_SIZE 32
/**
 * pin_user_pages() number of pages per call to be accessed in misdev ioclt().
 */
#define GUP_NR_PER_CALL 512

/**
 * The macro is used to construct the misdev file name on /dev.
 *
 * @param dbdf[in]: is a valid structure of type `crono_dev_DBDF`.
 */
#define CRONO_CONSTRUCT_MISCDEV_NAME(dev_name, device_id, dbdf)                \
        snprintf(dev_name, CRONO_DEV_NAME_MAX_SIZE,                            \
                 "crono_%02X_%02X%02X%02X%01X", device_id, dbdf.domain,        \
                 dbdf.bus, dbdf.dev, dbdf.func);

struct crono_dev_DBDF {
        unsigned int domain;
        unsigned int bus;
        unsigned int dev;
        unsigned int func;
};

/**
 * Structure used to hold the clean commands information. It's the object passed
 * to `ioctl`.
 */
typedef struct {
        CRONO_KERNEL_CMD *cmds;
        uint64_t ucmds; // Is used exchangeably with `cmds`.
                        // It is mainly provided for backward compatibility
                        // with kernel versions earlier than 5.6
        uint32_t count; // Count of elements in `cmds`
} CRONO_KERNEL_CMDS_INFO;

/**
 * Command value passed to miscdev ioctl() to lock a memory buffer.
 * 'c' is for `cronologic`.
 */
#define IOCTL_CRONO_LOCK_BUFFER _IOWR('c', 0, CRONO_SG_BUFFER_INFO *)
/**
 * Command value passed to miscdev ioctl() to unlock a memory buffer
 * 'c' is for `cronologic`. Passing buffer wrapper ID in kernel module.
 */
#define IOCTL_CRONO_UNLOCK_BUFFER _IOWR('c', 1, int *)
/**
 * Command value passed to miscdev ioctl() to cleanup setup
 */
#define IOCTL_CRONO_CLEANUP_SETUP _IOWR('c', 2, CRONO_KERNEL_CMDS_INFO *)
/**
 * Command value passed to miscdev ioctl() to lock a contiguour memory buffer.
 * 'c' is for `cronologic`.
 */
#define IOCTL_CRONO_LOCK_CONTIG_BUFFER _IOWR('c', 3, CRONO_CONTIG_BUFFER_INFO *)
/**
 * Command value passed to miscdev ioctl() to unlock a contiguous memory buffer
 * 'c' is for `cronologic`. Passing buffer wrapper ID in kernel module.
 */
#define IOCTL_CRONO_UNLOCK_CONTIG_BUFFER _IOWR('c', 4, int *)

#endif // #ifndef _CRONO_LINUX_KERNEL_H_
