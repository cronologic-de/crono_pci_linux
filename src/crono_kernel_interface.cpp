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
variables
 * defined and initialized in the functions.
d*/
#define CRONO_VALIDATE_MEM_RANGE                                               \
        if ((dwOffset + sizeof(val)) > pDevice->bar_addr.dwSize) {             \
                return -ENOMEM;                                                \
        }

uint32_t
CRONO_KERNEL_PciScanDevices(uint32_t dwVendorId, uint32_t dwDeviceId,
                            CRONO_KERNEL_PCI_SCAN_RESULT *pPciScanResult) {
        struct stat st;
        DIR *dr = NULL;
        struct dirent *en;
        unsigned domain, bus, dev, func;
        uint16_t vendor_id, device_id;
        int index_in_result = 0;

        if (stat(SYS_BUS_PCIDEVS_PATH, &st) != 0) {
                printf("Error %d: PCI FS is not found.\n", errno);
                return errno;
        }
        if (((st.st_mode) & S_IFMT) != S_IFDIR) {
                printf("Error: PCI FS is not a directory.\n");
                return errno;
        }
        dr = opendir(SYS_BUS_PCIDEVS_PATH); // open all or present directory
        if (!dr) {
                printf("Error %d: opening PCI directory.\n", errno);
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
                crono_read_vendor_device(domain, bus, dev, func,
                                         (unsigned int *)&vendor_id,
                                         (unsigned int *)&device_id);

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

        // Init variables and validate parameters
        CRONO_RET_INV_PARAM_IF_NULL(phDev);
        CRONO_RET_INV_PARAM_IF_NULL(pDeviceInfo);
        *phDev = NULL;

        dr = opendir(SYS_BUS_PCIDEVS_PATH); // open all or present directory
        if (!dr) {
                printf("Error %d: opening PCI directory.\n", errno);
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
                        PCRONO_KERNEL_DEVICE pDevice;
                        pDevice = (PCRONO_KERNEL_DEVICE)malloc(
                            sizeof(CRONO_KERNEL_DEVICE));
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
                        unsigned int vendor_id, device_id;
                        ret = crono_read_vendor_device(domain, bus, dev, func,
                                                       &vendor_id, &device_id);
                        if (CRONO_SUCCESS != ret) {
                                goto device_error;
                        }
                        pDevice->dwDeviceId = device_id;
                        pDevice->dwVendorId = vendor_id;

                        // Set bar_addr
                        // Map BAR0 full memory starting @ offset 0 to bar_addr
                        void *BAR_base_mem_address;
                        pciaddr_t dwSize = 0;
                        ret = crono_get_BAR0_mem_addr(
                            domain, bus, dev, func, 0, &dwSize,
                            &BAR_base_mem_address, NULL);
                        if (CRONO_SUCCESS != ret) {
                                goto device_error;
                        }
                        pDevice->bar_addr.pUserDirectMemAddr =
                            (size_t)BAR_base_mem_address;
                        pDevice->bar_addr.dwSize = dwSize;

                        // Get the device `miscdev` file name, and set it to
                        // `pDevice`
                        struct crono_dev_DBDF dbdf = {domain, bus, dev, func};
                        CRONO_CONSTRUCT_MISCDEV_NAME(pDevice->miscdev_name,
                                                     device_id, dbdf);
                        struct stat miscdev_stat;
                        char miscdev_path[PATH_MAX];
                        sprintf(miscdev_path, "/dev/%s", pDevice->miscdev_name);
                        if (stat(miscdev_path, &miscdev_stat) != 0) {
                                printf("Error: miscdev `%s` is not found.\n",
                                       miscdev_path);
                                ret = -EINVAL;
                                goto device_error;
                        }

                        // Open the miscellanous driver file
                        int miscdev_fd = open(miscdev_path, O_RDWR);
                        switch (errno) {
                        case 0:
                                // Success
                                pDevice->miscdev_fd = miscdev_fd;
                                CRONO_DEBUG("Device <%s> is opened as <%d>.\n", 
                                    pDevice->miscdev_name, pDevice->miscdev_fd);
                                break;
                        case EBUSY:
                                printf("Device is busy, miscdev_fd is left as is <%d>\n", 
                                    pDevice->miscdev_fd);
                                break;
                        case ENODEV:
                                printf("No device found\n");
                                ret = CRONO_KERNEL_NO_DEVICE_OBJECT;
                                goto device_error;
                        default:
                                printf("Error %d: cannot open device file "
                                       "<%s>...\n",
                                       errno, miscdev_path);
                                ret = CRONO_KERNEL_INSUFFICIENT_RESOURCES;
                                goto device_error;
                        }

                        // Set phDev
                        *phDev = pDevice;
                        break;
                }
        }

        // Clean up
        closedir(dr);

        // Successfully scanned
        return CRONO_SUCCESS;

device_error:
        if (dr) {
                closedir(dr);
        }
        return ret;
}

uint32_t CRONO_KERNEL_PciDeviceClose(CRONO_KERNEL_DEVICE_HANDLE hDev) {
        // Init variables and validate parameters
        CRONO_INIT_HDEV_FUNC(hDev);

        // Free memory allocated in CRONO_KERNEL_PciDeviceOpen
        free(pDevice);

        return CRONO_SUCCESS;
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
        *val = *(
            (volatile unsigned char
                 *)(((unsigned char *)(pDevice->bar_addr.pUserDirectMemAddr)) +
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
        *val = *(
            (volatile unsigned short
                 *)(((unsigned char *)(pDevice->bar_addr.pUserDirectMemAddr)) +
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

        *val = *(
            (volatile uint32_t *)(((unsigned char *)(pDevice->bar_addr
                                                         .pUserDirectMemAddr)) +
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
        *val = *(
            (volatile uint64_t *)(((unsigned char *)(pDevice->bar_addr
                                                         .pUserDirectMemAddr)) +
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
        *((volatile unsigned char
               *)(((unsigned char *)(pDevice->bar_addr.pUserDirectMemAddr)) +
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
        *((volatile unsigned short
               *)(((unsigned char *)(pDevice->bar_addr.pUserDirectMemAddr)) +
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
        *((volatile uint32_t *)(((unsigned char *)(pDevice->bar_addr
                                                       .pUserDirectMemAddr)) +
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
        *((volatile uint64_t *)(((unsigned char *)(pDevice->bar_addr
                                                       .pUserDirectMemAddr)) +
                                dwOffset)) = val;

        // Success
        return CRONO_SUCCESS;
}

uint32_t CRONO_KERNEL_GetBarPointer(CRONO_KERNEL_DEVICE_HANDLE hDev,
                                    uint32_t *barPointer) {
        // Init variables and validate parameters
        CRONO_INIT_HDEV_FUNC(hDev);
        CRONO_RET_INV_PARAM_IF_NULL(barPointer);

        barPointer = (uint32_t *)(pDevice->bar_addr.pUserDirectMemAddr);

        // Success
        return CRONO_SUCCESS;
}

uint32_t CRONO_KERNEL_DMASGBufLock(CRONO_KERNEL_DEVICE_HANDLE hDev, void *pBuf,
                                   uint32_t dwOptions, uint32_t dwDMABufSize,
                                   CRONO_KERNEL_DMA_SG **ppDma) {

        int ret = CRONO_SUCCESS;
        CRONO_BUFFER_INFO buff_info;
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
        CRONO_DEBUG("Done locking buffer id <%d>.\n", pDma->id);
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

        // _______
        // Cleanup
        //
        // Free memory
        if (NULL != pDma->Page) {
                free(pDma->Page);
                pDma->Page = NULL;
        }

        CRONO_DEBUG("Done unlocking buffer id <%d>.\n", pDma->id);
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
        *pBARUSAddr = pDevice->bar_addr.pUserDirectMemAddr;
        *pBARMemSize = pDevice->bar_addr.dwSize;

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
uint32_t CRONO_KERNEL_PciWriteCfg32Arr(CRONO_KERNEL_DEVICE_HANDLE hDev, uint32_t dwOffset, uint32_t* val, uint32_t arr_size)
{
	uint32_t code = CRONO_KERNEL_STATUS_SUCCESS;
        // Not implemented
	return code;
}

