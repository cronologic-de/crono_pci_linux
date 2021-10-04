#include "crono_userspace.h"
#include "crono_kernel_interface.h"

int main() {
        CRONO_KERNEL_PCI_SCAN_RESULT PCIScanResult;

        printf("\nScanning all PCI devices: \n");
        int scan_ret =
            CRONO_KERNEL_PciScanDevices(PCI_ANY_ID, PCI_ANY_ID, &PCIScanResult);
        if (CRONO_SUCCESS == scan_ret) {
                for (uint32_t pci_index = 0;
                     pci_index < PCIScanResult.dwNumDevices; pci_index++) {
                        printf("Device %02d ==> Vendor ID: 0x%04x, Device ID: "
                               "0x%04x, DBDF: %04x:%02x:%02x:%x\n",
                               pci_index,
                               (uint16_t)(PCIScanResult.deviceId[pci_index]
                                              .dwVendorId),
                               (uint16_t)(PCIScanResult.deviceId[pci_index]
                                              .dwDeviceId),
                               (uint16_t)(PCIScanResult.deviceSlot[pci_index]
                                              .dwDomain),
                               (uint8_t)(PCIScanResult.deviceSlot[pci_index]
                                             .dwBus),
                               (uint8_t)(PCIScanResult.deviceSlot[pci_index]
                                             .dwSlot),
                               (uint8_t)(PCIScanResult.deviceSlot[pci_index]
                                             .dwFunction));
                }
        }

        return 0;
}
