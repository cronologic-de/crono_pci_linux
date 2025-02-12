/*++

Module Name:

    crono_kernel_interface.h

Abstract:

    This module contains the common declarations shared by driver
    and user applications.

Environment:

    user and kernel

--*/

#ifndef __CRONO_KERNEL_INTERFACE_H__
#define __CRONO_KERNEL_INTERFACE_H__ (1)

#if defined(__cplusplus)
extern "C" {
#endif

#if defined(_WIN32) || defined(_WIN64)
#ifdef CRONO_KERNEL_DRIVER_EXPORTS
#define CRONO_KERNEL_API __declspec(dllexport)
#else
#ifndef CRONO_KERNEL_DRIVER_STATIC
#define CRONO_KERNEL_API __declspec(dllimport)
#else
#define CRONO_KERNEL_API
#endif
#endif //  CRONO_KERNEL_DRIVER_EXPORTS
#else
#define CRONO_KERNEL_API
#endif // #if defined(_WIN32) || defined(_WIN64)

#include <stdint.h>

#ifdef __linux__
#include <stddef.h>
#include <string.h>
typedef uint64_t pciaddr_t;
#define PCI_ANY_ID (~0)
#define CRONO_SUCCESS 0
#define CRONO_VENDOR_ID 0x1A13
#ifndef FALSE
#define FALSE (0)
#endif // #ifndef FALSE
#ifndef TRUE
#define TRUE (!FALSE)
#endif // #ifndef TRUE
#define MAX_NAME 128
#define MAX_DESC 128
#endif // #ifdef __linux__

typedef uint64_t DMA_ADDR;

typedef struct {
        uint32_t version_major;
        uint32_t version_minor;
        uint32_t version_revision;
        uint32_t version_build;
} CRONO_KERNEL_VERSION;

typedef struct {
        DMA_ADDR pPhysicalAddr; // Physical address of page.
        uint32_t dwBytes;       // Size of page.
} CRONO_KERNEL_DMA_PAGE;

typedef struct {
        uint32_t addr; // 32 bti register address.
        uint32_t data; // Use for 32 bit transfer.
} CRONO_KERNEL_CMD;

typedef struct {
        //	uint32_t hDma;             // Handle of DMA buffer
        void *pUserAddr;  // Beginning of buffer.
        uint32_t dwPages; // Number of pages in buffer.
        CRONO_KERNEL_DMA_PAGE *Page;

        // Kernel Information
        int id; // Internal kernel ID of the buffer
} CRONO_KERNEL_DMA_SG;

typedef struct {
        uint32_t hDma;    // Handle of DMA buffer
        void *pUserAddr;  // Beginning of buffer.
        uint32_t dwBytes; // Size of buffer.
        DMA_ADDR pPhysicalAddr;

        // Kernel Information
        int id; // Internal kernel ID of the buffer
} CRONO_KERNEL_DMA_CONTIG;

// these are dwOptions
enum {
        DMA_KERNEL_BUFFER_ALLOC =
            0x1, // The system allocates a contiguous buffer.
                 // The user does not need to supply linear address.

        DMA_KBUF_BELOW_16M = 0x2, // If DMA_KERNEL_BUFFER_ALLOC is used,
                                  // this will make sure it is under 16M.

        DMA_LARGE_BUFFER =
            0x4, // If DMA_LARGE_BUFFER is used,
                 // the maximum number of pages are dwPages, and not
                 // WD_DMA_PAGES. If you lock a user buffer (not a
                 // kernel allocated buffer) that is larger than 1MB,
                 // then use this option and allocate memory for pages.

        DMA_ALLOW_CACHE = 0x8, // Allow caching of contiguous memory.

        DMA_KERNEL_ONLY_MAP =
            0x10, // Only map to kernel, dont map to user-mode.
                  // relevant with DMA_KERNEL_BUFFER_ALLOC flag only

        DMA_FROM_DEVICE =
            0x20, // memory pages are locked to be written by device

        DMA_TO_DEVICE = 0x40, // memory pages are locked to be read by device

        DMA_TO_FROM_DEVICE =
            (DMA_FROM_DEVICE | DMA_TO_DEVICE), // memory pages are
                                               // locked for both read and write

        DMA_ALLOW_64BIT_ADDRESS = 0x80, // Use this value for devices that
                                        // support 64-bit DMA addressing.

        DMA_ALLOW_NO_HCARD = 0x100, // allow memory lock without hCard

        DMA_PAGE_SIZE_2MB = 0x200, // expect huge pages as result
};

/* Macros for backward compatibility */
#define DMA_READ_FROM_DEVICE DMA_FROM_DEVICE
#define DMA_WRITE_TO_DEVICE DMA_TO_DEVICE

enum { CRONO_KERNEL_PCI_CARDS = 100 }; // Slots max X Functions max

//--------------------------------
// Structs used by kmdf framework
//--------------------------------
typedef struct {
#ifdef __linux__
        uint32_t dwDomain;
#endif
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
        // CRONO_KERNEL_CARD Card;           /* Card information */
} CRONO_KERNEL_PCI_CARD_INFO;

typedef enum { EVENT_STATUS_OK = 0 } EVENT_STATUS;

typedef enum {
        CRONO_KERNEL_STATUS_SUCCESS = 0x0L,
        CRONO_KERNEL_STATUS_INVALID_CRONO_KERNEL_HANDLE = 0xffffffffL,

        CRONO_KERNEL_CRONO_KERNEL_STATUS_ERROR = 0x20000000L,

        CRONO_KERNEL_INVALID_HANDLE = 0x20000001L,
        CRONO_KERNEL_READ_WRITE_CONFLICT =
            0x20000003L, /* Request to read from an OUT (write)
                          *  pipe or request to write to an IN
                          *  (read) pipe */
        CRONO_KERNEL_ZERO_PACKET_SIZE =
            0x20000004L, /* Maximum packet size is zero */
        CRONO_KERNEL_INSUFFICIENT_RESOURCES = 0x20000005L,
        CRONO_KERNEL_SYSTEM_INTERNAL_ERROR = 0x20000007L,
        CRONO_KERNEL_DATA_MISMATCH = 0x20000008L,
        CRONO_KERNEL_NOT_IMPLEMENTED = 0x2000000aL,
        CRONO_KERNEL_KERPLUG_FAILURE = 0x2000000bL,
        CRONO_KERNEL_RESOURCE_OVERLAP = 0x2000000eL,
        CRONO_KERNEL_DEVICE_NOT_FOUND = 0x2000000fL,
        CRONO_KERNEL_WRONG_UNIQUE_ID = 0x20000010L,
        CRONO_KERNEL_OPERATION_ALREADY_DONE = 0x20000011L,
        CRONO_KERNEL_SET_CONFIGURATION_FAILED = 0x20000013L,
        CRONO_KERNEL_CANT_OBTAIN_PDO = 0x20000014L,
        CRONO_KERNEL_TIME_OUT_EXPIRED = 0x20000015L,
        CRONO_KERNEL_IRP_CANCELED = 0x20000016L,
        CRONO_KERNEL_FAILED_USER_MAPPING = 0x20000017L,
        CRONO_KERNEL_FAILED_KERNEL_MAPPING = 0x20000018L,
        CRONO_KERNEL_NO_RESOURCES_ON_DEVICE = 0x20000019L,
        CRONO_KERNEL_NO_EVENTS = 0x2000001aL,
        CRONO_KERNEL_INVALID_PARAMETER = 0x2000001bL,
        CRONO_KERNEL_INCORRECT_VERSION = 0x2000001cL,
        CRONO_KERNEL_TRY_AGAIN = 0x2000001dL,
        CRONO_KERNEL_CRONO_KERNEL_NOT_FOUND = 0x2000001eL,
        CRONO_KERNEL_INVALID_IOCTL = 0x2000001fL,
        CRONO_KERNEL_OPERATION_FAILED = 0x20000020L,
        CRONO_KERNEL_TOO_MANY_HANDLES = 0x20000022L,
        CRONO_KERNEL_NO_DEVICE_OBJECT = 0x20000023L,
        CRONO_KERNEL_OS_PLATFORM_MISMATCH = 0x20000024L,

} CRONO_KERNEL_ERROR_CODES;

typedef enum {
        CRONO_KERNEL_ACKNOWLEDGE = 0x1,
        CRONO_KERNEL_ACCEPT_CONTROL = 0x2 // used in WD_EVENT_SEND (acknowledge)
} CRONO_KERNEL_EVENT_OPTION;

typedef struct {
        uint32_t handle;
        uint32_t dwAction; // WD_EVENT_ACTION
        uint32_t dwStatus; // EVENT_STATUS
        uint32_t dwEventId;
        uint32_t hKernelPlugIn;
        uint32_t dwOptions; // WD_EVENT_OPTION
        union {
                struct {
                        CRONO_KERNEL_PCI_ID cardId;
                        CRONO_KERNEL_PCI_SLOT pciSlot;
                } Pci;
        } u;
        uint32_t dwEventVer;
} CRONO_KERNEL_EVENT;

#ifndef BZERO
#define BZERO(buf) memset(&(buf), 0, sizeof(buf))
#endif

/* *end* extracted from windrvr.h */

CRONO_KERNEL_API const char *Stat2Str(uint32_t dwStatus);

/**************************************************************
  General definitions
 **************************************************************/

/* Handle to device information struct */
typedef void *CRONO_KERNEL_DEVICE_HANDLE;

/* PCI/PCMCIA slot */
typedef union {
        CRONO_KERNEL_PCI_SLOT pciSlot;
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

/**************************************************************
  Function Prototypes
 **************************************************************/

/* -----------------------------------------------
    Scan bus (PCI/PCMCIA)
   ----------------------------------------------- */
CRONO_KERNEL_API uint32_t
CRONO_KERNEL_PciScanDevices(uint32_t dwVendorId, uint32_t dwDeviceId,
                            CRONO_KERNEL_PCI_SCAN_RESULT *pPciScanResult);

/* -------------------------------------------------
    Get device's resources information (PCI/PCMCIA)
   ------------------------------------------------- */
CRONO_KERNEL_API uint32_t
CRONO_KERNEL_PciGetDeviceInfo(CRONO_KERNEL_PCI_CARD_INFO *pDeviceInfo);

/* -------------------------------------------------
Get device´s driver version information
------------------------------------------------- */
CRONO_KERNEL_API uint32_t CRONO_KERNEL_PciDriverVersion(
    CRONO_KERNEL_DEVICE_HANDLE hDev, CRONO_KERNEL_VERSION *pVersion);

/* -----------------------------------------------
    Open/Close device
   ----------------------------------------------- */
#if !defined(__KERNEL__)
CRONO_KERNEL_API uint32_t
CRONO_KERNEL_PciDeviceOpen(CRONO_KERNEL_DEVICE_HANDLE *phDev,
                           const CRONO_KERNEL_PCI_CARD_INFO *pDeviceInfo);

CRONO_KERNEL_API uint32_t
CRONO_KERNEL_PciDeviceClose(CRONO_KERNEL_DEVICE_HANDLE hDev);

#endif

/* -----------------------------------------------
    Set card cleanup commands
   ----------------------------------------------- */
CRONO_KERNEL_API uint32_t
CRONO_KERNEL_CardCleanupSetup(CRONO_KERNEL_DEVICE_HANDLE hDev,
                              CRONO_KERNEL_CMD *Cmd, uint32_t dwCmdCount);

/* -----------------------------------------------
    Read/Write memory and I/O addresses
   ----------------------------------------------- */

/* Read/write a device's address space (8/16/32/64 bits) of BAR0*/
CRONO_KERNEL_API uint32_t CRONO_KERNEL_ReadAddr8(
    CRONO_KERNEL_DEVICE_HANDLE hDev, uint32_t dwOffset, uint8_t *val);
CRONO_KERNEL_API uint32_t CRONO_KERNEL_ReadAddr16(
    CRONO_KERNEL_DEVICE_HANDLE hDev, uint32_t dwOffset, uint16_t *val);
CRONO_KERNEL_API uint32_t CRONO_KERNEL_ReadAddr32(
    CRONO_KERNEL_DEVICE_HANDLE hDev, uint32_t dwOffset, uint32_t *val);
CRONO_KERNEL_API uint32_t CRONO_KERNEL_ReadAddr64(
    CRONO_KERNEL_DEVICE_HANDLE hDev, uint32_t dwOffset, uint64_t *val);

CRONO_KERNEL_API uint32_t CRONO_KERNEL_WriteAddr8(
    CRONO_KERNEL_DEVICE_HANDLE hDev, uint32_t dwOffset, uint8_t val);
CRONO_KERNEL_API uint32_t CRONO_KERNEL_WriteAddr16(
    CRONO_KERNEL_DEVICE_HANDLE hDev, uint32_t dwOffset, uint16_t val);
CRONO_KERNEL_API uint32_t CRONO_KERNEL_WriteAddr32(
    CRONO_KERNEL_DEVICE_HANDLE hDev, uint32_t dwOffset, uint32_t val);
CRONO_KERNEL_API uint32_t CRONO_KERNEL_WriteAddr64(
    CRONO_KERNEL_DEVICE_HANDLE hDev, uint32_t dwOffset, uint64_t val);

/* -----------------------------------------------
    Access PCI configuration space
   ----------------------------------------------- */
/* Read/write 8/16/32/64 bits from the PCI configuration space.
   Identify device by handle */
CRONO_KERNEL_API uint32_t CRONO_KERNEL_PciReadCfg32(
    CRONO_KERNEL_DEVICE_HANDLE hDev, uint32_t dwOffset, uint32_t *val);

CRONO_KERNEL_API uint32_t CRONO_KERNEL_PciWriteCfg32(
    CRONO_KERNEL_DEVICE_HANDLE hDev, uint32_t dwOffset, uint32_t val);

CRONO_KERNEL_API uint32_t CRONO_KERNEL_PciWriteCfg32Arr(
    CRONO_KERNEL_DEVICE_HANDLE hDev, uint32_t dwOffset, uint32_t *val,
    uint32_t arr_size);

/* -----------------------------------------------
    DMA (Direct Memory Access)
   ----------------------------------------------- */
/* Allocate and lock a contiguous DMA buffer */
// dwOptions are:	DMA_KERNEL_BUFFER_ALLOC, DMA_KBUF_BELOW_16M,
//					DMA_LARGE_BUFFER, DMA_ALLOW_CACHE,
// DMA_KERNEL_ONLY_MAP, 					DMA_FROM_DEVICE,
// DMA_TO_DEVICE, DMA_ALLOW_64BIT_ADDRESS
CRONO_KERNEL_API uint32_t CRONO_KERNEL_DMAContigBufLock(
    CRONO_KERNEL_DEVICE_HANDLE hDev, void **ppBuf, uint32_t dwOptions,
    uint32_t dwDMABufSize, CRONO_KERNEL_DMA_CONTIG **ppDma);

/* Lock a Scatter/Gather DMA buffer */
// dwOptions are:	DMA_KERNEL_BUFFER_ALLOC, DMA_KBUF_BELOW_16M,
//					DMA_LARGE_BUFFER, DMA_ALLOW_CACHE,
// DMA_KERNEL_ONLY_MAP, 					DMA_FROM_DEVICE,
// DMA_TO_DEVICE, DMA_ALLOW_64BIT_ADDRESS
CRONO_KERNEL_API uint32_t CRONO_KERNEL_DMASGBufLock(
    CRONO_KERNEL_DEVICE_HANDLE hDev, void *pBuf, uint32_t dwOptions,
    uint32_t dwDMABufSize, CRONO_KERNEL_DMA_SG **ppDma);

/* Unlock a DMA buffer */
CRONO_KERNEL_API uint32_t CRONO_KERNEL_DMAContigBufUnlock(
    CRONO_KERNEL_DEVICE_HANDLE hDev, CRONO_KERNEL_DMA_CONTIG *pDma);

/* Unlock a DMA buffer */
CRONO_KERNEL_API uint32_t CRONO_KERNEL_DMASGBufUnlock(
    CRONO_KERNEL_DEVICE_HANDLE hDev, CRONO_KERNEL_DMA_SG *pDma);

/* -----------------------------------------------
    General
   ----------------------------------------------- */
/* PCI/PCMCIA device ID */
typedef union {
        CRONO_KERNEL_PCI_ID pciId;
} CRONO_KERNEL_ID_U;

#define CRONO_KERNEL_BAR_FLAG_32_BIT

typedef struct {
        uint32_t barNum; // number of the BAR (BAR0-5) not the index
        uint32_t flags;
        uint64_t userAddress;
        uint64_t physicalAddress;
        uint32_t length;
} CRONO_KERNEL_BAR_DESC;

/**
 * @brief Get device BAR Memory information for all present BARs (upto 6).
 * Memory addresses will not be valid after device is closed.
 *
 * @param hDev[in]: A valid handle to the device.
 * @param barCount[out]: Number of BarDesc objects filled
 * @param barDescs[in/out]: Must point to an array of 6 CRONO_KERNEL_BAR_DESC
 * structure objects.
 *
 * @return CRONO_SUCCESS in case of no error, or errno in case of error.
 */
CRONO_KERNEL_API uint32_t CRONO_KERNEL_GetBarDescriptions(
    CRONO_KERNEL_DEVICE_HANDLE hDev, uint32_t *barCount,
    CRONO_KERNEL_BAR_DESC *barDescs);

#ifdef __linux__
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
#ifdef __linux__
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
#endif // #ifdef __linux__

/**
 * @brief Get device BAR Memory information.
 *
 * @param hDev[in]: A valid handle to the device.
 * @param pAddr[out]: Memory address.
 * @param pSize[out]: Size of memory in Bytes.
 *
 * @return `CRONO_SUCCESS` in case of no error, or `errno` in case of error.
 */
CRONO_KERNEL_API uint32_t CRONO_KERNEL_GetDeviceBARMem(
    CRONO_KERNEL_DEVICE_HANDLE hDev, uint64_t *pBARUSAddr, size_t *pBARMemSize);

/**
 * @brief Get the device misc driver name.
 *
 * @param hDev[in]: A valid handle to the device.
 * @param pMiscName[out]: Buffer to hold the name.
 * @param nBuffSize[out]: Size of buffer `pMiscName` in Bytes.
 *
 * @return `CRONO_SUCCESS` in case of no error, or `errno` in case of error.
 */
CRONO_KERNEL_API uint32_t CRONO_KERNEL_GetDeviceMiscName(
    CRONO_KERNEL_DEVICE_HANDLE hDev, char *pMiscName, int nBuffSize);
#endif // #ifdef __linux__
#ifdef __cplusplus
}
#endif

#endif /* __CRONO_KERNEL_INTERFACE_H__ */
