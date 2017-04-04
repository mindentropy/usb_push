#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>

#include <libusb-1.0/libusb.h>
#include "usb_push.h"


static libusb_context *usb_context;
static libusb_device **usb_dev_list;

static struct libusb_device * find_s3c2440_device(void)
{
	ssize_t device_list_size = 0;
	struct libusb_device_descriptor desc;
	int i = 0;

	device_list_size =
			libusb_get_device_list(usb_context,
				&usb_dev_list);

/*	printf("Found %lu devices \n",device_list_size); */

	while((device_list_size != 0) && (usb_dev_list[i] != NULL)) {

		libusb_get_device_descriptor(usb_dev_list[i], &desc);

		if((desc.idVendor == MINI2440_VENDOR_ID) &&
			(desc.idProduct == MINI2440_PRODUCT_ID)) {
			return usb_dev_list[i];
		}

		i++;
	}

	return NULL;
}

static uint16_t checksum(const uint8_t *data,
							uint32_t len)
{
	uint16_t csum = 0;
	int i = 0;

	for(i = 0; i < len; i++) {
		csum += data[i];
	}

	return csum;
}

static int send_data(
					libusb_device_handle *handle,
					uint32_t ram_base_addr,
					uint8_t *map_addr,
					uint32_t len
					)
{
	int ret_val = 0;
	uint16_t csum = checksum(map_addr, len);
	uint8_t *buff, *curr;
	uint32_t remain = 0;
	int32_t transferred = 0;

	/*
	 * The data contains the following header:
	 * RAM address: 4 bytes
	 * Data length: 4 bytes
	 *
	 * The data contains the following footer:
	 * Checksum: 2 bytes
	 *
	 * The extra bytes apart from the data is 4 + 4 + 2 = 10 bytes
	 * Hence total size = 10 bytes (header + footer) + data_size
	 *
	 */

	uint32_t total_size = len + 10;

	printf("Checksum : 0x%4x\n",csum);

	buff = malloc(total_size);

	if(buff == NULL) {
		return -ENOMEM;
	}

	buff[0] = ram_base_addr & 0xFF;
	buff[1] = (ram_base_addr >> 8) & 0xFF;
	buff[2] = (ram_base_addr >> 16) & 0xFF;
	buff[3] = (ram_base_addr >> 24) & 0xFF;

	buff[4] = (total_size) & 0xFF;
	buff[5] = (total_size >> 8) & 0xFF;
	buff[6] = (total_size >> 16) & 0xFF;
	buff[7] = (total_size >> 24) & 0xFF;

	memcpy(buff+8,map_addr,len);

	/* Add an offset of 8 as the buff starts at an offset of 8 i.e. the header */
	buff[len + 8] = csum & 0xFF;
	buff[len + 9] = (csum >> 8) & 0xFF;

	printf("Sending file to address: 0x%x having length : %u bytes\n",
						ram_base_addr,
						len);

	for(curr = buff; curr < (buff + total_size); curr += WRITE_CHUNK_SIZE) {
		remain = (buff + total_size) - curr;

		if(remain > WRITE_CHUNK_SIZE) {
			remain = WRITE_CHUNK_SIZE;
		}

		ret_val = libusb_bulk_transfer(handle,
					MINI2440_EP_3_OUT,
					curr,
					remain,
					&transferred,
					0
					);

		if(ret_val < 0)
			break;

	}

	free(buff);

	return ret_val;
}


static void print_usage()
{
	printf("Usage: usb_push <filename> <ram_base_address> \n");
}

static void usb_print_error(const char *err_str, int errcode)
{
#if defined(LIBUSB_API_VERSION) && (LIBUSB_API_VERSION >= 0x01000104)
fprintf(stderr,"%s. Reason: %s\n",
							err_str,
							libusb_strerror(errcode)
							);
#else
fprintf(stderr,"%s. Error code: %d\n",
				err_str,
				errcode
				);
#endif
}

int main(int argc, char **argv)
{
	struct libusb_device *s3c_usb_dev = NULL;
	libusb_device_handle *handle = NULL;
	int retval = 0, fd = 0;
	struct stat st;
	uint8_t *file_mm = NULL;
	uint32_t ram_base_addr = 0;

	if(argc != ARG_SIZE) {
		print_usage();
		return 0;
	}

	libusb_init(&usb_context);

	s3c_usb_dev = find_s3c2440_device();

	if(s3c_usb_dev == NULL) {
		fprintf(stderr, "MINI2440 device not found\n");
		goto end;
	}

	printf("MINI2440 device found\n");

	if((retval = libusb_open(s3c_usb_dev,
				&handle)) != 0) {

		usb_print_error("Could not open device",retval);
		/*printf("Could not open device ");
		printf("Reason: %s\n",libusb_strerror(retval));*/

		goto end;
	}

	/* Claiming interface 0 as per lsusb there is only 1 interface starting with 0*/
	if(libusb_claim_interface(handle,
					MINI2440_USB_IF_NUM) != 0) {
		usb_print_error("Could not claim interface",retval);
		/*printf("Could not claim interface\n");*/
		goto end;
	}

/*	printf("Claimed interface\n"); */

	ram_base_addr = atof(argv[2]);
	printf("Ram base addr : 0x%x\n",ram_base_addr);

	fd = open(argv[1],O_RDONLY);

	if(fd < 0) {
		printf("Could not open file\n");
		goto file_error;
	}

	if((retval = fstat(fd, &st)) < 0) {
		perror("usb_push");
		goto file_error;
	}

	file_mm = mmap(NULL,
					st.st_size,
					PROT_READ,
					MAP_SHARED,
					fd,
					0);

	if(file_mm == NULL) {
		close(fd);
		perror("usb_push");
		goto file_error;
	}

	retval = send_data(handle,
			RAM_BASE_ADDR,
			file_mm,
			st.st_size);

	if(retval < 0) {
		usb_print_error("Send data error",retval);
	/*	printf("Send data error. Reason: %s\n",
								libusb_strerror(retval));*/
	} else {
		printf("Data send successful\n");
	}

	munmap(file_mm,
			st.st_size);
	close(fd);

file_error:
	libusb_release_interface(handle,
							MINI2440_USB_IF_NUM);
end:
	libusb_free_device_list(usb_dev_list,1);
	libusb_exit(usb_context);

	return 0;
}
