#include "crono_kernel_interface.h"
#include "crono_linux_kernel.h"
#include "crono_userspace.h"

int crono_read_config(unsigned domain, unsigned bus, unsigned dev,
                      unsigned func, void *data, pciaddr_t offset,
                      pciaddr_t size, pciaddr_t *bytes_read) {

        char config_file_path[PATH_MAX];
        pciaddr_t temp_size = size;
        int err = CRONO_SUCCESS;
        int fd;
        char *data_bytes = (char *)data;
        // Uncomment for detailed debug
        // #ifdef CRONO_DEBUG_ENABLED
        //         pciaddr_t byte_index;
        // #endif
        if (bytes_read != NULL) {
                *bytes_read = 0;
        }

        CRONO_CONSTRUCT_CONFIG_FILE_PATH(config_file_path, domain, bus, dev,
                                         func);
        fd = open(config_file_path, O_RDONLY | O_CLOEXEC);
        if (fd == -1) {
                CRONO_DEBUG("Error reading configuration of file <%s>\n",
                            config_file_path);
                return errno;
        }

        while (temp_size > 0) {
                const ssize_t bytes =
                    pread64(fd, data_bytes, temp_size, offset);

                /* If zero bytes were read, then we assume it's the end of the
                 * config file.
                 */
                if (bytes == 0)
                        break;
                if (bytes < 0) {
                        err = errno;
                        break;
                }

                temp_size -= bytes;
                offset += bytes;
                data_bytes += bytes;
        }

        if (bytes_read != NULL) {
                *bytes_read = size - temp_size;
        }
        close(fd);

#ifdef CRONO_DEBUG_ENABLED
        // Uncomment for detailed debug
        // printf("Reading configuration at index <0x%08lX>, size <%ld>"
        //        ", file path <%s>\n",
        //        offset, size, config_file_path);
        // for (byte_index = 0; byte_index < size; byte_index++) {
        //         printf("Read byte #%03ld: <0x%02X>\n", byte_index,
        //                ((unsigned char *)data)[byte_index]);
        // }
#endif
        return err;
}

int crono_read_vendor_device(unsigned domain, unsigned bus, unsigned dev,
                             unsigned func, uint16_t *pVendor,
                             uint16_t *pDevice) {
        pciaddr_t bytes_read;
        int err = CRONO_SUCCESS;

        // Read Vendor ID and Device ID together from configuration space,
        // offset:0, of 4-bytes length. So, offset is DWORD-aligned for the
        // full value.
        uint32_t vendor_device_val;
        err = crono_read_config(domain, bus, dev, func, &vendor_device_val, 0,
                                4, &bytes_read);
        if ((bytes_read != 4) || err) {
                return err;
        }

        *pVendor = (uint16_t)(vendor_device_val & 0xFFFF);
        *pDevice = (uint16_t)(vendor_device_val >> 16);

#ifdef CRONO_DEBUG_ENABLED // Uncomment if device is not recognized
        // printf("Read vendor <0x%04X>\n", *pVendor);
        // printf("Read device <0x%04X>\n", *pDevice);
#endif

        return err;
}

int crono_get_config_space_size(unsigned domain, unsigned bus, unsigned dev,
                                unsigned func, pciaddr_t *pSize) {
        struct stat st;
        char config_file_path[PATH_MAX];

        CRONO_CONSTRUCT_CONFIG_FILE_PATH(config_file_path, domain, bus, dev,
                                         func);
        if (stat(config_file_path, &st) != 0) {
                printf(
                    "Error %d: Error getting configuration file stat of %s.\n",
                    errno, config_file_path);
                return errno;
        }
        if (NULL != pSize) {
                *pSize = st.st_size;
        }
        return CRONO_SUCCESS;
}

int crono_write_config(unsigned domain, unsigned bus, unsigned dev,
                       unsigned func, void *data, pciaddr_t offset,
                       pciaddr_t size, pciaddr_t *bytes_written) {
        char config_file_path[PATH_MAX];
        pciaddr_t temp_size = size;
        int err = CRONO_SUCCESS;
        int fd;
        const char *data_bytes = (char *)data;

        if (bytes_written != NULL) {
                *bytes_written = 0;
        }

        CRONO_CONSTRUCT_CONFIG_FILE_PATH(config_file_path, domain, bus, dev,
                                         func);

        fd = open(config_file_path, O_WRONLY | O_CLOEXEC);
        if (fd == -1) {
                return errno;
        }

        while (temp_size > 0) {
                const ssize_t bytes =
                    pwrite64(fd, data_bytes, temp_size, offset);
                // No logging here to save logging milliseconds performance
                // CRONO_DEBUG("Writing: File Path %s, Data %d, Size %lu, Offset
                // "
                //       "%08lx, Written Size %ld\n",
                //       config_file_path, *((unsigned int *)data), size,
                //       offset, bytes);

                /* If zero bytes were written, then we assume it's the end of
                 * the config file.
                 */
                if (bytes == 0)
                        break;
                if (bytes < 0) {
                        err = errno;
                        break;
                }
                temp_size -= bytes;
                offset += bytes;
                data_bytes += bytes;
        }

        if (bytes_written != NULL) {
                *bytes_written = size - temp_size;
        }

        close(fd);
        return err;
}

int crono_get_sys_devices_directory_path(unsigned domain, unsigned bus,
                                         unsigned dev, unsigned func,
                                         char *pPath) {
        char dev_slink_path[PATH_MAX];
        char dev_slink_content_path[PATH_MAX - 16]; // 16 = "/sys/" + 9
        ssize_t dev_slink_content_len = 0;

        CRONO_RET_INV_PARAM_IF_NULL(pPath);

        // Get the /sys/devices/ symbolic link content
        // e.g. /sys/bus/pci/devices/0000:03:00.0 symbolic link
        CRONO_CONSTRUCT_DEV_SLINK_PATH(dev_slink_path, domain, bus, dev, func);
        dev_slink_content_len =
            readlink(dev_slink_path, dev_slink_content_path, PATH_MAX - 16);
        if (-1 == dev_slink_content_len) {
                CRONO_DEBUG("Error reading link <%s> for %d, %d, %d, %d\n",
                            dev_slink_path, domain, bus, dev, func);
                return errno;
        }
        // e.g. ../../../devices/pci0000:00/0000:00:1c.7/0000:03:00.0
        dev_slink_content_path[dev_slink_content_len] = '\0';

        // Build pPath
        sprintf(pPath, "/sys/%s", &(dev_slink_content_path[9]));

        return CRONO_SUCCESS;
}
