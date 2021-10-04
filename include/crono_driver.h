#ifndef _CRONO_DRIVER_H_
#define _CRONO_DRIVER_H_

/**
 * Header file got from cronologic-de PCI Linux Driver found on
 * https://github.com/cronologic-de/cronologic_linux_kernel
 */
#ifdef CRONO_KERNEL_MODE
#include <linux/kernel.h>
#endif // #ifdef CRONO_KERNEL_MODE

/**
 * The header file used by external applications and userspace.
 */
typedef uint64_t DMA_ADDR;
typedef uint64_t KPTR;

typedef struct {
        DMA_ADDR pPhysicalAddr; // Physical address of page.
        uint32_t dwBytes;       // Size of page.
} CRONO_KERNEL_DMA_PAGE;

typedef struct {
#if !defined(CRONO_KERNEL_MODE) && defined(KERNEL_64BIT)
        PVOID64 pUserAddr;
#else
        void *pUserAddr; // Beginning of buffer.
#endif
        uint32_t dwBytes; // Size of buffer.
        uint32_t dwPages; // Number of pages in buffer.
        CRONO_KERNEL_DMA_PAGE *Page;
        void **kernel_pages; // Array of page pointers, Needed to be cached for
                             // `unpin_user_pages` Created, filled and freed by
                             // the driver module
        void *sgt; // SG Table. Created, filled and freed by the driver module
        uint32_t pinned_pages_nr; // Number of actual pages pinned, needed to be
                                  // known if pin failed
        uint32_t dwOptions;       // Allocation options:
} CRONO_KERNEL_DMA;

/**
 * Structure passed to ioctl() in the `arg` parameter.
 */
typedef struct {
        unsigned long npages;

        CRONO_KERNEL_DMA **ppDma; // Should be filled as it's the parameter sent
                                  // in `CRONO_KERNEL_DMASGBufLock`
        unsigned long ulPage; // Is used exchangeably with `ppDma[0]->Page`. It
                              // is mainly provided for backword compatibility
                              // with kernel versions earlier than 5.6
        unsigned long ulpDma; // Is used exchangeably with `ppDma[0]`. It is
                              // mainly provided for backword compatibility with
                              // kernel versions earlier than 5.6
        void *pBuf;
        size_t dwDMABufSize;
        unsigned long dwOptions;
} DMASGBufLock_parameters;

/**
 * CRONO PCI Driver Name passed in pci_driver structure, and is found under
 * /sys/bus/pci/drivers after installing the driver module.
 */
#define CRONO_PCI_DRIVER_NAME "crono_pci_driver"
/**
 * Maximum size of the misdev name string under /dev
 */
#define CRONO_MAX_DEV_NAME_SIZE 100
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
        snprintf(dev_name, CRONO_MAX_DEV_NAME_SIZE,                            \
                 "crono_%02X_%02X%02X%02X%01X", device_id, dbdf.domain,        \
                 dbdf.bus, dbdf.dev, dbdf.func);

struct crono_dev_DBDF {
        unsigned int domain;
        unsigned int bus;
        unsigned int dev;
        unsigned int func;
};

/**
 * Command value passed to miscdev ioctl() to lock a memory buffer.
 * 'c' is for `cronologic`.
 */
#define IOCTL_CRONO_LOCK_BUFFER _IOWR('c', 0, DMASGBufLock_parameters *)
/**
 * Command value passed to miscdev ioctl() to unlock a memory buffer
 * 'c' is for `cronologic`.
 */
#define IOCTL_CRONO_UNLOCK_BUFFER _IOWR('c', 1, DMASGBufLock_parameters *)
/**
 * Command value passed to miscdev ioctl() to cleanup setup
 */
#define IOCTL_CRONO_CLEANUP_SETUP _IOWR('c', 2, void *)

#endif // #define _CRONO_DRIVER_H_
