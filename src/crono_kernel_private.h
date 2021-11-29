/*++

Module Name:

crono_kernel_private.h

Abstract:

This module contains the kernel-only declarations.

Environment:

kernel

--*/

#ifndef __CRONO_KERNEL_PRIVATE_H__
#define __CRONO_KERNEL_PRIVATE_H__

#if defined(__cplusplus)
extern "C" {
#endif

#include "crono_kernel_interface.h"
#include "crono_linux_kernel.h"

typedef uint64_t DMA_ADDR;

/* Address space information struct */
typedef struct {
        uint64_t
            pUserDirectMemAddr; /* Memory address for direct user-mode access */
        size_t dwSize;          // Added for Linux, to be used with munmap
} CRONO_KERNEL_ADDR_DESC;

/* Device information struct */
typedef struct CRONO_KERNEL_DEVICE {
        CRONO_KERNEL_PCI_SLOT
        pciSlot; // PCI/PCMCIA device slot location information
        uint32_t dwVendorId;
        uint32_t dwDeviceId;

        CRONO_KERNEL_ADDR_DESC bar_addr; // BAR0 userspace mapped address

        /**
         * The name of the corresponding `miscdev` file, found under /dev
         */
        char miscdev_name[CRONO_DEV_NAME_MAX_SIZE];

        int miscdev_fd;

} CRONO_KERNEL_DEVICE, *PCRONO_KERNEL_DEVICE;

#define crono_sleep(x) usleep(1000 * x)

/**
 * Return Error Code `error_code` if value `value_to_validate` is NULL
 *
 * @param value_to_validate[in]: value to be validated, mostly a pointer.
 * @param error_code[in]: error code to be returned.
 */
#define CRONO_RET_ERR_CODE_IF_NULL(value_to_validate, error_code)              \
        if (NULL == (value_to_validate)) {                                     \
                return error_code;                                             \
        }
/**
 * Return Error Code `-EINVAL`
 * if value `value_to_validate` is NULL
 *
 * @param value_to_validate[in]: value to be validated, mostly a pointer.
 */
#define CRONO_RET_INV_PARAM_IF_NULL(value_to_validate)                         \
        if (NULL == (value_to_validate)) {                                     \
                return -EINVAL;                                                \
        }

/**
 * Return Error Code `-EINVAL`
 * if value `value_to_validate` is 0
 *
 * @param value_to_validate[in]: value to be validated, mostly a pointer.
 */
#define CRONO_RET_INV_PARAM_IF_ZERO(value_to_validate)                         \
        if (0 == (value_to_validate)) {                                        \
                return -EINVAL;                                                \
        }

#ifdef __cplusplus
}
#endif

#endif
