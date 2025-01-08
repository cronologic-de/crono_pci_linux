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

/* Device information struct */
typedef struct CRONO_KERNEL_DEVICE {
        /**
         * PCI/PCMCIA device slot location information
         */
        CRONO_KERNEL_PCI_SLOT pciSlot;

        uint32_t dwVendorId;
        uint32_t dwDeviceId;

        /**
         * BARs userspace mapped descriptions
         * We just fill the bars that are present, with the corresponding
         * barNum. If the first BAR is BAR5 then *barCount=1 and barDescs[0]
         * will be filled with barNum=5.
         */
        CRONO_KERNEL_BAR_DESC bar_descs[6];

        /**
         * Count of valid elements in `bar_descs`, should be initialized with
         * ZERO
         */
        uint32_t bar_count;

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

uint32_t freeDeviceMem(PCRONO_KERNEL_DEVICE pDevice);

/**
 * @brief Fill `pDevice->bar_descs` if not filled before, also mmap the BARs and
 * set `pDevice->bar_count`. Returns immediately if previously done
 * (pDevice->bar_count > 0). Prereuiqisites:
 * - `pDevice->miscdev_fd` is valid for an opened file.
 * - `pDevice->pciSlot` is filled.
 *
 * @param pDevice
 * pDevice->pciSlot should be already set.
 * @return uint32_t
 */
uint32_t fill_device_bar_descriptions(PCRONO_KERNEL_DEVICE pDevice);

#ifdef __cplusplus
}
#endif

#endif
