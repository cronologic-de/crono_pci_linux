#include "crono_kernel_interface.h"
#include "crono_userspace.h"

int crono_read_config(unsigned domain, unsigned bus, unsigned dev,
                      unsigned func, void *data, pciaddr_t offset,
                      pciaddr_t size, pciaddr_t *bytes_read) {

        char config_file_path[PATH_MAX];
        pciaddr_t temp_size = size;
        int err = CRONO_SUCCESS;
        int fd;
        char *data_bytes = (char *)data;
#ifdef CRONO_DEBUG_ENABLED
        pciaddr_t byte_index;
#endif
        if (bytes_read != NULL) {
                *bytes_read = 0;
        }

        CRONO_CONSTRUCT_CONFIG_FILE_PATH(config_file_path, domain, bus, dev,
                                         func);

        CRONO_DEBUG("Reading <%ld> bytes of file <%s>\n", size,
                    config_file_path);

        fd = open(config_file_path, O_RDONLY | O_CLOEXEC);
        if (fd == -1) {
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
        for (byte_index = 0; byte_index < size; byte_index++) {
                printf("Read byte #%03ld: <0x%02X>\n", byte_index,
                       ((unsigned char *)data)[byte_index]);
        }
#endif
        return err;
}

int crono_read_vendor_device(unsigned domain, unsigned bus, unsigned dev,
                             unsigned func, unsigned *pVendor,
                             unsigned *pDevice) {
        pciaddr_t bytes_read;
        int err = CRONO_SUCCESS;

        // Read Vendor ID from configuration space, offset:0, of 2-bytes length
        *pVendor = 0;
        err = crono_read_config(domain, bus, dev, func, pVendor, 0, 2,
                                &bytes_read);
        if ((bytes_read != 2) || err) {
                return err;
        }

        // Read Device ID from configuration space, offset:2, of 2-bytes length
        *pDevice = 0;
        err = crono_read_config(domain, bus, dev, func, pDevice, 2, 2,
                                &bytes_read);
        if ((bytes_read != 2) || err) {
                return err;
        }
        CRONO_DEBUG("Read Vendord ID: <0x%02x>, Device ID: <0x%02x>\n",
                    *pVendor, *pDevice);

        return err;
}

int crono_get_config_space_size(unsigned domain, unsigned bus, unsigned dev,
                                unsigned func, pciaddr_t *pSize) {
        struct stat st;
        char config_file_path[PATH_MAX];

        CRONO_CONSTRUCT_CONFIG_FILE_PATH(config_file_path, domain, bus, dev,
                                         func);
        if (stat(config_file_path, &st) != 0) {
                printf("Error %d: Error getting configuration file stat.\n",
                       errno);
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
                printf("Writing: File Path %s, Data %d, Size %lu, Offset "
                       "%08lx, Written Size %ld\n",
                       config_file_path, *((unsigned int *)data), size, offset,
                       bytes);
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
        char dev_slink_content_path[PATH_MAX];
        ssize_t dev_slink_content_len = 0;

        CRONO_RET_INV_PARAM_IF_NULL(pPath);

        // Get the /sys/devices/ symbolic link content
        // e.g. /sys/bus/pci/devices/0000:03:00.0 symbolic link
        CRONO_CONSTRUCT_DEV_SLINK_PATH(dev_slink_path, domain, bus, dev, func);
        dev_slink_content_len =
            readlink(dev_slink_path, dev_slink_content_path, PATH_MAX - 1);
        if (-1 == dev_slink_content_len) {
                return errno;
        }
        // e.g. ../../../devices/pci0000:00/0000:00:1c.7/0000:03:00.0
        dev_slink_content_path[dev_slink_content_len] = '\0';

        // Build pPath
        sprintf(pPath, "/sys/%s", &(dev_slink_content_path[9]));

        return CRONO_SUCCESS;
}

int crono_get_BAR0_file_path(unsigned domain, unsigned bus, unsigned dev,
                             unsigned func, char *pPath) {
        char sys_dev_dir_path[PATH_MAX];
        int err;

        // Init variables and validate parameters
        CRONO_RET_INV_PARAM_IF_NULL(pPath);

        // Get the /sys/device directory path
        err = crono_get_sys_devices_directory_path(domain, bus, dev, func,
                                                   sys_dev_dir_path);
        if (CRONO_SUCCESS != err) {
                printf("Path Error %d\n", err);
                return err;
        }

        // Construct the BAR0 resource file path
        snprintf(pPath, PATH_MAX, "%s/resource0", sys_dev_dir_path);

        // Success
        return CRONO_SUCCESS;
}

int crono_get_BAR0_file_size(unsigned domain, unsigned bus, unsigned dev,
                             unsigned func, pciaddr_t *pSize) {
        char BAR0_resource_path[PATH_MAX];
        struct stat st;
        int err;

        // Init variables and validate parameters
        CRONO_RET_INV_PARAM_IF_NULL(pSize);
        if (CRONO_SUCCESS !=
            (err = crono_get_BAR0_file_path(domain, bus, dev, func,
                                            BAR0_resource_path))) {
                return err;
        }

        // Get the resource file stat
        if (stat(BAR0_resource_path, &st) != 0) {
                printf("Error %d: Error getting BAR resource file stat.\n",
                       errno);
                return errno;
        }
        *pSize = st.st_size;

        return CRONO_SUCCESS;
}

int crono_get_BAR0_mem_addr(unsigned domain, unsigned bus, unsigned dev,
                            unsigned func, pciaddr_t dwOffset, pciaddr_t *size,
                            void **base_mem_addr, void **data_mem_addr) {
        char BAR0_resource_path[PATH_MAX];
        int err, fd = -1;
        pciaddr_t BAR0_full_mem_size;

        // Init variables and validate parameters
        CRONO_RET_INV_PARAM_IF_NULL(base_mem_addr);
        *base_mem_addr = NULL;

        // Get size and open BAR0 resource sysfs file
        if (CRONO_SUCCESS !=
            (err = crono_get_BAR0_file_size(domain, bus, dev, func,
                                            &BAR0_full_mem_size))) {
                return err;
        }
        if (dwOffset > BAR0_full_mem_size) {
                return ENOMEM;
        }
        if (0 == (*size)) {
                // Get BAR0 full memory
                *size = (pciaddr_t)BAR0_full_mem_size;
                // printf("\n*size = %ld, BAR0_full_mem_size: %ld\n", *size,
                // BAR0_full_mem_size) ;
        }
        if (CRONO_SUCCESS !=
            (err = crono_get_BAR0_file_path(domain, bus, dev, func,
                                            BAR0_resource_path))) {
                return err;
        }
        fd = open(BAR0_resource_path, O_RDWR | O_SYNC);
        if (fd < 0) {
                if (13 == errno) {
                        printf("Error %d: Cannot open resource0 <%s>, try "
                               "using sudo\n",
                               errno, BAR0_resource_path);
                } else {
                        printf("Error %d: Cannot open resource0 <%s>\n", errno,
                               BAR0_resource_path);
                }
                return errno;
        }

        // Calculate offset
        uint64_t offset_page_base_address, data_offset_from_page_base;
        // mmap offset must be a multiple of the page size
        // Get the offset of the requested page
        offset_page_base_address = PAGE_SIZE * (dwOffset / PAGE_SIZE);
        data_offset_from_page_base = dwOffset % PAGE_SIZE;

        // printf("\nmmap: size = %lu, offset: %lu\n",
        //    data_offset_from_page_base + (*size), offset_page_base_address) ;

        // Mapping
        *base_mem_addr = mmap(NULL, // Let kernel chooses the (page-aligned)
                                    // address at which to create the mapping
                              data_offset_from_page_base + (*size),
                              PROT_READ | PROT_WRITE, // Read & Write
                              MAP_SHARED, fd, offset_page_base_address);
        if (MAP_FAILED == (*base_mem_addr)) {
                printf("Error %d: mmap\n", errno);
                err = errno;
                goto mmap_error;
        }

        // Set the parameters
        // Set 'data_mem_addr'
        if (NULL != data_mem_addr) {
                // Set 'data_mem_addr' to the address of data memory @
                // 'dwOffset' regardless of the `dwOffset` page-alignment.
                *data_mem_addr = ((unsigned char *)(*base_mem_addr)) +
                                 data_offset_from_page_base;
        }
        // Set size to the TOTAL size of the memory mapped.
        *size = data_offset_from_page_base + (*size);

        // Clean up
        // After the mmap() call has returned, the file descriptor, fd, can
        // be closed immediately without invalidating the mapping.
        close(fd);

        // Success
        return CRONO_SUCCESS;

mmap_error:
        if (fd >= 0) {
                close(fd);
        }
        return err;
}
