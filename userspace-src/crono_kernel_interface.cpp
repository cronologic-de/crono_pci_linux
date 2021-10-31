#include "crono_kernel_interface.h"
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
                           const CRONO_KERNEL_PCI_CARD_INFO *pDeviceInfo,
                           const void *pDevCtx, void *reserved,
                           const char *pcKPDriverName, void *pKPOpenData) {
        DIR *dr = NULL;
        struct dirent *en;
        unsigned domain, bus, dev, func;
        int err;

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
                        err = crono_read_vendor_device(domain, bus, dev, func,
                                                       &vendor_id, &device_id);
                        if (CRONO_SUCCESS != err) {
                                goto device_error;
                        }
                        pDevice->dwDeviceId = device_id;
                        pDevice->dwVendorId = vendor_id;

                        // Set bar_addr
                        // Map BAR0 full memory starting @ offset 0 to bar_addr
                        void *BAR_base_mem_address;
                        pciaddr_t dwSize = 0;
                        err = crono_get_BAR0_mem_addr(
                            domain, bus, dev, func, 0, &dwSize,
                            &BAR_base_mem_address, NULL);
                        if (CRONO_SUCCESS != err) {
                                goto device_error;
                        }
                        pDevice->bar_addr.pUserDirectMemAddr =
                            (UPTR)BAR_base_mem_address;
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
                                err = -EINVAL;
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
        return err;
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
                                       CRONO_KERNEL_TRANSFER *Cmd,
                                       uint32_t dwCmds, int bForceCleanup) {
        int ret = CRONO_SUCCESS;
        char miscdev_path[PATH_MAX];
        int miscdev_fd;

        // Init variables and validate parameters
        CRONO_INIT_HDEV_FUNC(hDev);
        miscdev_fd = 0;

        // Open device file
        // Get device file name & open it
        sprintf(miscdev_path, "/dev/%s", pDevice->miscdev_name);
        miscdev_fd = open(miscdev_path, O_RDWR);
        if (miscdev_fd < 0) {
                printf("Error %d: cannot open device file <%s>...\n", errno,
                       miscdev_path);
                return errno;
        }

        if (CRONO_SUCCESS !=
            ioctl(miscdev_fd, IOCTL_CRONO_UNLOCK_BUFFER, NULL)) {
                return -EPERM;
        }

        return CRONO_SUCCESS;
}

uint32_t CRONO_KERNEL_PciReadCfg32(CRONO_KERNEL_DEVICE_HANDLE hDev,
                                   uint32_t dwOffset, uint32_t *val) {
        pciaddr_t bytes_read;
        int err = CRONO_SUCCESS;
        pciaddr_t config_space_size = 0;

        // Init variables and validate parameters
        CRONO_INIT_HDEV_FUNC(hDev);
        CRONO_RET_INV_PARAM_IF_NULL(val);

        // Validate offset and data size are within configuration spcae size
        err = crono_get_config_space_size(
            pDevice->pciSlot.dwDomain, pDevice->pciSlot.dwBus,
            pDevice->pciSlot.dwSlot, pDevice->pciSlot.dwFunction,
            &config_space_size);
        if (CRONO_SUCCESS != err) {
                return err;
        }
        if (config_space_size < (dwOffset + sizeof(*val))) {
                return -EINVAL;
        }

        // Read the configurtion
        err = crono_read_config(pDevice->pciSlot.dwDomain,
                                pDevice->pciSlot.dwBus, pDevice->pciSlot.dwSlot,
                                pDevice->pciSlot.dwFunction, val, dwOffset,
                                sizeof(*val), &bytes_read);
        if ((bytes_read != 4) || err) {
                return err;
        }

        // Success
        return CRONO_SUCCESS;
}

uint32_t CRONO_KERNEL_PciWriteCfg32(CRONO_KERNEL_DEVICE_HANDLE hDev,
                                    uint32_t dwOffset, uint32_t val) {
        pciaddr_t bytes_written;
        int err = CRONO_SUCCESS;
        pciaddr_t config_space_size = 0;

        // Init variables and validate parameters
        CRONO_INIT_HDEV_FUNC(hDev);

        // Validate offset and data size are within configuration spcae size
        err = crono_get_config_space_size(
            pDevice->pciSlot.dwDomain, pDevice->pciSlot.dwBus,
            pDevice->pciSlot.dwSlot, pDevice->pciSlot.dwFunction,
            &config_space_size);
        if (CRONO_SUCCESS != err) {
                return err;
        }
        if (config_space_size < (dwOffset + sizeof(val))) {
                return -EINVAL;
        }

        // Write the configuration
        err = crono_write_config(
            pDevice->pciSlot.dwDomain, pDevice->pciSlot.dwBus,
            pDevice->pciSlot.dwSlot, pDevice->pciSlot.dwFunction, &val,
            dwOffset, sizeof(val), &bytes_written);
        if ((bytes_written != 4) || err) {
                return err;
        }

        // Success
        return CRONO_SUCCESS;
}

uint32_t CRONO_KERNEL_ReadAddr8(CRONO_KERNEL_DEVICE_HANDLE hDev,
                                uint32_t dwAddrSpace, KPTR dwOffset,
                                unsigned char *val) {
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
                                 uint32_t dwAddrSpace, KPTR dwOffset,
                                 unsigned short *val) {
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
                                 uint32_t dwAddrSpace, KPTR dwOffset,
                                 uint32_t *val) {
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
                                 uint32_t dwAddrSpace, KPTR dwOffset,
                                 uint64_t *val) {
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
                                 uint32_t dwAddrSpace, KPTR dwOffset,
                                 unsigned char val) {
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
                                  uint32_t dwAddrSpace, KPTR dwOffset,
                                  unsigned short val) {
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
                                  uint32_t dwAddrSpace, KPTR dwOffset,
                                  uint32_t val) {
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
                                  uint32_t dwAddrSpace, KPTR dwOffset,
                                  uint64_t val) {
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

uint32_t CRONO_KERNEL_DriverOpen(CRONO_KERNEL_DRV_OPEN_OPTIONS openOptions) {
        return CRONO_SUCCESS;
}

uint32_t CRONO_KERNEL_DriverClose() { return CRONO_SUCCESS; }

void *CRONO_KERNEL_GetDevContext(CRONO_KERNEL_DEVICE_HANDLE hDev) {
        // Init variables and validate parameters
        PCRONO_KERNEL_DEVICE pDevice;
        if (NULL == hDev)
                return NULL;
        pDevice = (PCRONO_KERNEL_DEVICE)hDev;
        return pDevice->pCtx;
}

int CRONO_KERNEL_AddrSpaceIsActive(CRONO_KERNEL_DEVICE_HANDLE hDev,
                                   uint32_t dwAddrSpace) {

        CRONO_INIT_HDEV_FUNC(hDev);
        return CRONO_SUCCESS;
}

uint32_t CRONO_KERNEL_DMASGBufLock(CRONO_KERNEL_DEVICE_HANDLE hDev, void *pBuf,
                                   uint32_t dwOptions, uint32_t dwDMABufSize,
                                   CRONO_KERNEL_DMA **ppDma) {

        int ret = CRONO_SUCCESS;
        DMASGBufLock_parameters params;
        int miscdev_fd = 0;
        char miscdev_path[PATH_MAX];
        CRONO_KERNEL_DMA *pDma;

        // ______________________________________
        // Init variables and validate parameters
        //
        CRONO_DEBUG("Locking Buffer of address = <%p>, size = <%u>\n", pBuf,
                    dwDMABufSize);
        CRONO_INIT_HDEV_FUNC(hDev);
        CRONO_RET_INV_PARAM_IF_NULL(pBuf);
        CRONO_RET_INV_PARAM_IF_NULL(ppDma);
        CRONO_RET_INV_PARAM_IF_ZERO(dwDMABufSize);

        // Construct DMASGBufLock_parameters
        params.pBuf = pBuf;
        params.dwDMABufSize = dwDMABufSize;
        params.dwOptions = dwOptions;

        // Allocate the DMA Pages Memory
        pDma = (CRONO_KERNEL_DMA *)malloc(sizeof(CRONO_KERNEL_DMA));
        if (NULL == pDma) {
                printf("Error allocating DMA struct memory");
                return -ENOMEM;
        }
        memset(pDma, 0, sizeof(CRONO_KERNEL_DMA));
#define DIV_ROUND_UP(n, d) (((n) + (d)-1) / (d))
        pDma->dwPages = DIV_ROUND_UP(dwDMABufSize, PAGE_SIZE);
        CRONO_DEBUG("Allocating memory of size = <%lu>, of pages count <%d>\n",
                    sizeof(CRONO_KERNEL_DMA_PAGE) * pDma->dwPages,
                    pDma->dwPages);
        pDma->Page = (CRONO_KERNEL_DMA_PAGE *)malloc(
            sizeof(CRONO_KERNEL_DMA_PAGE) * pDma->dwPages);
        if (NULL == pDma->Page) {
                printf("Error allocating Page memory");
                free(pDma);
                return -ENOMEM;
        }
        params.ulPage = (unsigned long)pDma->Page;
        CRONO_DEBUG("Allocated `ppDma[0]->Page` of size <%ld>\n",
                    sizeof(CRONO_KERNEL_DMA_PAGE) * pDma->dwPages);
        *ppDma = pDma; // Do not ppDma = &pDma
        params.ppDma = ppDma;
        params.ulpDma = (unsigned long)pDma;
        CRONO_DEBUG("Passing ulpDma = 0x%lx\n", params.ulpDma);

        // ________________
        // Open device file
        //
        // Get device file name & open it
        sprintf(miscdev_path, "/dev/%s", pDevice->miscdev_name);
        miscdev_fd = open(miscdev_path, O_RDWR);
        if (miscdev_fd < 0) {
                printf("Error %d: cannot open device file <%s>...\n", errno,
                       miscdev_path);
                free(pDma->Page);
                free(pDma);
                return errno;
        }

        // ___________
        // Lock Buffer
        //
        ret = ioctl(miscdev_fd, IOCTL_CRONO_LOCK_BUFFER, &params);
        if (CRONO_SUCCESS != ret) {
                printf("Driver module error %d\n", ret);
                goto alloc_err;
        }

#ifdef CRONO_DEBUG_ENABLED
        for (unsigned int ipage = 0;
             ipage < (pDma->dwPages < 5 ? pDma->dwPages : 5); ipage++) {
                fprintf(stdout, "Buffer Page <%d> Physical Address is <%p>\n",
                        ipage, (void *)(pDma->Page[ipage].pPhysicalAddr));
        }
#endif

        // _______
        // Cleanup
        //
        // Close device file
        close(miscdev_fd);

        return ret;

alloc_err:
        if (miscdev_fd > 0)
                close(miscdev_fd);

        return ret;
}

uint32_t CRONO_KERNEL_DMABufUnlock(CRONO_KERNEL_DEVICE_HANDLE hDev,
                                   CRONO_KERNEL_DMA *pDma) {
        int ret = CRONO_SUCCESS;
        char miscdev_path[PATH_MAX];
        DMASGBufLock_parameters params;
        int miscdev_fd;

        // ______________________________________
        // Init variables and validate parameters
        //
        CRONO_INIT_HDEV_FUNC(hDev);
        CRONO_RET_INV_PARAM_IF_NULL(pDma);
        params.ppDma = NULL;
        miscdev_fd = 0;
        params.ppDma = (CRONO_KERNEL_DMA **)malloc(sizeof(CRONO_KERNEL_DMA **));
        params.ppDma[0] = pDma;
        params.ulpDma = (unsigned long)pDma;

        // ________________
        // Open device file
        //
        // Get device file name & open it
        sprintf(miscdev_path, "/dev/%s", pDevice->miscdev_name);
        miscdev_fd = open(miscdev_path, O_RDWR);
        if (miscdev_fd < 0) {
                printf("Error %d: cannot open device file <%s>...\n", errno,
                       miscdev_path);
                return errno;
        }

        // _____________
        // Unlock Buffer
        //
        // Call ioctl() to unlock the buffer and cleanup
        if (CRONO_SUCCESS !=
            ioctl(miscdev_fd, IOCTL_CRONO_UNLOCK_BUFFER, &params)) {
                goto ioctl_err;
        }

        // _______
        // Cleanup
        //
        // Free memory
        if (NULL != params.ppDma[0]->Page) {
                free(params.ppDma[0]->Page);
                params.ppDma[0]->Page = NULL;
        }
        if (NULL != params.ppDma) {
                free(params.ppDma);
        }

        // Close device file
        close(miscdev_fd);

        return ret;

ioctl_err:
        if (NULL != params.ppDma)
                free(params.ppDma);
        if (miscdev_fd > 0)
                close(miscdev_fd);

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