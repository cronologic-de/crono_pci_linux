#ifndef __CRONO_KERNEL_INTERFACE_H__
#define __CRONO_KERNEL_INTERFACE_H__

#ifndef __linux__
#pragma message("Source code is compliant with Linux only")
#endif

#if defined(__cplusplus)
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include "crono_driver.h"
#define PCI_ANY_ID (~0)
#define CRONO_SUCCESS 0
#define CRONO_VENDOR_ID 0x1A13
#ifndef FALSE
#define FALSE (0)
#endif
#ifndef TRUE
#define TRUE (!FALSE)
#endif

typedef uint64_t pciaddr_t;
typedef size_t UPTR;
typedef KPTR PHYS_ADDR;

enum { CRONO_KERNEL_PCI_CARDS = 100 }; // Slots max X Functions max

typedef struct {
        // Parameters used for string transfers:
        uint32_t dwBytes;   // For string transfer.
        uint32_t fAutoinc;  // Transfer from one port/address
                            // or use incremental range of addresses.
        uint32_t dwOptions; // Must be 0.
        union {
                unsigned char Byte;  // Use for 8 bit transfer.
                unsigned short Word; // Use for 16 bit transfer.
                uint32_t Dword;      // Use for 32 bit transfer.
                uint64_t Qword;      // Use for 64 bit transfer.
                void *pBuffer;       // Use for string transfer.
        } Data;
} CRONO_KERNEL_TRANSFER;

typedef struct {
        uint32_t dwDomain;
        uint32_t dwBus;
        uint32_t dwSlot;
        uint32_t dwFunction;
} CRONO_KERNEL_PCI_SLOT;

typedef struct {
        uint32_t dwVendorId;
        uint32_t dwDeviceId;
} CRONO_KERNEL_PCI_ID;

typedef struct {
        CRONO_KERNEL_PCI_SLOT pciSlot; /* PCI slot information */
} CRONO_KERNEL_PCI_CARD_INFO;

/* Handle to device information struct */
typedef void *CRONO_KERNEL_DEVICE_HANDLE;

/* PCI/PCMCIA slot */
typedef union {
} CRONO_KERNEL_SLOT_U;

/* PCI scan results */
typedef struct {
        uint32_t dwNumDevices; /* Number of matching devices */
        CRONO_KERNEL_PCI_ID
        deviceId[CRONO_KERNEL_PCI_CARDS]; /* Array of matching device IDs */
        CRONO_KERNEL_PCI_SLOT
        deviceSlot[CRONO_KERNEL_PCI_CARDS]; /* Array of matching device
                                             * locations
                                             */
} CRONO_KERNEL_PCI_SCAN_RESULT;

typedef uint32_t CRONO_KERNEL_DRV_OPEN_OPTIONS;

/**************************************************************
  Function Prototypes
 **************************************************************/
/* -----------------------------------------------
    General
   ----------------------------------------------- */
/* Get a device's user context */
void *CRONO_KERNEL_GetDevContext(CRONO_KERNEL_DEVICE_HANDLE hDev);

/* -----------------------------------------------
    Open/close driver and init/uninit CRONO_KERNEL library
   ----------------------------------------------- */
uint32_t CRONO_KERNEL_DriverOpen(CRONO_KERNEL_DRV_OPEN_OPTIONS openOptions);
uint32_t CRONO_KERNEL_DriverClose(void);

/* -----------------------------------------------
    Scan bus (PCI/PCMCIA)
   ----------------------------------------------- */
uint32_t
CRONO_KERNEL_PciScanDevices(uint32_t dwVendorId, uint32_t dwDeviceId,
                            CRONO_KERNEL_PCI_SCAN_RESULT *pPciScanResult);

/* -----------------------------------------------
    Open/Close device
   ----------------------------------------------- */
uint32_t
CRONO_KERNEL_PciDeviceOpen(CRONO_KERNEL_DEVICE_HANDLE *phDev,
                           const CRONO_KERNEL_PCI_CARD_INFO *pDeviceInfo,
                           const void *pDevCtx, void *reserved,
                           const char *pcKPDriverName, void *pKPOpenData);

uint32_t CRONO_KERNEL_PciDeviceClose(CRONO_KERNEL_DEVICE_HANDLE hDev);

/* -----------------------------------------------
    Set card cleanup commands
   ----------------------------------------------- */
uint32_t CRONO_KERNEL_CardCleanupSetup( //$$ finalize
    CRONO_KERNEL_DEVICE_HANDLE hDev, CRONO_KERNEL_TRANSFER *Cmd,
    uint32_t dwCmds, int bForceCleanup);

/* -----------------------------------------------
    Read/Write memory and I/O addresses
   ----------------------------------------------- */

/* Read/write a device's address space (8/16/32/64 bits) */
uint32_t CRONO_KERNEL_ReadAddr8(CRONO_KERNEL_DEVICE_HANDLE hDev,
                                uint32_t dwAddrSpace, KPTR dwOffset,
                                unsigned char *val);
uint32_t CRONO_KERNEL_ReadAddr16(CRONO_KERNEL_DEVICE_HANDLE hDev,
                                 uint32_t dwAddrSpace, KPTR dwOffset,
                                 unsigned short *val);
uint32_t CRONO_KERNEL_ReadAddr32(CRONO_KERNEL_DEVICE_HANDLE hDev,
                                 uint32_t dwAddrSpace, KPTR dwOffset,
                                 uint32_t *val);
uint32_t CRONO_KERNEL_ReadAddr64(CRONO_KERNEL_DEVICE_HANDLE hDev,
                                 uint32_t dwAddrSpace, KPTR dwOffset,
                                 uint64_t *val);

uint32_t CRONO_KERNEL_WriteAddr8(CRONO_KERNEL_DEVICE_HANDLE hDev,
                                 uint32_t dwAddrSpace, KPTR dwOffset,
                                 unsigned char val);
uint32_t CRONO_KERNEL_WriteAddr16(CRONO_KERNEL_DEVICE_HANDLE hDev,
                                  uint32_t dwAddrSpace, KPTR dwOffset,
                                  unsigned short val);
uint32_t CRONO_KERNEL_WriteAddr32(CRONO_KERNEL_DEVICE_HANDLE hDev,
                                  uint32_t dwAddrSpace, KPTR dwOffset,
                                  uint32_t val);
uint32_t CRONO_KERNEL_WriteAddr64(CRONO_KERNEL_DEVICE_HANDLE hDev,
                                  uint32_t dwAddrSpace, KPTR dwOffset,
                                  uint64_t val);

/* Is address space active */
int CRONO_KERNEL_AddrSpaceIsActive(CRONO_KERNEL_DEVICE_HANDLE hDev,
                                   uint32_t dwAddrSpace);

/* -----------------------------------------------
    Access PCI configuration space
   ----------------------------------------------- */
/* Read/write 8/16/32/64 bits from the PCI configuration space.
   Identify device by handle */
uint32_t CRONO_KERNEL_PciReadCfg32(CRONO_KERNEL_DEVICE_HANDLE hDev,
                                   uint32_t dwOffset, uint32_t *val);

uint32_t CRONO_KERNEL_PciWriteCfg32(CRONO_KERNEL_DEVICE_HANDLE hDev,
                                    uint32_t dwOffset, uint32_t val);

/* -----------------------------------------------
    DMA (Direct Memory Access)
   ----------------------------------------------- */
/* Allocate and lock a contiguous DMA buffer */
uint32_t CRONO_KERNEL_DMAContigBufLock(CRONO_KERNEL_DEVICE_HANDLE hDev,
                                       void **ppBuf, uint32_t dwOptions,
                                       uint32_t dwDMABufSize,
                                       CRONO_KERNEL_DMA **ppDma);

/**
 * Lock a Scatter/Gather DMA buffer.
 * This function calls the user mode process to  create a mapping of virtual
 * space to a list of chunked physical addresses (mdl). The kernel mode process
 * also locks the mapping.
 *
 * @return DMASGBufLock_parameters, that are now completely filled.
 * The relevant part (MDL etc.) is stored in the CRONO_KERNEL_DMA struct.
 * Any error that appear will be stored in the item value.
 */
uint32_t CRONO_KERNEL_DMASGBufLock(CRONO_KERNEL_DEVICE_HANDLE hDev, void *pBuf,
                                   uint32_t dwOptions, uint32_t dwDMABufSize,
                                   CRONO_KERNEL_DMA **ppDma);

/* Unlock a DMA buffer */
uint32_t CRONO_KERNEL_DMABufUnlock(CRONO_KERNEL_DEVICE_HANDLE hDev,
                                   CRONO_KERNEL_DMA *pDma);

/* Address space information struct */
typedef struct {
        UPTR
            pUserDirectMemAddr; /* Memory address for direct user-mode access */
        size_t dwSize;          // Added for Linux, to be used with munmap
} CRONO_KERNEL_ADDR_DESC;

/* Device information struct */
typedef struct CRONO_KERNEL_DEVICE {
        uint32_t dwVendorId;
        uint32_t dwDeviceId;
        CRONO_KERNEL_PCI_SLOT pciSlot;     /* PCI/PCMCIA device slot location
                                            * information */
        CRONO_KERNEL_ADDR_DESC *pAddrDesc; /* Array of device's address spaces
                                            * information */
        void *pCtx;                        /* User-specific context */
        CRONO_KERNEL_ADDR_DESC bar_addr; // BAR0 userspace mapped address

        /**
         * The name of the corresponding `miscdev` file, found under /dev
         */
        char miscdev_name[CRONO_MAX_DEV_NAME_SIZE];

} CRONO_KERNEL_DEVICE, *PCRONO_KERNEL_DEVICE;

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

#define sprintf_s snprintf
void printFreeMemInfoDebug(const char *msg);
#ifdef CRONO_DEBUG_ENABLED
#define CRONO_DEBUG(...) fprintf(stdout, __VA_ARGS__);
#define CRONO_DEBUG_MEM_MSG(fmt, ...)                                          \
        fprintf(stdout, "crono Memory Debug Info: " fmt, ...)
#else // #ifdef CRONO_DEBUG_ENABLED
#define CRONO_DEBUG(...)
#define CRONO_DEBUG_MEM_MSG(fmt, ...)
#endif // #ifdef CRONO_DEBUG_ENABLED

#ifdef __cplusplus
}
#endif

#endif /* __CRONO_KERNEL_INTERFACE_H__ */
