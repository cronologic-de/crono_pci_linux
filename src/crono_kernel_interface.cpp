#include "crono_kernel_interface.h"
#include "crono_kernel_private.h"
#include "crono_linux_kernel.h"
#include "crono_userspace.h"

/**
 * Defines `pDevice` and `pDev_handle`, and initialize them from `hDev`
 * Returns `EINVAL` if hDev is NULL
 */
#define CRONO_INIT_HDEV_FUNC(hDev)                                             \
        PCRONO_KERNEL_DEVICE pDevice;                                          \
        CRONO_RET_ERR_CODE_IF_NULL(hDev, -EINVAL);                             \
        pDevice = (PCRONO_KERNEL_DEVICE)hDev;                                  \
        if (0 == pDevice->dwDeviceId) {                                        \
                return -EINVAL;                                                \
        }

/**
 * Validates that both `dwOffset` and `val` size are within the memory range.
 * Returns '-ENOMEM' if not.
 * Is part of the functions in this file, a prerequisite to have all used
 * variables defined and initialized in the functions. Applies only on BAR0
 */
#define CRONO_VALIDATE_MEM_RANGE                                               \
        if ((dwOffset + sizeof(val)) > pDevice->bar_descs[0].length) {         \
                return -ENOMEM;                                                \
        }

PCRONO_KERNEL_DEVICE devices[8];
int iNewDev = 0; // New device index in `devices`

uint32_t
CRONO_KERNEL_PciScanDevices(uint32_t dwVendorId, uint32_t dwDeviceId,
                            CRONO_KERNEL_PCI_SCAN_RESULT *pPciScanResult) {
        struct stat st;
        DIR *dr = NULL;
        struct dirent *en;
        unsigned domain, bus, dev, func;
        uint16_t vendor_id, device_id;
        int index_in_result = 0;

        // Don't freeDevicesMem() as devices may be counted again
        // while a device is already open for any reason.

        if (stat(SYS_BUS_PCIDEVS_PATH, &st) != 0) {
                perror("Error: PCI FS is not found.");
                return errno;
        }
        if (((st.st_mode) & S_IFMT) != S_IFDIR) {
                printf("Error: PCI FS is not a directory.\n");
                return errno;
        }
        dr = opendir(SYS_BUS_PCIDEVS_PATH); // open all or present directory
        if (!dr) {
                perror("Error: Can't open PCI directory.");
                return errno;
        }
        while ((en = readdir(dr)) != NULL) {
                if (en->d_type != DT_LNK) {
                        continue;
                }

                // Get PCI Domain, Bus, Device, and Function
                sscanf(en->d_name, "%04x:%02x:%02x.%1u", &domain, &bus, &dev,
                       &func);

                // Get Vendor ID and Device ID
                int ret = crono_read_vendor_device(domain, bus, dev, func,
                                                   &vendor_id, &device_id);
                if (CRONO_SUCCESS != ret) {
                        CRONO_DEBUG("Error <%d> finding devices\n", ret);
                        closedir(dr);
                        return ret;
                }

                // Check values & fill pPciScanResult if device matches the
                // Vendor/Device
                if (((vendor_id == dwVendorId) ||
                     (((uint32_t)PCI_ANY_ID) == dwVendorId)) &&
                    ((device_id == dwDeviceId) ||
                     (((uint32_t)PCI_ANY_ID) == dwDeviceId))) {
                        pPciScanResult->deviceId[index_in_result].dwDeviceId =
                            device_id;
                        pPciScanResult->deviceId[index_in_result].dwVendorId =
                            vendor_id;
                        pPciScanResult->deviceSlot[index_in_result].dwDomain =
                            domain;
                        pPciScanResult->deviceSlot[index_in_result].dwBus = bus;
                        pPciScanResult->deviceSlot[index_in_result].dwSlot =
                            dev;
                        pPciScanResult->deviceSlot[index_in_result].dwFunction =
                            func;
                        index_in_result++;
                        pPciScanResult->dwNumDevices = index_in_result;
                        CRONO_DEBUG("Added matched device in index <%d>\n",
                                    index_in_result - 1);
                } else {
                }
        }

        // Clean up
        closedir(dr);

        // Successfully scanned
        return CRONO_SUCCESS;
}

/**
 * Deprecated
 */
uint32_t
CRONO_KERNEL_PciGetDeviceInfo(CRONO_KERNEL_PCI_CARD_INFO *pDeviceInfo) {
        return CRONO_SUCCESS;
}

uint32_t
CRONO_KERNEL_PciDeviceOpen(CRONO_KERNEL_DEVICE_HANDLE *phDev,
                           const CRONO_KERNEL_PCI_CARD_INFO *pDeviceInfo) {
        DIR *dr = NULL;
        struct dirent *en;
        unsigned domain, bus, dev, func;
        int ret;
        PCRONO_KERNEL_DEVICE pDevice = nullptr;

        // Init variables and validate parameters
        CRONO_RET_INV_PARAM_IF_NULL(phDev);
        CRONO_RET_INV_PARAM_IF_NULL(pDeviceInfo);
        *phDev = NULL;

        dr = opendir(SYS_BUS_PCIDEVS_PATH); // open all or present directory
        if (!dr) {
                perror("Error: opening PCI directory");
                return errno;
        }
        while ((en = readdir(dr)) != NULL) {
                if (en->d_type != DT_LNK) {
                        continue;
                }

                // Get PCI Domain, Bus, Device, and Function
                sscanf(en->d_name, "%04x:%02x:%02x.%1u", &domain, &bus, &dev,
                       &func);

                // Check values & fill pDevice if device matches the
                // Vendor/Device
                if ((pDeviceInfo->pciSlot.dwDomain == domain) &&
                    (pDeviceInfo->pciSlot.dwBus == bus) &&
                    (pDeviceInfo->pciSlot.dwSlot == dev) &&
                    (pDeviceInfo->pciSlot.dwFunction == func)) {
                        // Allocate `pDevice` memory
                        pDevice = (PCRONO_KERNEL_DEVICE)malloc(
                            sizeof(CRONO_KERNEL_DEVICE));
                        // Save value if a later cleanup is needed
                        devices[iNewDev++] = pDevice;

                        // Initialize struct elements to zeros
                        memset(pDevice, 0, sizeof(CRONO_KERNEL_DEVICE));

                        // Set `pDevice` slot information
                        pDevice->pciSlot.dwDomain =
                            pDeviceInfo->pciSlot.dwDomain;
                        pDevice->pciSlot.dwBus = pDeviceInfo->pciSlot.dwBus;
                        pDevice->pciSlot.dwSlot = pDeviceInfo->pciSlot.dwSlot;
                        pDevice->pciSlot.dwFunction =
                            pDeviceInfo->pciSlot.dwFunction;

                        // Get device `Vendor ID` and `Device ID` and set them
                        // to `pDevice`
                        uint16_t vendor_id, device_id;
                        ret = crono_read_vendor_device(domain, bus, dev, func,
                                                       &vendor_id, &device_id);
                        if (CRONO_SUCCESS != ret) {
                                printf("Error getting vendor\n");
                                ret = CRONO_KERNEL_TRY_AGAIN;
                                goto device_error;
                        }
                        pDevice->dwDeviceId = device_id;
                        pDevice->dwVendorId = vendor_id;

                        // Get the device `miscdev` file name, and set it to
                        // `pDevice`
                        struct crono_dev_DBDF dbdf = {domain, bus, dev, func};
                        CRONO_CONSTRUCT_MISCDEV_NAME(pDevice->miscdev_name,
                                                     device_id, dbdf);
                        struct stat miscdev_stat;
                        char miscdev_path[PATH_MAX];
                        sprintf(miscdev_path, "/dev/%s", pDevice->miscdev_name);
                        if (stat(miscdev_path, &miscdev_stat) != 0) {
                                printf("Error: miscdev `%s` is not found. <%d> "
                                       "<%s>\n",
                                       miscdev_path, errno, strerror(errno));
                                ret = -EINVAL;
                                goto device_error;
                        }

                        // Open the miscellanous driver file
                        int miscdev_fd = open(miscdev_path, O_RDWR);
                        if (miscdev_fd <= 0) {
                                // Error opening the device
                                switch (errno) {
                                case EBUSY:
                                        // Mostly returned by the OS
                                        printf("Device of file descriptor <%d> "
                                               "is busy\n",
                                               pDevice->miscdev_fd);
                                        ret = CRONO_KERNEL_TRY_AGAIN;
                                        goto device_error;
                                case ENODEV:
                                        printf("No device found\n");
                                        ret = CRONO_KERNEL_NO_DEVICE_OBJECT;
                                        goto device_error;
                                default:
                                        printf("Error: cannot open device file "
                                               "<%s>. <%d> <%s>\n",
                                               miscdev_path, errno,
                                               strerror(errno));
                                        ret =
                                            CRONO_KERNEL_INSUFFICIENT_RESOURCES;
                                        goto device_error;
                                }
                        }

                        // Success
                        pDevice->miscdev_fd = miscdev_fd;
                        CRONO_DEBUG("Device <%s> is opened as <%d>.\n",
                                    pDevice->miscdev_name, pDevice->miscdev_fd);
                        // Set phDev
                        *phDev = pDevice;

                        // Successfully opened

                        // Set bar descriptions
                        ret = fill_device_bar_descriptions(pDevice);
                        if (ret != CRONO_SUCCESS) {
                                close(miscdev_fd);
                                goto device_error;
                        }

                        break; // Device is found and set, no further search is
                               // needed
                }
        }

        // Clean up
        closedir(dr);

        // Successfully scanned
        return CRONO_SUCCESS;

// Called after `iNewDev` is incremented with the new device
device_error:
        if (dr) {
                closedir(dr);
        }
        if (pDevice != nullptr) {
                free(pDevice);
        }
        iNewDev--; // Device freed, drop it from `devices`
        return ret;
}

uint32_t CRONO_KERNEL_PciDeviceClose(CRONO_KERNEL_DEVICE_HANDLE hDev) {
        // Init variables and validate parameters
        int ret = CRONO_SUCCESS;
        CRONO_INIT_HDEV_FUNC(hDev);

        // Close the device
        if (-1 == close(pDevice->miscdev_fd)) {
                // Error
                printf("Error: cannot close device file descriptor "
                       "<%d>: <%d> <%s> \n",
                       pDevice->miscdev_fd, errno, strerror(errno));
                return errno;
        }
        CRONO_DEBUG("Device <%s> is closed as <%d>.\n", pDevice->miscdev_name,
                    pDevice->miscdev_fd);

        // Free memory allocated in CRONO_KERNEL_PciDeviceOpen
        ret = freeDeviceMem(pDevice);
        if (CRONO_SUCCESS != ret) {
                return ret;
        }

        return ret;
}

/* -----------------------------------------------
Set card cleanup commands
----------------------------------------------- */
uint32_t CRONO_KERNEL_CardCleanupSetup(CRONO_KERNEL_DEVICE_HANDLE hDev,
                                       CRONO_KERNEL_CMD *Cmd,
                                       uint32_t dwCmdCount) {
        int ret = CRONO_SUCCESS;
        CRONO_KERNEL_CMDS_INFO cmds_info;

        // Init variables and validate parameters
        CRONO_INIT_HDEV_FUNC(hDev);
        if (pDevice->miscdev_fd <= 0) {
                printf("Error: CRONO_KERNEL_PciDeviceOpen must be called "
                       "before calling "
                       "CRONO_KERNEL_CardCleanupSetup()\n");
                return -ENOENT;
        }

        cmds_info.cmds =
            (CRONO_KERNEL_CMD *)calloc(dwCmdCount, sizeof(CRONO_KERNEL_CMD));
        for (uint32_t i = 0; i < dwCmdCount; i++) {
                cmds_info.cmds[i].addr = Cmd[i].addr;
                cmds_info.cmds[i].data = Cmd[i].data;
        }
        cmds_info.ucmds = (uint64_t)cmds_info.cmds;
        cmds_info.count = dwCmdCount;

        // Call ioctl
        ret = ioctl(pDevice->miscdev_fd, IOCTL_CRONO_CLEANUP_SETUP, &cmds_info);

        // Cleanup and return
        free(cmds_info.cmds);
        return ret;
}

uint32_t CRONO_KERNEL_PciReadCfg32(CRONO_KERNEL_DEVICE_HANDLE hDev,
                                   uint32_t dwOffset, uint32_t *val) {
        pciaddr_t bytes_read;
        int ret = CRONO_SUCCESS;
        pciaddr_t config_space_size = 0;

        // Init variables and validate parameters
        CRONO_INIT_HDEV_FUNC(hDev);
        CRONO_RET_INV_PARAM_IF_NULL(val);

        // Validate offset and data size are within configuration spcae size
        ret = crono_get_config_space_size(
            pDevice->pciSlot.dwDomain, pDevice->pciSlot.dwBus,
            pDevice->pciSlot.dwSlot, pDevice->pciSlot.dwFunction,
            &config_space_size);
        if (CRONO_SUCCESS != ret) {
                return ret;
        }
        if (config_space_size < (dwOffset + sizeof(*val))) {
                return -EINVAL;
        }

        // Read the configurtion
        ret = crono_read_config(pDevice->pciSlot.dwDomain,
                                pDevice->pciSlot.dwBus, pDevice->pciSlot.dwSlot,
                                pDevice->pciSlot.dwFunction, val, dwOffset,
                                sizeof(*val), &bytes_read);
        if ((bytes_read != 4) || ret) {
                return ret;
        }

        // Success
        return CRONO_SUCCESS;
}

uint32_t CRONO_KERNEL_PciWriteCfg32(CRONO_KERNEL_DEVICE_HANDLE hDev,
                                    uint32_t dwOffset, uint32_t val) {
        pciaddr_t bytes_written;
        int ret = CRONO_SUCCESS;
        pciaddr_t config_space_size = 0;

        // Init variables and validate parameters
        CRONO_INIT_HDEV_FUNC(hDev);

        // Validate offset and data size are within configuration spcae size
        ret = crono_get_config_space_size(
            pDevice->pciSlot.dwDomain, pDevice->pciSlot.dwBus,
            pDevice->pciSlot.dwSlot, pDevice->pciSlot.dwFunction,
            &config_space_size);
        if (CRONO_SUCCESS != ret) {
                return ret;
        }
        if (config_space_size < (dwOffset + sizeof(val))) {
                return -EINVAL;
        }

        // Write the configuration
        ret = crono_write_config(
            pDevice->pciSlot.dwDomain, pDevice->pciSlot.dwBus,
            pDevice->pciSlot.dwSlot, pDevice->pciSlot.dwFunction, &val,
            dwOffset, sizeof(val), &bytes_written);
        if ((bytes_written != 4) || ret) {
                return ret;
        }

        // Success
        return CRONO_SUCCESS;
}

uint32_t CRONO_KERNEL_ReadAddr8(CRONO_KERNEL_DEVICE_HANDLE hDev,
                                uint32_t dwOffset, uint8_t *val) {
        // Init variables and validate parameters
        CRONO_INIT_HDEV_FUNC(hDev);
        CRONO_RET_ERR_CODE_IF_NULL(val, -ENOMEM);
        CRONO_VALIDATE_MEM_RANGE;

        // Copy memory
        *val =
            *((volatile unsigned char
                   *)(((unsigned char *)(pDevice->bar_descs[0].userAddress)) +
                      dwOffset));

        // Success
        return CRONO_SUCCESS;
}

uint32_t CRONO_KERNEL_ReadAddr16(CRONO_KERNEL_DEVICE_HANDLE hDev,
                                 uint32_t dwOffset, unsigned short *val) {
        // Init variables and validate parameters
        CRONO_INIT_HDEV_FUNC(hDev);
        CRONO_RET_ERR_CODE_IF_NULL(val, -ENOMEM);
        CRONO_VALIDATE_MEM_RANGE;

        // Copy memory
        *val =
            *((volatile unsigned short
                   *)(((unsigned char *)(pDevice->bar_descs[0].userAddress)) +
                      dwOffset));

        // Success
        return CRONO_SUCCESS;
}

uint32_t CRONO_KERNEL_ReadAddr32(CRONO_KERNEL_DEVICE_HANDLE hDev,
                                 uint32_t dwOffset, uint32_t *val) {
        // Init variables and validate parameters
        CRONO_INIT_HDEV_FUNC(hDev);
        CRONO_RET_ERR_CODE_IF_NULL(val, -ENOMEM);
        CRONO_VALIDATE_MEM_RANGE;

        *val = *((volatile uint32_t *)(((unsigned char *)(pDevice->bar_descs[0]
                                                              .userAddress)) +
                                       dwOffset));

        // Success
        return CRONO_SUCCESS;
}

uint32_t CRONO_KERNEL_ReadAddr64(CRONO_KERNEL_DEVICE_HANDLE hDev,
                                 uint32_t dwOffset, uint64_t *val) {
        // Init variables and validate parameters
        CRONO_INIT_HDEV_FUNC(hDev);
        CRONO_RET_ERR_CODE_IF_NULL(val, -ENOMEM);
        CRONO_VALIDATE_MEM_RANGE;

        // Copy memory
        *val = *((volatile uint64_t *)(((unsigned char *)(pDevice->bar_descs[0]
                                                              .userAddress)) +
                                       dwOffset));

        // Success
        return CRONO_SUCCESS;
}

uint32_t CRONO_KERNEL_WriteAddr8(CRONO_KERNEL_DEVICE_HANDLE hDev,
                                 uint32_t dwOffset, unsigned char val) {
        // Init variables and validate parameters
        CRONO_INIT_HDEV_FUNC(hDev);
        CRONO_VALIDATE_MEM_RANGE;

        // Copy memory
        *((volatile unsigned char *)(((unsigned char *)(pDevice->bar_descs[0]
                                                            .userAddress)) +
                                     dwOffset)) = val;

        // Success
        return CRONO_SUCCESS;
}

uint32_t CRONO_KERNEL_WriteAddr16(CRONO_KERNEL_DEVICE_HANDLE hDev,
                                  uint32_t dwOffset, unsigned short val) {
        // Init variables and validate parameters
        CRONO_INIT_HDEV_FUNC(hDev);
        CRONO_VALIDATE_MEM_RANGE;

        // Copy memory
        *((volatile unsigned short *)(((unsigned char *)(pDevice->bar_descs[0]
                                                             .userAddress)) +
                                      dwOffset)) = val;

        // Success
        return CRONO_SUCCESS;
}

uint32_t CRONO_KERNEL_WriteAddr32(CRONO_KERNEL_DEVICE_HANDLE hDev,
                                  uint32_t dwOffset, uint32_t val) {
        // Init variables and validate parameters
        CRONO_INIT_HDEV_FUNC(hDev);
        CRONO_VALIDATE_MEM_RANGE;

        // Copy memory
        *((volatile uint32_t *)(((unsigned char *)(pDevice->bar_descs[0]
                                                       .userAddress)) +
                                dwOffset)) = val;

        // Success
        return CRONO_SUCCESS;
}

uint32_t CRONO_KERNEL_WriteAddr64(CRONO_KERNEL_DEVICE_HANDLE hDev,
                                  uint32_t dwOffset, uint64_t val) {
        // Init variables and validate parameters
        CRONO_INIT_HDEV_FUNC(hDev);
        CRONO_VALIDATE_MEM_RANGE;

        // Copy memory
        *((volatile uint64_t *)(((unsigned char *)(pDevice->bar_descs[0]
                                                       .userAddress)) +
                                dwOffset)) = val;

        // Success
        return CRONO_SUCCESS;
}

uint32_t CRONO_KERNEL_GetBarPointer(CRONO_KERNEL_DEVICE_HANDLE hDev,
                                    uint32_t *barPointer) {
        // Init variables and validate parameters
        CRONO_INIT_HDEV_FUNC(hDev);
        CRONO_RET_INV_PARAM_IF_NULL(barPointer);

        barPointer = (uint32_t *)(pDevice->bar_descs[0].userAddress);

        // Success
        return CRONO_SUCCESS;
}

/**
 * @brief
 * - Allocate `ppDma` content.
 * - "INTERNALLY" allocate `buff_info` (`CRONO_SG_BUFFER_INFO`), used for
 *   communication with the kernel module.
 * - Call `ioctl` using `IOCTL_CRONO_LOCK_BUFFER` to allocate kernel internal
 *   information and lock the buffer, returning physical memory info.
 * - Fill `ppDma` content with the physical memory info, and `pDma->id` with
 *   the kernel module buffer id, used for next communication (e.g. unlock).
 *
 * @param hDev [in]
 * @param pBuf [in]
 * @param dwOptions [in]
 * @param dwDMABufSize [in]
 * @param ppDma [out]
 * @return uint32_t
 * `CRONO_SUCCESS` or error code.
 */
uint32_t CRONO_KERNEL_DMASGBufLock(CRONO_KERNEL_DEVICE_HANDLE hDev, void *pBuf,
                                   uint32_t dwOptions, uint32_t dwDMABufSize,
                                   CRONO_KERNEL_DMA_SG **ppDma) {
        int ret = CRONO_SUCCESS;
        CRONO_SG_BUFFER_INFO buff_info;
        CRONO_KERNEL_DMA_SG *pDma = NULL;

        // ______________________________________
        // Init variables and validate parameters
        //
        CRONO_DEBUG("Locking Buffer: address <%p>, size <%u>\n", pBuf,
                    dwDMABufSize);
        CRONO_INIT_HDEV_FUNC(hDev);
        CRONO_RET_INV_PARAM_IF_NULL(pBuf);
        CRONO_RET_INV_PARAM_IF_NULL(ppDma);
        CRONO_RET_INV_PARAM_IF_ZERO(dwDMABufSize);
        if (pDevice->miscdev_fd <= 0) {
                printf("Error: CRONO_KERNEL_PciDeviceOpen must be called "
                       "before calling CRONO_KERNEL_DMASGBufLock()\n");
                return -ENOENT;
        }

        // Construct buff_info
        buff_info.addr = pBuf;
        buff_info.size = dwDMABufSize;
        buff_info.pages = NULL;
        buff_info.id = -1; // Initialize with invalid value

        // Allocate the DMA Pages Memory
        pDma = (CRONO_KERNEL_DMA_SG *)malloc(sizeof(CRONO_KERNEL_DMA_SG));
        if (NULL == pDma) {
                printf("Error allocating DMA struct memory");
                return -ENOMEM;
        }
        memset(pDma, 0, sizeof(CRONO_KERNEL_DMA_SG));
#define DIV_ROUND_UP(n, d) (((n) + (d)-1) / (d))
        buff_info.pages_count = pDma->dwPages =
            DIV_ROUND_UP(dwDMABufSize, PAGE_SIZE);
        pDma->pUserAddr = pBuf;
        CRONO_DEBUG("Allocating memory: size <%lu>, pages count <%d>\n",
                    sizeof(CRONO_KERNEL_DMA_PAGE) * pDma->dwPages,
                    pDma->dwPages);
        pDma->Page = (CRONO_KERNEL_DMA_PAGE *)malloc(
            sizeof(CRONO_KERNEL_DMA_PAGE) * pDma->dwPages);
        if (NULL == pDma->Page) {
                printf("Error allocating Page memory");
                free(pDma);
                return -ENOMEM;
        }
        CRONO_DEBUG("Allocated `ppDma[0]->Page`: size <%ld>\n",
                    sizeof(CRONO_KERNEL_DMA_PAGE) * pDma->dwPages);
        *ppDma = pDma; // Do not ppDma = &pDma

        CRONO_DEBUG("Allocating memory: size <%lu>, pages count <%d>\n",
                    sizeof(uint64_t) * buff_info.pages_count,
                    buff_info.pages_count);
        buff_info.pages =
            (uint64_t *)malloc(sizeof(uint64_t) * buff_info.pages_count);
        if (NULL == buff_info.pages) {
                printf("Error allocating Page memory");
                free(pDma->Page);
                free(pDma);
                return -ENOMEM;
        }
        buff_info.upages = (DMA_ADDR)buff_info.pages;

        // ___________
        // Lock Buffer
        //
        CRONO_DEBUG("Locking buff_info: address <%p>, buffer size <%lu>, "
                    "pages count <%d>\n",
                    &buff_info, buff_info.size, buff_info.pages_count);
        // `pDevice->miscdev_fd` Must be already opened
        ret = ioctl(pDevice->miscdev_fd, IOCTL_CRONO_LOCK_BUFFER, &buff_info);
        if (CRONO_SUCCESS != ret) {
                printf("Driver module error %d\n", ret);
                goto alloc_err;
        }

        pDma->id = buff_info.id;
        CRONO_DEBUG("Copying locked addresses: ID <%d>, pages count <%d>\n",
                    pDma->id, buff_info.pages_count);

        for (uint64_t iPage = 0; iPage < buff_info.pages_count; iPage++) {
                pDma->Page[iPage].pPhysicalAddr = buff_info.pages[iPage];
                pDma->Page[iPage].dwBytes = 4096;
        }

#ifdef CRONO_DEBUG_ENABLED
        for (unsigned int ipage = 0;
             ipage < (pDma->dwPages < 5 ? pDma->dwPages : 5); ipage++) {
                fprintf(stdout, "Buffer Page <%d> Physical Address is <%p>\n",
                        ipage, (void *)(pDma->Page[ipage].pPhysicalAddr));
        }
#endif
        // ___________________
        // Cleanup, and return
        //
        CRONO_DEBUG("Done locking SG buffer id <%d>.\n", pDma->id);
        free(buff_info.pages);
        return ret;

alloc_err:
        if (NULL != pDma) {
                if (NULL != pDma->Page) {
                        free(pDma->Page);
                }
                free(pDma);
        }
        if (NULL != buff_info.pages) {
                free(buff_info.pages);
        }

        return ret;
}

/**
 * @brief
 * Unlock the buffer previously locked by `CRONO_KERNEL_DMASGBufLock`. and
 * clea its data.
 *
 * @param hDev [in]
 * @param pDma [in]
 * `pDma->id` is the kernel module internal id returned in `lock` function.
 * @return uint32_t
 */
uint32_t CRONO_KERNEL_DMASGBufUnlock(CRONO_KERNEL_DEVICE_HANDLE hDev,
                                     CRONO_KERNEL_DMA_SG *pDma) {
        int ret = CRONO_SUCCESS;

        // ______________________________________
        // Init variables and validate parameters
        //
        CRONO_DEBUG("Unlocking buffer...\n");
        CRONO_INIT_HDEV_FUNC(hDev);
        CRONO_RET_INV_PARAM_IF_NULL(pDma);
        CRONO_DEBUG("Buffer: id <%d>\n", pDma->id);
        if (pDevice->miscdev_fd <= 0) {
                printf("Error: CRONO_KERNEL_PciDeviceOpen must be called "
                       "before calling CRONO_KERNEL_DMASGBufUnlock()\n");
                return -ENOENT;
        }

        // _____________
        // Unlock Buffer
        //
        // Call ioctl() to unlock the buffer and cleanup
        // `pDevice->miscdev_fd` Must be already opened
        if (CRONO_SUCCESS !=
            (ret = ioctl(pDevice->miscdev_fd, IOCTL_CRONO_UNLOCK_BUFFER,
                         &pDma->id))) {
                return ret;
        }

        CRONO_DEBUG("Done unlocking buffer id <%d>.\n", pDma->id);

        // _______
        // Cleanup
        //
        // Free memory
        if (NULL != pDma->Page) {
                free(pDma->Page);
                pDma->Page = NULL;
        }
        free(pDma); // Preallocated in `CRONO_KERNEL_DMASGBufLock`

        return ret;
}

#include <sys/sysinfo.h>
void printFreeMemInfoDebug(const char *msg) {
#ifdef CRONO_DEBUG_ENABLED
        struct sysinfo info;
        sysinfo(&info);
        printf("%s: %ld in bytes / %ld in KB / %ld in MB / %ld in GB\n", msg,
               info.freeram, info.freeram / 1024, (info.freeram / 1024) / 1024,
               ((info.freeram / 1024) / 1024) / 1024);
#endif
}

CRONO_KERNEL_API uint32_t
CRONO_KERNEL_GetDeviceBARMem(CRONO_KERNEL_DEVICE_HANDLE hDev,
                             uint64_t *pBARUSAddr, size_t *pBARMemSize) {
        // Init variables and validate parameters
        int ret = CRONO_SUCCESS;
        CRONO_INIT_HDEV_FUNC(hDev);
        CRONO_RET_INV_PARAM_IF_NULL(pBARUSAddr);
        CRONO_RET_INV_PARAM_IF_NULL(pBARMemSize);

        // Copy values
        *pBARUSAddr = pDevice->bar_descs[0].userAddress;
        *pBARMemSize = pDevice->bar_descs[0].length;

        // Return result
        return ret;
}

CRONO_KERNEL_API uint32_t CRONO_KERNEL_GetDeviceMiscName(
    CRONO_KERNEL_DEVICE_HANDLE hDev, char *pMiscName, int nBuffSize) {
        // Init variables and validate parameters
        int ret = CRONO_SUCCESS;
        CRONO_INIT_HDEV_FUNC(hDev);
        CRONO_RET_INV_PARAM_IF_NULL(pMiscName);

        // Copy value
        strncpy(pMiscName, pDevice->miscdev_name, nBuffSize);

        // Return result
        return ret;
}

CRONO_KERNEL_API uint32_t CRONO_KERNEL_PciDriverVersion(
    CRONO_KERNEL_DEVICE_HANDLE hDev, CRONO_KERNEL_VERSION *pVersion) {
        // Needs Implemententation
        return CRONO_SUCCESS;
}

CRONO_KERNEL_API const char *Stat2Str(uint32_t dwStatus) {
        // Added for windows version compabatilbility
        return "Status\n";
}

/* Write byte array tp the PCI configuration space.
Identify device by handle */
uint32_t CRONO_KERNEL_PciWriteCfg32Arr(CRONO_KERNEL_DEVICE_HANDLE hDev,
                                       uint32_t dwOffset, uint32_t *val,
                                       uint32_t arr_size) {
        uint32_t code = CRONO_KERNEL_STATUS_SUCCESS;
        // Not implemented
        return code;
}

/**
 * - If not preallocated (ppDma is null):
 *   - Allocate `ppDma` content.
 *   - "INTERNALLY" allocate `buff_info` (`CRONO_CONTIG_BUFFER_INFO`), used for
 *     communication with the kernel module.
 *   - Call `ioctl` using `IOCTL_CRONO_LOCK_CONTIG_BUFFER`
 *     to allocate kernel internal information and lock the buffer , returning
 *     physical memory info.
 *   - Fill `ppDma` content with the physical memory info, and `pDma->id` with
 *     the kernel module buffer id, used for next communication (e.g. unlock).
 * - If preallocated, jsut return its physical address immediately, as it's
 *   already locked by kernel module when allocated.
 *
 * @param hDev
 * @param ppBuf
 * @param dwOptions
 * @param dwDMABufSize
 * @param ppDma
 * @return uint32_t
 */
uint32_t CRONO_KERNEL_DMAContigBufLock(CRONO_KERNEL_DEVICE_HANDLE hDev,
                                       void **ppBuf, uint32_t dwOptions,
                                       uint32_t dwDMABufSize,
                                       CRONO_KERNEL_DMA_CONTIG **ppDma) {
        int ret = CRONO_SUCCESS;
        CRONO_CONTIG_BUFFER_INFO buff_info;
        CRONO_KERNEL_DMA_CONTIG *pDma = NULL;

        // ______________________________________
        // Init variables and validate parameters
        //
        CRONO_RET_INV_PARAM_IF_NULL(ppDma);
        if (*ppDma) {
                // It's already preallocated
                CRONO_DEBUG("Locking Preallocated Buffer: size <%u>\n",
                            dwDMABufSize);
                CRONO_DEBUG(
                    "Done locking preallocated contiguous buffer id <%d>.\n",
                    pDma->id);
                // Buffer is "preallocated", just return
                return ret;
        }

        // New buffer needs to be allocated
        CRONO_DEBUG("Allocating and locking Contiguous Buffer: size <%u>\n",
                    dwDMABufSize);

        CRONO_INIT_HDEV_FUNC(hDev);
        CRONO_RET_INV_PARAM_IF_NULL(ppBuf);
        CRONO_RET_INV_PARAM_IF_ZERO(dwDMABufSize);
        if (pDevice->miscdev_fd <= 0) {
                printf("Error: CRONO_KERNEL_PciDeviceOpen must be called "
                       "before calling CRONO_KERNEL_DMAContigBufLock()\n");
                return -ENOENT;
        }

        // _______________________
        // Allocate and Map Buffer
        //
        // Initialize `buff_info`
        memset(&buff_info, 0, sizeof(CRONO_CONTIG_BUFFER_INFO));
        buff_info.size = dwDMABufSize;
        buff_info.id = -1;

        // Allocate memory
        // `pDevice->miscdev_fd` Must be already opened
        ret = ioctl(pDevice->miscdev_fd, IOCTL_CRONO_LOCK_CONTIG_BUFFER,
                    &buff_info);
        if (CRONO_SUCCESS != ret) {
                printf("Driver module error %d\n", ret);
                return ret;
        }

        // __________________________
        // Fill in returned variables
        //
        // Allocate and initialize `pDma`
        pDma =
            (CRONO_KERNEL_DMA_CONTIG *)malloc(sizeof(CRONO_KERNEL_DMA_CONTIG));
        if (NULL == pDma) {
                // $$ CRONO_KERNEL_DMAContigBufUnlock
                perror("Error allocating DMA struct memory");
                return -ENOMEM;
        }
        memset(pDma, 0, sizeof(CRONO_KERNEL_DMA_CONTIG));
        *ppDma = pDma;
        pDma->dwBytes = dwDMABufSize;
        pDma->pPhysicalAddr = (DMA_ADDR)buff_info.dma_handle;
        pDma->id = buff_info.id;

        // `mmap` `offset` argument should be aligned on a page boundary, so the
        // buffer id is sent to `mmap` multiplied by PAGE_SIZE.
        buff_info.pUserAddr = pDma->pUserAddr =
            mmap(NULL, dwDMABufSize, PROT_READ | PROT_WRITE, MAP_SHARED,
                 pDevice->miscdev_fd, buff_info.id * PAGE_SIZE);
        if (pDma->pUserAddr == MAP_FAILED) {
                perror("Failed to map DMA memory to user space");
                // $$ CRONO_KERNEL_DMAContigBufUnlock
                free(pDma);
                return -ENOMEM;
        }

        // Set `ppBuf`
        *ppBuf = buff_info.pUserAddr;

        // ___________________
        // Cleanup, and return
        //
        CRONO_DEBUG("Done locking contiguous buffer id <%d>."
                    "Physical address: <0x%lx>, User Address <%p>\n",
                    buff_info.id, pDma->pPhysicalAddr, buff_info.pUserAddr);
        return ret;
}

uint32_t CRONO_KERNEL_DMAContigBufUnlock(CRONO_KERNEL_DEVICE_HANDLE hDev,
                                         CRONO_KERNEL_DMA_CONTIG *pDma) {
        int ret = CRONO_SUCCESS;

        // ______________________________________
        // Init variables and validate parameters
        //
        CRONO_DEBUG("Unlocking buffer...\n");
        CRONO_INIT_HDEV_FUNC(hDev);
        CRONO_RET_INV_PARAM_IF_NULL(pDma);
        CRONO_DEBUG("Buffer: id <%d>\n", pDma->id);
        if (pDevice->miscdev_fd <= 0) {
                printf("Error: CRONO_KERNEL_PciDeviceOpen must be called "
                       "before calling CRONO_KERNEL_DMAContigBufUnlock()\n");
                return -ENOENT;
        }

        // _____________
        // Unlock Buffer
        //
        // Call ioctl() to unlock the buffer and cleanup
        // `pDevice->miscdev_fd` Must be already opened
        if (CRONO_SUCCESS !=
            (ret = ioctl(pDevice->miscdev_fd, IOCTL_CRONO_UNLOCK_CONTIG_BUFFER,
                         &pDma->id))) {
                printf("Driver module error %d\n", ret);
                return ret;
        }

        // _______
        // Cleanup
        //
        // Unmap and free memory - no map is done
        if (munmap(pDma->pUserAddr, pDma->dwBytes) < 0) {
                printf("Failed to unmap memory\n");
        }
        free(pDma);

        CRONO_DEBUG("Done unlocking buffer id <%d>.\n", pDma->id);
        return CRONO_SUCCESS;
}

CRONO_KERNEL_API uint32_t CRONO_KERNEL_GetBarDescriptions(
    CRONO_KERNEL_DEVICE_HANDLE hDev, uint32_t *barCount,
    CRONO_KERNEL_BAR_DESC *barDescs) {

        int ret = CRONO_SUCCESS;
        CRONO_INIT_HDEV_FUNC(hDev);
        CRONO_RET_INV_PARAM_IF_NULL(barCount);
        CRONO_RET_INV_PARAM_IF_NULL(barDescs);

        ret = fill_device_bar_descriptions(pDevice);
        if (ret != CRONO_SUCCESS)
                return ret;

        // Already got before, use it
        memcpy(barDescs, pDevice->bar_descs, sizeof(CRONO_KERNEL_BAR_DESC) * 6);
        *barCount = pDevice->bar_count;
        return CRONO_SUCCESS;
}

uint32_t fill_device_bar_descriptions(PCRONO_KERNEL_DEVICE pDevice) {

        int ret = CRONO_SUCCESS;
        char sys_dev_dir_path[PATH_MAX - 11]; // 11 for "/resourceN"
        uint32_t bar_count = 0;
        CRONO_KERNEL_BAR_DESC temp_bar_descs[6];

        // ______________________________________
        // Init variables and validate parameters
        //
        CRONO_RET_INV_PARAM_IF_NULL(pDevice);
        memset(temp_bar_descs, 0, sizeof(CRONO_KERNEL_BAR_DESC) * 6);

        // ________________________________________
        // Return if descripions are previously set
        //
        if (pDevice->bar_count > 0) {
                // Assuming `bar_count` is initialized with zero
                return CRONO_SUCCESS;
        }

        // ___________________________________
        // Get existing BARs, and fill structs
        //
        ret = crono_get_sys_devices_directory_path(
            pDevice->pciSlot.dwDomain, pDevice->pciSlot.dwBus,
            pDevice->pciSlot.dwSlot, pDevice->pciSlot.dwFunction,
            sys_dev_dir_path);
        if (CRONO_SUCCESS != ret) {
                printf("Device path error <%d>\n", ret);
                return ret;
        }

        // Open the `resource` file to get memory addresses and flags from
        std::string resource_file_path =
            std::string(sys_dev_dir_path) + "/resource";
        CRONO_DEBUG("Getting bar descriptions for resource file <%s>\n",
                    resource_file_path.c_str());
        int resource_fd = open(resource_file_path.c_str(), O_RDONLY | O_SYNC);
        if (resource_fd < 0) {
                printf("Error opening resource file <%s>: <%d> <%s>\n",
                       resource_file_path.c_str(), errno, strerror(errno));
                return errno;
        }
        std::ifstream resource_file(resource_file_path);
        if (!resource_file.is_open()) {
                printf("Error streaming resource file <%s>: <%d> <%s>\n",
                       resource_file_path.c_str(), errno, strerror(errno));
                close(resource_fd);
                return -EINVAL;
        }

        // Fill `temp_bar_descs` with the 6 BARs info, then copy it to
        // `pDevice`.
        for (int ibar = 0; ibar < 6; ibar++) {
                // Read a line in resource file related to `ibar`, even if
                // it's not supported, this is in order to sync the file cursor
                // with the current ibar Command: `cat
                // /sys/bus/pci/devices/0000:03:00.0/resource`
                unsigned long long start_addr, end_addr, flags;
                resource_file >> std::hex >> start_addr >> end_addr >> flags;

                // Construct the BAR resource file path and check it
                if (start_addr == 0 && end_addr == 0) {
                        // BAR resource doesn't exist,
                        CRONO_DEBUG("BAR not supported: Index: <%d>, Resource "
                                    "File: <%s>, \n",
                                    ibar, resource_file_path.c_str());
                        continue; // Check next BAR if valid
                }

                // A new BAR is found, fill a new element for it in
                // `temp_bar_descs`
                temp_bar_descs[bar_count].barNum = ibar + 1;
                CRONO_DEBUG("Found BAR No. %d, %s, \n",
                            temp_bar_descs[bar_count].barNum,
                            resource_file_path.c_str());

                // Set `length` (memory size)
                // Another way: `st.st_size` of `resourceN`
                temp_bar_descs[bar_count].length = end_addr - start_addr + 1;
                temp_bar_descs[bar_count].physicalAddress = start_addr;

                // Open BAR resource file to map the memory
                std::string bar_resource_file_path =
                    std::string(sys_dev_dir_path) + "/resource" +
                    std::to_string(ibar);
                CRONO_DEBUG("Getting bar descriptions for resource file <%s>\n",
                            bar_resource_file_path.c_str());
                int bar_resource_fd =
                    open(bar_resource_file_path.c_str(), O_RDWR | O_SYNC);
                if (bar_resource_fd < 0) {
                        printf("Error opening resource file <%s>: <%d> <%s>\n",
                               bar_resource_file_path.c_str(), errno,
                               strerror(errno));

                        // Clean up and return error
                        resource_file.close();
                        close(resource_fd);
                        return errno;
                }

                // mmap to get user address
                void *user_addr = mmap(NULL, temp_bar_descs[bar_count].length,
                                       PROT_READ | PROT_WRITE, MAP_SHARED,
                                       bar_resource_fd, 0);
                if (user_addr == MAP_FAILED) {
                        printf("Failed to map BAR memory <%s> to user space: "
                               "<%d> <%s>\n",
                               bar_resource_file_path.c_str(), errno,
                               strerror(errno));

                        // Clean up and return error
                        close(bar_resource_fd);
                        resource_file.close();
                        close(resource_fd);
                        return errno;
                }
                temp_bar_descs[bar_count].userAddress = (uint64_t)user_addr;
                close(bar_resource_fd);

                // Increment valid BARs count in array
                bar_count++;
        }

        // _____________________________
        // Success, cleanup and finalize
        //
        resource_file.close();
        close(resource_fd);

        // Copy full memory (6 elements) including the empty ones for non-found
        // BARs, so `pDevice->bar_descs` is "fully" set.
        memcpy(pDevice->bar_descs, temp_bar_descs,
               sizeof(CRONO_KERNEL_BAR_DESC) * 6);
        pDevice->bar_count = bar_count;
        return CRONO_SUCCESS;
}

uint32_t freeDeviceMem(PCRONO_KERNEL_DEVICE pDevice) {
        int iDev;
        for (iDev = 0; iDev < iNewDev; iDev++) {
                if (pDevice != devices[iDev])
                        continue;
                for (uint32_t ibar = 0; ibar < pDevice->bar_count; ibar++) {
                        if (!pDevice->bar_descs[ibar].userAddress)
                                continue;
                        if (munmap((void *)pDevice->bar_descs[ibar].userAddress,
                                   pDevice->bar_descs[ibar].length) == -1) {
                                int err = errno;
                                printf("Crono Error: munmap for ibar <%d>. "
                                       "<%d> <%s>\n",
                                       ibar, errno, strerror(errno));
                                return err;
                        }
                        pDevice->bar_descs[ibar].userAddress = 0;
                }
                pDevice->bar_count = 0;
                free(devices[iDev]);
                devices[iDev] = nullptr; // avoid double free
        }
        // Shrink array for null elements at the end if found
        while (iNewDev > 0 && devices[iNewDev] == nullptr) {
                // Last element in the array is empty, shrink it
                iNewDev--;
        }
        return CRONO_SUCCESS;
}

void freeDevicesMem() {
        for (int iDev = 0; iDev < iNewDev; iDev++) {
                if (devices[iDev]) {
                        // $$ unmmap device mem
                        free(devices[iDev]);
                        devices[iDev] = nullptr; // reset
                }
        }
        iNewDev = 0;
}

extern "C" __attribute__((destructor)) void onUnload() { freeDevicesMem(); }
